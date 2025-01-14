// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_FILE_SYSTEM_PROVIDER_CONTENT_CACHE_CONTEXT_DATABASE_H_
#define CHROME_BROWSER_ASH_FILE_SYSTEM_PROVIDER_CONTENT_CACHE_CONTEXT_DATABASE_H_

#include "base/sequence_checker.h"
#include "base/threading/sequence_bound.h"
#include "sql/database.h"
#include "sql/meta_table.h"

namespace ash::file_system_provider {

// The persistent data store for items that are cached via an FSP mount. There
// is 1:1 mapping of `ContextDatabase` per FSP mount and when the content cache
// is removed, the database is removed with it.
class ContextDatabase {
 public:
  explicit ContextDatabase(const base::FilePath& db_path);

  ContextDatabase(const ContextDatabase&) = delete;
  ContextDatabase& operator=(const ContextDatabase&) = delete;

  ~ContextDatabase();

  // Initialize the database either in-memory (if the constructor `db_path` was
  // empty) or at the path specified by the `db_path` in the constructor.
  bool Initialize();

  // Insert an item into the database and returns the unique ID that references
  // this item. In the event the `fsp_path` and `version_tag` have previously
  // been used, this will remove the conflicted record and insert a new one.
  bool AddItem(const base::FilePath& fsp_path,
               const std::string& version_tag,
               base::Time accessed_time,
               int64_t* inserted_id);

  // Represents a row returned from the SQLite database with the additional
  // field of `item_exists` that will be set to `false` if the item requested is
  // not available.
  struct Item {
    base::FilePath fsp_path;
    base::Time accessed_time;
    std::string version_tag;
    bool item_exists = true;
  };

  // Retrieve an item by `item_id`. If this returns false, indicates a more
  // fatal error. If this returns true the supplied `Item` will contain the data
  // or alternatively the field `item_exists` will be false indicating the
  // requested record is not available.
  bool GetItemById(int64_t item_id, Item& item);

  // Update the accessed time for the supplied `item_id`.
  bool UpdateAccessedTime(int64_t item_id, base::Time new_accessed_time);

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  // Remove the database and poison any subsequent requests.
  bool Raze();

  static const int kCurrentVersionNumber;
  static const int kCompatibleVersionNumber;

  const base::FilePath db_path_;
  sql::Database db_ GUARDED_BY_CONTEXT(sequence_checker_);
  sql::MetaTable meta_table_ GUARDED_BY_CONTEXT(sequence_checker_);
};

using OptionalContextDatabase = std::optional<std::unique_ptr<ContextDatabase>>;
using BoundContextDatabase =
    base::SequenceBound<std::unique_ptr<ContextDatabase>>;

}  // namespace ash::file_system_provider

#endif  // CHROME_BROWSER_ASH_FILE_SYSTEM_PROVIDER_CONTENT_CACHE_CONTEXT_DATABASE_H_
