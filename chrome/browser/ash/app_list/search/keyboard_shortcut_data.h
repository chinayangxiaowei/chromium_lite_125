// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_APP_LIST_SEARCH_KEYBOARD_SHORTCUT_DATA_H_
#define CHROME_BROWSER_ASH_APP_LIST_SEARCH_KEYBOARD_SHORTCUT_DATA_H_

#include <optional>
#include <string>

#include "ash/public/cpp/keyboard_shortcut_item.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace app_list {

// An intermediary class between keyboard shortcut data classes in //ash and
// the launcher backend. Hierarchy:
//
//   1) ash::KeyboardShortcutItem.
//   2) app_list::KeyboardShortcutData [this].
//   3) app_list::KeyboardShortcutResult. A subclass of ChromeSearchResult.
//
// Data needed for the launcher is extracted from a hard-coded list of (1)
// objects, and this data is stored in the form of (2). (2) is then later used
// to create (3) - the benefits of this are that (2) is more lightweight than
// (3), and can provide encapsulation of processing tasks.
class KeyboardShortcutData {
 public:
  explicit KeyboardShortcutData(const ash::KeyboardShortcutItem& item);
  // For testing purposes.
  explicit KeyboardShortcutData(const std::u16string description,
                                int description_message_id,
                                int shortcut_message_id);
  KeyboardShortcutData(const KeyboardShortcutData&);
  KeyboardShortcutData& operator=(const KeyboardShortcutData&) = default;

  ~KeyboardShortcutData();

  int description_message_id() const { return description_message_id_; }
  const std::u16string& description() const { return description_; }
  const std::optional<int> shortcut_message_id() const {
    return shortcut_message_id_;
  }
  const std::vector<ui::KeyboardCode> shortcut_key_codes() const {
    return shortcut_key_codes_;
  }
  const std::vector<double> embedding() const { return embedding_; }

  void SetEmbedding(std::vector<double>& embedding);

 private:
  // ID of the message resource describing the action the shortcut performs.
  int description_message_id_;

  // The description (corresponding to |description_message_id_|) of the
  // shortcut action e.g. "Dock a window on the right".
  std::u16string description_;

  // ID of the message template resource used to list the keys making up the
  // shortcut.
  std::optional<int> shortcut_message_id_;

  // The VKEY codes of the key and each modifier comprising the shortcut. See
  // ash/public/cpp/keyboard_shortcut_item.h for more detail.
  std::vector<ui::KeyboardCode> shortcut_key_codes_;

  // Embedding representation of the |description_| used for Manatee search.
  std::vector<double> embedding_;
};

}  // namespace app_list

#endif  // CHROME_BROWSER_ASH_APP_LIST_SEARCH_KEYBOARD_SHORTCUT_DATA_H_
