// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/dips/dips_redirect_info.h"

DIPSRedirectChainInfo::DIPSRedirectChainInfo(const GURL& initial_url,
                                             const GURL& final_url,
                                             size_t length,
                                             bool is_partial_chain)
    : initial_url(initial_url),
      initial_site(GetSiteForDIPS(initial_url)),
      final_url(final_url),
      final_site(GetSiteForDIPS(final_url)),
      initial_and_final_sites_same(initial_site == final_site),
      length(length),
      is_partial_chain(is_partial_chain) {}

DIPSRedirectChainInfo::DIPSRedirectChainInfo(const DIPSRedirectChainInfo&) =
    default;

DIPSRedirectChainInfo::~DIPSRedirectChainInfo() = default;

DIPSRedirectInfo::DIPSRedirectInfo(const GURL& url,
                                   DIPSRedirectType redirect_type,
                                   SiteDataAccessType access_type,
                                   ukm::SourceId source_id,
                                   base::Time time)
    : DIPSRedirectInfo(url,
                       redirect_type,
                       access_type,
                       source_id,
                       time,
                       /*client_bounce_delay=*/base::TimeDelta(),
                       /*has_sticky_activation=*/false,
                       /*web_authn_assertion_request_succeeded=*/false) {
  // This constructor should only be called for server-side redirects;
  // client-side redirects should call the constructor with extra arguments.
  DCHECK_EQ(redirect_type, DIPSRedirectType::kServer);
}

DIPSRedirectInfo::DIPSRedirectInfo(const GURL& url,
                                   DIPSRedirectType redirect_type,
                                   SiteDataAccessType access_type,
                                   ukm::SourceId source_id,
                                   base::Time time,
                                   base::TimeDelta client_bounce_delay,
                                   bool has_sticky_activation,
                                   bool web_authn_assertion_request_succeeded)
    : url(url),
      site(GetSiteForDIPS(url)),
      redirect_type(redirect_type),
      access_type(access_type),
      source_id(source_id),
      time(time),
      client_bounce_delay(client_bounce_delay),
      has_sticky_activation(has_sticky_activation),
      web_authn_assertion_request_succeeded(
          web_authn_assertion_request_succeeded) {}

DIPSRedirectInfo::DIPSRedirectInfo(const DIPSRedirectInfo&) = default;

DIPSRedirectInfo::~DIPSRedirectInfo() = default;
