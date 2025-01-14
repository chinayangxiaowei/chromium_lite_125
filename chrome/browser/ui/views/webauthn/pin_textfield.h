// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_WEBAUTHN_PIN_TEXTFIELD_H_
#define CHROME_BROWSER_UI_VIEWS_WEBAUTHN_PIN_TEXTFIELD_H_

#include <memory>
#include <vector>

#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/render_text.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/view.h"

// Implements textfield for entering a PIN number with custom drawing logic for
// displaying each digit in a separate cell.
class PinTextfield : public views::Textfield {
  METADATA_HEADER(PinTextfield, views::View)

 public:
  explicit PinTextfield(int pin_digits_count);
  ~PinTextfield() override;

  // Appends a digit to the next free pin cell. Does nothing if all pin digits
  // are already typed. Returns true if a digit was appended.
  bool AppendDigit(std::u16string digit);

  // Removes a digit from the last cell. Does nothing if no digits are typed.
  // Returns true if a digit was removed.
  bool RemoveDigit();

  // Returns currently typed pin.
  std::u16string GetPin();

  void SetObscured(bool obscured);

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize() const override;
  void OnThemeChanged() override;

 private:
  // Returns true for the first empty cell or the last cell when the full pin is
  // typed (when the whole view has focus).
  bool HasCellFocus(int cell) const;

  // Render text for each of the pin cells.
  std::vector<std::unique_ptr<gfx::RenderText>> render_texts_;

  // Amount of pin cells.
  const int pin_digits_count_;

  // Amount of digits that are currently typed.
  int digits_typed_count_ = 0;
};

#endif  // CHROME_BROWSER_UI_VIEWS_WEBAUTHN_PIN_TEXTFIELD_H_
