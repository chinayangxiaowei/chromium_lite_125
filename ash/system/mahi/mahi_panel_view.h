// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_MAHI_MAHI_PANEL_VIEW_H_
#define ASH_SYSTEM_MAHI_MAHI_PANEL_VIEW_H_

#include <optional>
#include <string>

#include "ash/ash_export.h"
#include "ash/system/mahi/mahi_ui_controller.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/layout/flex_layout_view.h"

namespace views {
class ImageView;
class Label;
class Textfield;
}  // namespace views

namespace ash {

class IconButton;
class MahiQuestionAnswerView;
class MahiUiUpdate;
class SummaryOutlinesSection;
enum class VisibilityState;

// The code for Mahi main panel view. This view is placed within
// `MahiPanelWidget`.
class ASH_EXPORT MahiPanelView : public views::FlexLayoutView,
                                 public views::TextfieldController,
                                 public MahiUiController::Delegate {
  METADATA_HEADER(MahiPanelView, views::FlexLayoutView)

 public:
  explicit MahiPanelView(MahiUiController* ui_controller);
  MahiPanelView(const MahiPanelView&) = delete;
  MahiPanelView& operator=(const MahiPanelView&) = delete;
  ~MahiPanelView() override;

 private:
  // views::TextfieldController:
  bool HandleKeyEvent(views::Textfield* textfield,
                      const ui::KeyEvent& key_event) override;

  // MahiUiController::Delegate:
  views::View* GetView() override;
  bool GetViewVisibility(VisibilityState state) const override;
  void OnUpdated(const MahiUiUpdate& update) override;

  // Creates the header row, which includes a back button (visible only
  // in the Q&A view), the panel title, an experiment badge and a close button.
  std::unique_ptr<views::View> CreateHeaderRow();

  // Callbacks for buttons and link.
  void OnCloseButtonPressed(const ui::Event& event);
  void OnLearnMoreLinkClicked();
  void OnBackButtonPressed();
  void OnSendButtonPressed();

  // `ui_controller_` will outlive `this`.
  const raw_ptr<MahiUiController> ui_controller_;

  // Owned by the views hierarchy.
  raw_ptr<views::View> back_button_ = nullptr;
  raw_ptr<views::ImageView> content_icon_ = nullptr;
  raw_ptr<views::Label> content_title_ = nullptr;
  raw_ptr<MahiQuestionAnswerView> question_answer_view_ = nullptr;
  raw_ptr<SummaryOutlinesSection> summary_outlines_section_ = nullptr;
  raw_ptr<views::Textfield> question_textfield_ = nullptr;
  raw_ptr<IconButton> send_button_ = nullptr;

  // The time when this view is constructed, which is when the user opens this
  // view.
  base::TimeTicks open_time_;

  base::WeakPtrFactory<MahiPanelView> weak_ptr_factory_{this};
};

}  // namespace ash

#endif  // ASH_SYSTEM_MAHI_MAHI_PANEL_VIEW_H_
