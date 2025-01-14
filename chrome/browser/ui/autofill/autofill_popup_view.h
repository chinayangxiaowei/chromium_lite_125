// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_AUTOFILL_AUTOFILL_POPUP_VIEW_H_
#define CHROME_BROWSER_UI_AUTOFILL_AUTOFILL_POPUP_VIEW_H_

#include <stddef.h>

#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/common/aliases.h"
#include "content/public/common/input/native_web_keyboard_event.h"

namespace autofill {

class AutofillPopupController;

// The interface for creating and controlling a platform-dependent
// AutofillPopupView.
class AutofillPopupView {
 public:
  // Factory function for creating the view.
  static base::WeakPtr<AutofillPopupView> Create(
      base::WeakPtr<AutofillPopupController> controller);

  // Attempts to display the Autofill popup and fills it with data from the
  // controller. Returns whether the popup was shown.
  virtual bool Show(AutoselectFirstSuggestion autoselect_first_suggestion) = 0;

  // Hides the popup from view. This will cause the popup to be deleted.
  virtual void Hide() = 0;

  // Handles a key press event and returns whether the event should be
  // swallowed. This allows views to handle events that depend on its internal
  // state, such as changing the selected Autofill cell.
  virtual bool HandleKeyPressEvent(
      const content::NativeWebKeyboardEvent& event) = 0;

  // Refreshes the position and redraws popup when suggestions change. Returns
  // whether the resulting popup was shown (or had to hide, e.g. due to
  // insufficient size).
  virtual void OnSuggestionsChanged() = 0;

  // Returns true if the autofill popup overlaps with the
  // picture-in-picture window.
  virtual bool OverlapsWithPictureInPictureWindow() const = 0;

  // Makes accessibility announcement.
  virtual void AxAnnounce(const std::u16string& text) = 0;

  // Return the autofill popup view's ax unique id.
  virtual std::optional<int32_t> GetAxUniqueId() = 0;

  // Creates a sub-popup (child) view linked to this (parent) view.
  // The child's lifetime depends on its parent, i.e. when the parent dies
  // the child dies also.
  virtual base::WeakPtr<AutofillPopupView> CreateSubPopupView(
      base::WeakPtr<AutofillPopupController> sub_controller) = 0;

  virtual std::optional<AutofillClient::PopupScreenLocation>
  GetPopupScreenLocation() const = 0;

  // Returns a weak pointer to itself.
  virtual base::WeakPtr<AutofillPopupView> GetWeakPtr() = 0;

 protected:
  virtual ~AutofillPopupView() {}
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_AUTOFILL_AUTOFILL_POPUP_VIEW_H_
