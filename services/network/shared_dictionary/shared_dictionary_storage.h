// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_SHARED_DICTIONARY_SHARED_DICTIONARY_STORAGE_H_
#define SERVICES_NETWORK_SHARED_DICTIONARY_SHARED_DICTIONARY_STORAGE_H_

#include <map>
#include <set>
#include <string>

#include "base/component_export.h"
#include "base/containers/contains.h"
#include "base/functional/callback.h"
#include "base/memory/ref_counted.h"
#include "base/strings/pattern.h"
#include "base/time/time.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"

class GURL;

namespace net {
class HttpResponseHeaders;
}  // namespace net

namespace network {
namespace mojom {
enum class RequestDestination : int32_t;
}  // namespace mojom

class SharedDictionary;
class SharedDictionaryWriter;

// Shared Dictionary Storage manages dictionaries for a particular
// net::SharedDictionaryIsolationKey.
class COMPONENT_EXPORT(NETWORK_SERVICE) SharedDictionaryStorage
    : public base::RefCounted<SharedDictionaryStorage> {
 public:
  SharedDictionaryStorage(const SharedDictionaryStorage&) = delete;
  SharedDictionaryStorage& operator=(const SharedDictionaryStorage&) = delete;

  // Returns a matching SharedDictionary for `url`. If the metadata has not been
  // read from the database, this method returns nullptr.
  virtual std::unique_ptr<SharedDictionary> GetDictionarySync(
      const GURL& url,
      mojom::RequestDestination destination) = 0;

  // If the metadata has already been read from the database, this method calls
  // `callback` synchronously with a matching `SharedDictionary`. Otherwise,
  // this method waits until the metadata is available, and then calls
  // `callback` with a matching `SharedDictionary`.
  virtual void GetDictionary(
      const GURL& url,
      mojom::RequestDestination destination,
      base::OnceCallback<void(std::unique_ptr<SharedDictionary>)> callback) = 0;

  // Returns a SharedDictionaryWriter if `headers` has a valid
  // `use-as-dictionary` header, and `access_allowed_check_callback`
  // returns true,
  scoped_refptr<SharedDictionaryWriter> MaybeCreateWriter(
      const GURL& url,
      const base::Time request_time,
      const base::Time response_time,
      const net::HttpResponseHeaders& headers,
      bool was_fetched_via_cache,
      base::OnceCallback<bool()> access_allowed_check_callback);

 protected:
  friend class base::RefCounted<SharedDictionaryStorage>;

  SharedDictionaryStorage();
  virtual ~SharedDictionaryStorage();

  // Called to create a SharedDictionaryWriter.
  virtual scoped_refptr<SharedDictionaryWriter> CreateWriter(
      const GURL& url,
      base::Time response_time,
      base::TimeDelta expiration,
      const std::string& match,
      const std::set<mojom::RequestDestination>& match_dest,
      const std::string& id) = 0;

  // Called to avoid registering the same dictionary from the disk cache.
  virtual bool IsAlreadyRegistered(
      const GURL& url,
      base::Time response_time,
      base::TimeDelta expiration,
      const std::string& match,
      const std::set<mojom::RequestDestination>& match_dest,
      const std::string& id) = 0;
};

// Returns a matching dictionary for `url` from `dictionary_info_map`.
// This is a template method because SharedDictionaryStorageInMemory and
// SharedDictionaryStorageOnDisk are using different class for
// DictionaryInfoType.
template <class DictionaryInfoType>
DictionaryInfoType* GetMatchingDictionaryFromDictionaryInfoMap(
    std::map<
        url::SchemeHostPort,
        std::map<std::tuple<std::string, std::set<mojom::RequestDestination>>,
                 DictionaryInfoType>>& dictionary_info_map,
    const GURL& url,
    mojom::RequestDestination destination) {
  auto it = dictionary_info_map.find(url::SchemeHostPort(url));
  if (it == dictionary_info_map.end()) {
    return nullptr;
  }
  DictionaryInfoType* matched_info = nullptr;
  for (auto& item : it->second) {
    DictionaryInfoType& info = item.second;
    CHECK(std::make_tuple(info.match(), info.match_dest()) == item.first);
    if (matched_info &&
        ((matched_info->match().size() > info.match().size()) ||
         (matched_info->match().size() == info.match().size() &&
          matched_info->response_time() > info.response_time()))) {
      continue;
    }
    // When `match_dest` is empty, we don't check the `destination`.
    if (!info.match_dest().empty() &&
        !base::Contains(info.match_dest(), destination)) {
      continue;
    }
    CHECK(info.matcher());
    if (info.matcher()->Match(url)) {
      matched_info = &info;
    }
  }
  return matched_info;
}

// Returns true if the same dictionary is already registered in
// `dictionary_info_map`. This is used to avoid registering the same dictionary
// from the disk cache.
// This is a template method because SharedDictionaryStorageInMemory and
// SharedDictionaryStorageOnDisk are using different class for
// DictionaryInfoType.
template <class DictionaryInfoType>
bool IsAlreadyRegisteredInDictionaryInfoMap(
    std::map<
        url::SchemeHostPort,
        std::map<std::tuple<std::string, std::set<mojom::RequestDestination>>,
                 DictionaryInfoType>>& dictionary_info_map,
    const GURL& url,
    base::Time response_time,
    base::TimeDelta expiration,
    const std::string& match,
    const std::set<mojom::RequestDestination>& match_dest,
    const std::string& id) {
  auto it1 = dictionary_info_map.find(url::SchemeHostPort(url));
  if (it1 == dictionary_info_map.end()) {
    return false;
  }
  auto it2 = it1->second.find(std::make_tuple(match, match_dest));
  if (it2 == it1->second.end()) {
    return false;
  }
  return it2->second.url() == url &&
         it2->second.response_time() == response_time &&
         it2->second.expiration() == expiration && it2->second.id() == id;
}

}  // namespace network

#endif  // SERVICES_NETWORK_SHARED_DICTIONARY_SHARED_DICTIONARY_STORAGE_H_
