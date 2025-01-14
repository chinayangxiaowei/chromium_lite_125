// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/ssl/ssl_client_auth_handler.h"

#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "net/ssl/client_cert_store.h"
#include "net/ssl/ssl_private_key.h"

namespace content {

class SSLClientAuthHandler::ClientCertificateDelegateImpl
    : public ClientCertificateDelegate {
 public:
  explicit ClientCertificateDelegateImpl(
      base::WeakPtr<SSLClientAuthHandler> handler)
      : handler_(std::move(handler)) {}

  ClientCertificateDelegateImpl(const ClientCertificateDelegateImpl&) = delete;
  ClientCertificateDelegateImpl& operator=(
      const ClientCertificateDelegateImpl&) = delete;

  ~ClientCertificateDelegateImpl() override {
    if (!continue_called_ && handler_) {
      handler_->delegate_->CancelCertificateSelection();
    }
  }

  // ClientCertificateDelegate implementation:
  void ContinueWithCertificate(scoped_refptr<net::X509Certificate> cert,
                               scoped_refptr<net::SSLPrivateKey> key) override {
    DCHECK(!continue_called_);
    continue_called_ = true;
    if (handler_) {
      handler_->delegate_->ContinueWithCertificate(std::move(cert),
                                                   std::move(key));
    }
  }

 private:
  base::WeakPtr<SSLClientAuthHandler> handler_;
  bool continue_called_ = false;
};

// A reference-counted core to allow the ClientCertStore and SSLCertRequestInfo
// to outlive SSLClientAuthHandler if needbe.
//
// TODO(davidben): Fix ClientCertStore's lifetime contract. See
// https://crbug.com/1011579.
class SSLClientAuthHandler::Core : public base::RefCountedThreadSafe<Core> {
 public:
  Core(const base::WeakPtr<SSLClientAuthHandler>& handler,
       std::unique_ptr<net::ClientCertStore> client_cert_store,
       net::SSLCertRequestInfo* cert_request_info)
      : handler_(handler),
        client_cert_store_(std::move(client_cert_store)),
        cert_request_info_(cert_request_info) {}

  void GetClientCerts() {
    if (client_cert_store_) {
      // TODO(davidben): This is still a cyclical ownership where
      // GetClientCerts' requirement that |client_cert_store_| remains alive
      // until the call completes is maintained by the reference held in the
      // callback.
      client_cert_store_->GetClientCerts(
          *cert_request_info_,
          base::BindOnce(&SSLClientAuthHandler::Core::DidGetClientCerts, this));
    } else {
      DidGetClientCerts(net::ClientCertIdentityList());
    }
  }

 private:
  friend class base::RefCountedThreadSafe<Core>;

  ~Core() {}

  // Called when |client_cert_store_| is done retrieving the cert list.
  void DidGetClientCerts(net::ClientCertIdentityList client_certs) {
    // Run this on a PostTask to avoid reentrancy problems.
    GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(&SSLClientAuthHandler::DidGetClientCerts,
                       std::move(handler_), std::move(client_certs)));
  }

  base::WeakPtr<SSLClientAuthHandler> handler_;
  std::unique_ptr<net::ClientCertStore> client_cert_store_;
  scoped_refptr<net::SSLCertRequestInfo> cert_request_info_;
};

SSLClientAuthHandler::SSLClientAuthHandler(
    std::unique_ptr<net::ClientCertStore> client_cert_store,
    base::WeakPtr<BrowserContext> browser_context,
    base::WeakPtr<WebContents> web_contents,
    net::SSLCertRequestInfo* cert_request_info,
    Delegate* delegate)
    : browser_context_(browser_context),
      web_contents_(web_contents),
      cert_request_info_(cert_request_info),
      delegate_(delegate) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (web_contents_) {
    CHECK_EQ(web_contents_->GetBrowserContext(), browser_context_.get());
  }

  core_ = new Core(weak_factory_.GetWeakPtr(), std::move(client_cert_store),
                   cert_request_info_.get());
}

SSLClientAuthHandler::~SSLClientAuthHandler() {
  // Invalidate our WeakPtrs in case invoking the cancellation callback would
  // cause |this| to be destructed again.
  weak_factory_.InvalidateWeakPtrs();
  if (cancellation_callback_) {
    std::move(cancellation_callback_).Run();
  }
}

void SSLClientAuthHandler::SelectCertificate() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // |core_| will call DidGetClientCerts when done.
  core_->GetClientCerts();
}

void SSLClientAuthHandler::DidGetClientCerts(
    net::ClientCertIdentityList client_certs) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (!browser_context_) {
    delegate_->CancelCertificateSelection();
    return;
  }

  // SelectClientCertificate() may call back into |delegate_| synchronously and
  // destroy this object, so guard the cancellation callback logic by a WeakPtr.
  base::WeakPtr<SSLClientAuthHandler> weak_self = weak_factory_.GetWeakPtr();
  base::OnceClosure cancellation_callback =
      GetContentClient()->browser()->SelectClientCertificate(
          browser_context_.get(), web_contents_.get(), cert_request_info_.get(),
          std::move(client_certs),
          std::make_unique<ClientCertificateDelegateImpl>(weak_self));
  if (weak_self) {
    cancellation_callback_ = std::move(cancellation_callback);
  } else if (!cancellation_callback.is_null()) {
    std::move(cancellation_callback).Run();
  }
}

}  // namespace content
