// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_FILE_MANAGER_INDEXING_FILE_INFO_TABLE_H_
#define CHROME_BROWSER_ASH_FILE_MANAGER_INDEXING_FILE_INFO_TABLE_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ash/file_manager/indexing/file_info.h"
#include "chrome/browser/ash/file_manager/indexing/url_table.h"
#include "sql/database.h"

namespace file_manager {

// Stores a mapping from URL IDs to FileInfo. This table is not intended to
// be used by itself, as it does not keep URLs. Instead, it is to be used by
// the SqlStorage class that can supply missing information from the url_table
// it owns.
class FileInfoTable {
 public:
  // Creates a table that maps URL IDs to FileInfo. It uses the
  // given `url_table` to
  explicit FileInfoTable(sql::Database* db);
  ~FileInfoTable();

  FileInfoTable(const FileInfoTable&) = delete;
  FileInfoTable& operator=(const FileInfoTable&) = delete;

  // Initializes the table. Returns true on success, and false on failure.
  bool Init();

  // Attempts to retrieve the unique FileInfo associated with the given URL.
  // Returns -1, if the file info was not found. Otherwise, returns URL ID and
  // the FileInfo object is populated with the retrieved data.
  // NO CHECK is performed whether the url_id corresponds to the `file_url`
  // field in the `info` object.
  int64_t GetFileInfo(int64_t url_id, FileInfo* info);

  // Attempts to store the given `info` in the table. If successful, it returns
  // the ID of the URL from the `info` object that was used to store the `info`
  // content. Otherwise, it returns -1. The `url_id` must be generated based on
  // the `file_url` field of the `info` object.
  // NO CHECK is performed whether the url_id corresponds to the `file_url`
  // field in the `info` object.
  int64_t PutFileInfo(int64_t url_id, const FileInfo& info);

  // Attempts to remove the given file info from the database. If not present,
  // this method returns -1. Otherwise, it returns the `url_id`.
  int64_t DeleteFileInfo(int64_t url_id);

 private:
  // The pointer to a database owned by the whoever created this table.
  raw_ptr<sql::Database> db_;
};

}  // namespace file_manager

#endif  // CHROME_BROWSER_ASH_FILE_MANAGER_INDEXING_FILE_INFO_TABLE_H_
