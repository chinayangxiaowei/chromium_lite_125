// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PICKER_VIEWS_PICKER_CONTENTS_VIEW_H_
#define ASH_PICKER_VIEWS_PICKER_CONTENTS_VIEW_H_

#include "ash/ash_export.h"
#include "ash/picker/views/picker_view.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

namespace ash {

// View for the main contents of the Picker.
// Consists of multiple "pages", with at most one page visible at a time.
class ASH_EXPORT PickerContentsView : public views::View {
  METADATA_HEADER(PickerContentsView, views::View)

 public:
  // Creates an empty view with no pages.
  explicit PickerContentsView(PickerView::PickerLayoutType layout_type);
  PickerContentsView(const PickerContentsView&) = delete;
  PickerContentsView& operator=(const PickerContentsView&) = delete;
  ~PickerContentsView() override;

  // Adds a new page.
  template <typename T>
  T* AddPage(std::unique_ptr<T> view) {
    view->SetVisible(false);
    return page_container_->AddChildView(std::move(view));
  }

  // Sets the visible page to be `view`.
  void SetActivePage(views::View* view);

  const views::View* page_container_for_testing() const {
    return page_container_;
  }

 private:
  raw_ptr<views::View> page_container_ = nullptr;
};

}  // namespace ash

#endif  // ASH_PICKER_VIEWS_PICKER_CONTENTS_VIEW_H_
