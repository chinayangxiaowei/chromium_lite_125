// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_cookie_access_policy.h"

#include <memory>

#include "android_webview/browser/aw_contents_io_thread_client.h"
#include "base/check_op.h"
#include "base/no_destructor.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/websocket_handshake_request_info.h"
#include "net/base/net_errors.h"
#include "net/cookies/site_for_cookies.h"
#include "net/cookies/static_cookie_policy.h"
#include "url/gurl.h"

using base::AutoLock;
using content::BrowserThread;
using content::WebSocketHandshakeRequestInfo;

namespace android_webview {

namespace {

using PrivacySetting = net::NetworkDelegate::PrivacySetting;

}  // namespace

AwCookieAccessPolicy::~AwCookieAccessPolicy() = default;

AwCookieAccessPolicy::AwCookieAccessPolicy() = default;

AwCookieAccessPolicy* AwCookieAccessPolicy::GetInstance() {
  static base::NoDestructor<AwCookieAccessPolicy> instance;
  return instance.get();
}

bool AwCookieAccessPolicy::GetShouldAcceptCookies() {
  AutoLock lock(lock_);
  return accept_cookies_;
}

void AwCookieAccessPolicy::SetShouldAcceptCookies(bool allow) {
  AutoLock lock(lock_);
  accept_cookies_ = allow;
}

bool AwCookieAccessPolicy::GetShouldAcceptThirdPartyCookies(
    base::optional_ref<const content::GlobalRenderFrameHostToken>
        global_frame_token,
    int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::unique_ptr<AwContentsIoThreadClient> io_thread_client;
  if (frame_tree_node_id != content::RenderFrameHost::kNoFrameTreeNodeId) {
    io_thread_client = AwContentsIoThreadClient::FromID(frame_tree_node_id);
  } else if (global_frame_token.has_value()) {
    io_thread_client =
        AwContentsIoThreadClient::FromToken(global_frame_token.value());
  }

  if (!io_thread_client) {
    return false;
  }
  return io_thread_client->ShouldAcceptThirdPartyCookies();
}

PrivacySetting AwCookieAccessPolicy::AllowCookies(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    base::optional_ref<const content::GlobalRenderFrameHostToken>
        global_frame_token,
    bool has_storage_access) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  bool third_party = GetShouldAcceptThirdPartyCookies(
      global_frame_token, content::RenderFrameHost::kNoFrameTreeNodeId);
  return CanAccessCookies(url, site_for_cookies, third_party,
                          has_storage_access);
}

PrivacySetting AwCookieAccessPolicy::CanAccessCookies(
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    bool accept_third_party_cookies,
    bool has_storage_access) {
  if (!accept_cookies_)
    return PrivacySetting::kStateDisallowed;

  if (accept_third_party_cookies)
    return PrivacySetting::kStateAllowed;

  // File URLs are a special case. We want file URLs to be able to set cookies
  // but (for the purpose of cookies) Chrome considers different file URLs to
  // come from different origins so we use the 'allow all' cookie policy for
  // file URLs.
  if (url.SchemeIsFile())
    return PrivacySetting::kStateAllowed;

  if (has_storage_access) {
    return PrivacySetting::kStateAllowed;
  }

  // Otherwise, block third-party cookies.
  bool should_allow_3pcs =
      net::StaticCookiePolicy(
          net::StaticCookiePolicy::BLOCK_ALL_THIRD_PARTY_COOKIES)
          .CanAccessCookies(url, site_for_cookies) == net::OK;

  return should_allow_3pcs ? PrivacySetting::kStateAllowed
                           : PrivacySetting::kPartitionedStateAllowedOnly;
}

}  // namespace android_webview
