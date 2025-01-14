// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/video_conference/bubble/set_camera_background_view.h"

#include "ash/public/cpp/image_util.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/camera/camera_effects_controller.h"
#include "ash/system/video_conference/bubble/bubble_view.h"
#include "ash/system/video_conference/bubble/bubble_view_ids.h"
#include "ash/system/video_conference/resources/grit/vc_resources.h"
#include "ash/system/video_conference/video_conference_tray_controller.h"
#include "ash/system/video_conference/video_conference_utils.h"
#include "ash/wallpaper/wallpaper_utils/sea_pen_metadata_utils.h"
#include "ash/webui/common/mojom/sea_pen.mojom.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/histogram_functions.h"
#include "skia/ext/image_operations.h"
#include "third_party/skia/include/core/SkPathBuilder.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/chromeos/styles/cros_tokens_color_mappings.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/image/image_skia_rep.h"
#include "ui/views/background.h"
#include "ui/views/controls/animated_image_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"

namespace ash::video_conference {

namespace {

using BackgroundImageInfo = CameraEffectsController::BackgroundImageInfo;

constexpr char kBackgroundImageButtonHistogramName[] =
    "Ash.VideoConferenceTray.BackgroundImageButton.Click";

constexpr char kCreateWithAiButtonHistogramName[] =
    "Ash.VideoConferenceTray.CreateWithAiButton.Click";

// Decides the margin for the `SetCameraBackgroundView`.
constexpr gfx::Insets kSetCameraBackgroundViewInsideBorderInsets =
    gfx::Insets::TLBR(10, 0, 0, 0);

constexpr gfx::Insets kImageLabelContainerInsideBorderInsets =
    gfx::Insets::TLBR(6, 0, 6, 0);

constexpr int kCreateImageButtonBetweenChildSpacing = 12;
constexpr int kSetCameraBackgroundViewBetweenChildSpacing = 10;
constexpr int kSetCameraBackgroundViewRadius = 16;
constexpr int kButtonHeight = 20;

constexpr int kMaxRecentBackgroundToDislay = 4;
constexpr int kRecentlyUsedImagesFullLength = 336;
constexpr int kRecentlyUsedImagesHeight = 64;
constexpr int kRecentlyUsedImagesSpacing = 10;

constexpr int kRecentlyUsedImageButtonId[] = {
    BubbleViewID::kBackgroundImage0,
    BubbleViewID::kBackgroundImage1,
    BubbleViewID::kBackgroundImage2,
    BubbleViewID::kBackgroundImage3,
};

// Helper for getting the size of each recently used images.
gfx::Size CalculateWantedImageSize(const int index, int image_count) {
  CHECK_LT(index, image_count);

  const int spacing = (image_count - 1) * kRecentlyUsedImagesSpacing;

  // If there is only 1 image, we only want that image to take half of the whole
  // area, not the full area.
  const int expected_width =
      image_count == 1
          ? (kRecentlyUsedImagesFullLength - kRecentlyUsedImagesSpacing) / 2
          : (kRecentlyUsedImagesFullLength - spacing) / image_count;

  return gfx::Size(expected_width, kRecentlyUsedImagesHeight);
}

CameraEffectsController* GetCameraEffectsController() {
  return Shell::Get()->camera_effects_controller();
}

// Returns a gradient lottie animation defined in the resource file for the
// `Create with AI` button.
std::unique_ptr<lottie::Animation> GetGradientAnimation(
    const ui::ColorProvider* color_provider) {
  std::optional<std::vector<uint8_t>> lottie_data =
      ui::ResourceBundle::GetSharedInstance().GetLottieData(
          IDR_VC_CREATE_WITH_AI_BUTTON_ANIMATION);
  CHECK(lottie_data.has_value());

  return std::make_unique<lottie::Animation>(
      cc::SkottieWrapper::UnsafeCreateSerializable(lottie_data.value()),
      video_conference_utils::CreateColorMapForGradientAnimation(
          color_provider));
}

// Image button for the recently used images as camera background.
class RecentlyUsedImageButton : public views::ImageButton {
  METADATA_HEADER(RecentlyUsedImageButton, views::ImageButton)

 public:
  RecentlyUsedImageButton(
      const gfx::ImageSkia& image,
      const std::string& metadata,
      int id,
      const base::RepeatingCallback<void()>& image_button_callback)
      : ImageButton(image_button_callback),
        check_icon_(&kBackgroundSelectionIcon,
                    cros_tokens::kCrosSysFocusRingOnPrimaryContainer) {
    SetID(id);
    background_image_ = image;

    SetImageModel(ButtonState::STATE_NORMAL,
                  ui::ImageModel::FromImageSkia(background_image_));

    // TODO(b/332573200): only construct this button when the metadata is
    // decodable.
    SetAccessibilityLabelFromMetadata(metadata);

    SetFlipCanvasOnPaintForRTLUI(false);
  }

  void SetSelected(bool selected) {
    if (selected_ == selected) {
      return;
    }
    selected_ = selected;
    SchedulePaint();
  }

 private:
  // ImageButton:
  void PaintButtonContents(gfx::Canvas* canvas) override {
    // If current image is selected, we need to draw the background image with
    // required boundary and then put the check mark on the top-left corner.
    if (selected_) {
      canvas->DrawImageInPath(background_image_, 0, 0, GetClipPath(),
                              cc::PaintFlags());
      canvas->DrawImageInt(check_icon_.GetImageSkia(GetColorProvider()), 0, 0);
    } else {
      // Otherwise, draw the normal background image.
      canvas->DrawImageInt(background_image_, 0, 0);
    }
  }

  // Extract medata then decode it.
  void SetAccessibilityLabelFromMetadata(const std::string& metadata) {
    // Used for testing.
    if (metadata.empty()) {
      SetAccessibilityLabelFromRecentSeaPenImageInfo(nullptr);
      return;
    }

    const std::string extracted_metadata =
        ExtractDcDescriptionContents(metadata);

    DecodeJsonMetadata(
        extracted_metadata.empty() ? metadata : extracted_metadata,
        base::BindOnce(&RecentlyUsedImageButton::
                           SetAccessibilityLabelFromRecentSeaPenImageInfo,
                       weak_factory_.GetWeakPtr()));
  }

  // Called when decoding metadata complete.
  void SetAccessibilityLabelFromRecentSeaPenImageInfo(
      personalization_app::mojom::RecentSeaPenImageInfoPtr info) {
    SetAccessibleRole(ax::mojom::Role::kListItem);
    SetAccessibleDescription(l10n_util::GetStringUTF16(
        IDS_ASH_VIDEO_CONFERENCE_BUBBLE_BACKGROUND_BLUR_IMAGE_LIST_ITEM_DESCRIPTION));

    std::u16string query;
    if (!info.is_null() && !info->user_visible_query.is_null()) {
      const auto& text = info->user_visible_query->text;
      if (!base::UTF8ToUTF16(text.c_str(), text.size(), &query)) {
        query.clear();
      }
    }
    SetAccessibleName(
        query, query.empty() ? ax::mojom::NameFrom::kAttributeExplicitlyEmpty
                             : ax::mojom::NameFrom::kAttribute);
  }

  SkPath GetClipPath() {
    const auto width = this->width();
    const auto height = this->height();
    const auto radius = kSetCameraBackgroundViewRadius;

    return SkPathBuilder()
        // Start just before the curve of the top-right corner.
        .moveTo(width - radius, 0.f)
        // Move to left before the curve.
        .lineTo(38, 0)
        // Draw first part of the top-left corner.
        .rCubicTo(-5.52f, 0, -10, 4.48f, -10, 10)
        // Move down a bit.
        .rLineTo(-0.f, 2.f)
        // Draw second part of the top-left corner.
        .rCubicTo(0, 8.84f, -7.16f, 16, -16, 16)
        // Move left a bit.
        .rLineTo(-2.f, 0.f)
        // Draw the third part of the top-left corner.
        .cubicTo(4.48f, 28, 0, 32.48f, 0, 38)
        // Move to the bottom-left corner.
        .lineTo(-0.f, height - radius)
        // Draw bottom-left curve.
        .rCubicTo(0, 8.84f, 7.16f, 16, 16, 16)
        // Move to the bottom-right corner.
        .lineTo(width - radius, height)
        // Draw bottom-right curve.
        .rCubicTo(8.84f, 0, 16, -7.16f, 16, -16)
        // Move to the top-right corner.
        .lineTo(width, 16)
        // Draw top-right curve.
        .rCubicTo(0, -8.84f, -7.16f, -16, -16, -16)
        .close()
        .detach();
  }

  bool selected_ = false;
  const ui::ThemedVectorIcon check_icon_;

  base::WeakPtrFactory<RecentlyUsedImageButton> weak_factory_{this};
};

BEGIN_METADATA(RecentlyUsedImageButton)
END_METADATA

// The RecentlyUsedBackgroundView contains a list of recently used background
// images as buttons.
class RecentlyUsedBackgroundView : public views::View {
  METADATA_HEADER(RecentlyUsedBackgroundView, views::View)

 public:
  explicit RecentlyUsedBackgroundView(BubbleView* bubble_view)
      : bubble_view_(bubble_view) {
    auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kHorizontal,
        /*inside_border_insets=*/gfx::Insets(),
        /*between_child_spacing=*/kRecentlyUsedImagesSpacing));
    layout->set_cross_axis_alignment(
        views::BoxLayout::CrossAxisAlignment::kStretch);

    GetCameraEffectsController()->GetRecentlyUsedBackgroundImages(
        kMaxRecentBackgroundToDislay,
        base::BindOnce(&RecentlyUsedBackgroundView::
                           GetRecentlyUsedBackgroundImagesComplete,
                       weak_factory_.GetWeakPtr()));
  }

  void GetRecentlyUsedBackgroundImagesComplete(
      const std::vector<BackgroundImageInfo>& images_info) {
    for (std::size_t i = 0; i < images_info.size(); ++i) {
      auto image = image_util::ResizeAndCropImage(
          images_info[i].image,
          CalculateWantedImageSize(i, images_info.size()));

      image = gfx::ImageSkiaOperations::CreateImageWithRoundRectClip(
          kSetCameraBackgroundViewRadius, image);

      auto recently_used_image_button =
          std::make_unique<RecentlyUsedImageButton>(
              image, images_info[i].metadata, kRecentlyUsedImageButtonId[i],
              base::BindRepeating(
                  &RecentlyUsedBackgroundView::OnImageButtonClicked,
                  weak_factory_.GetWeakPtr(), i, images_info[i].basename));
      // If background replace is applied, then set first image as selected.
      if (i == 0 &&
          GetCameraEffectsController()->GetCameraEffects()->replace_enabled) {
        recently_used_image_button->SetSelected(true);
      }
      AddChildView(std::move(recently_used_image_button));
    }

    // Because this is async, we need to update the ui when all images are
    // loaded.
    if (bubble_view_) {
      bubble_view_->ChildPreferredSizeChanged(this);
    }
  }

  // Called when index-th image button is clicked on.
  void OnImageButtonClicked(std::size_t index, const base::FilePath& filename) {
    // Select index-th image button and deselect the rest of the image buttons.
    for (std::size_t i = 0; i < children().size(); i++) {
      RecentlyUsedImageButton* button =
          views::AsViewClass<RecentlyUsedImageButton>(children()[i]);
      button->SetSelected(i == index);
    }

    GetCameraEffectsController()->SetBackgroundImage(filename,
                                                     base::DoNothing());

    base::UmaHistogramBoolean(kBackgroundImageButtonHistogramName, true);
  }

 private:
  raw_ptr<BubbleView> bubble_view_;

  base::WeakPtrFactory<RecentlyUsedBackgroundView> weak_factory_{this};
};

BEGIN_METADATA(RecentlyUsedBackgroundView)
END_METADATA

// Button for "Create with AI".
class CreateImageButton : public views::Button {
  METADATA_HEADER(CreateImageButton, views::Button)

 public:
  explicit CreateImageButton(VideoConferenceTrayController* controller)
      : views::Button(base::BindRepeating(&CreateImageButton::OnButtonClicked,
                                          base::Unretained(this))),
        controller_(controller) {
    // TODO(b/334205690): Use view builder pattern.
    SetID(BubbleViewID::kCreateWithAiButton);
    SetAccessibleName(
        l10n_util::GetStringUTF16(IDS_ASH_VIDEO_CONFERENCE_CREAT_WITH_AI_NAME));
    SetLayoutManager(std::make_unique<views::FillLayout>());
    SetBackground(views::CreateThemedRoundedRectBackground(
        cros_tokens::kCrosSysSystemOnBase, kSetCameraBackgroundViewRadius));

    lottie_animation_view_ =
        AddChildView(std::make_unique<views::AnimatedImageView>());

    auto* image_label_view_container =
        AddChildView(std::make_unique<views::BoxLayoutView>());
    image_label_view_container->SetBetweenChildSpacing(
        kCreateImageButtonBetweenChildSpacing);
    image_label_view_container->SetOrientation(
        views::BoxLayout::Orientation::kHorizontal);
    image_label_view_container->SetMainAxisAlignment(
        views::LayoutAlignment::kCenter);
    image_label_view_container->SetInsideBorderInsets(
        kImageLabelContainerInsideBorderInsets);
    image_label_view_container->SetMainAxisAlignment(
        views::LayoutAlignment::kCenter);

    image_label_view_container->AddChildView(
        std::make_unique<views::ImageView>(ui::ImageModel::FromVectorIcon(
            kAiWandIcon, ui::kColorMenuIcon, kButtonHeight)));
    image_label_view_container->AddChildView(
        std::make_unique<views::Label>(l10n_util::GetStringUTF16(
            IDS_ASH_VIDEO_CONFERENCE_CREAT_WITH_AI_NAME)));
  }

  CreateImageButton(const CreateImageButton&) = delete;
  CreateImageButton& operator=(const CreateImageButton&) = delete;
  ~CreateImageButton() override = default;

 private:
  // views::Button:
  // Reset the animated image on theme changed to get correct color for the
  // animation if the `lottie_animation_view_` should be shown and is visible.
  void OnThemeChanged() override {
    views::Button::OnThemeChanged();

    // Don't need to reset the animated image when animation shouldn't be shown
    // or `lottie_animation_view_` is invisible, or this button's bounds is
    // empty.
    // TODO(b/334205691): Set visibility correctly to make the check for bounds
    // no longer needed.
    if (!controller_->ShouldShowCreateWithAiButtonAnimation() ||
        !lottie_animation_view_->GetVisible() ||
        GetBoundsInScreen().IsEmpty()) {
      return;
    }

    lottie_animation_view_->SetAnimatedImage(
        GetGradientAnimation(GetColorProvider()));
    lottie_animation_view_->Play();
  }

  // CreateImageButton could be not laid out if there's no recently used images.
  // We don't want to play the animation if the button is not shown yet.
  void AddedToWidget() override {
    // TODO(b/334205691): Set visibility correctly to make the check for bounds
    // no longer needed.
    if (!controller_->ShouldShowCreateWithAiButtonAnimation() ||
        GetBoundsInScreen().IsEmpty()) {
      lottie_animation_view_->SetVisible(false);
      if (lottie_animation_view_->animated_image()) {
        lottie_animation_view_->Stop();
      }
      return;
    }

    PlayAnimation();
  }

  // Used to indicate when the button is laid out and shown.
  // TODO(b/334205691): Set visibility correctly. Change this function to
  // VisibilityChanged();
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override {
    UpdateAnimationViewVisibility();
  }

  void UpdateAnimationViewVisibility() {
    // TODO(b/334205691): Set visibility correctly to make the check for bounds
    // no longer needed.
    if (!is_first_time_animation_ ||
        !controller_->ShouldShowCreateWithAiButtonAnimation() ||
        GetBoundsInScreen().IsEmpty()) {
      lottie_animation_view_->SetVisible(false);
      if (lottie_animation_view_->animated_image()) {
        lottie_animation_view_->Stop();
      }
      return;
    }

    PlayAnimation();
  }

  void OnButtonClicked(const ui::Event& event) {
    HideAnimationView();
    controller_->CreateBackgroundImage();
    controller_->DismissCreateWithAiButtonAnimationForever();

    base::UmaHistogramBoolean(kCreateWithAiButtonHistogramName, true);
  }

  void PlayAnimation() {
    if (!lottie_animation_view_->animated_image()) {
      lottie_animation_view_->SetAnimatedImage(
          GetGradientAnimation(GetColorProvider()));
    }
    lottie_animation_view_->SetVisible(true);
    lottie_animation_view_->Play();
    stop_animation_timer_.Start(FROM_HERE, kGradientAnimationDuration, this,
                                &CreateImageButton::StopAnimation);
  }

  void StopAnimation() {
    is_first_time_animation_ = false;
    stop_animation_timer_.Stop();
    lottie_animation_view_->Stop();
    lottie_animation_view_->SetVisible(false);
  }

  void HideAnimationView() {
    if (!lottie_animation_view_->GetVisible()) {
      return;
    }
    stop_animation_timer_.Stop();
    lottie_animation_view_->Stop();
    lottie_animation_view_->SetVisible(false);
  }

  // Unowned by `CreateImageButton`.
  const raw_ptr<VideoConferenceTrayController> controller_;

  // Owned by the View's hierarchy. Used to play the animation on the button.
  raw_ptr<views::AnimatedImageView> lottie_animation_view_ = nullptr;

  // It's set false when the animation has been played during the lifetime of
  // `this`. When it's false, we shouldn't play animation animation anymore.
  bool is_first_time_animation_ = true;

  // Started when `lottie_animation_view_` starts playing the animation. It's
  // used to stop the animation after the animation duration.
  base::OneShotTimer stop_animation_timer_;
};

BEGIN_METADATA(CreateImageButton)
END_METADATA

}  // namespace

SetCameraBackgroundView::SetCameraBackgroundView(
    BubbleView* bubble_view,
    VideoConferenceTrayController* controller)
    : controller_(controller) {
  SetID(BubbleViewID::kSetCameraBackgroundView);

  // `SetCameraBackgroundView` has 2+ children, we want to stack them
  // vertically.
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      /*inside_border_insets=*/kSetCameraBackgroundViewInsideBorderInsets,
      /*between_child_spacing=*/kSetCameraBackgroundViewBetweenChildSpacing));
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  recently_used_background_view_ =
      AddChildView(std::make_unique<RecentlyUsedBackgroundView>(bubble_view));
  AddChildView(std::make_unique<CreateImageButton>(controller));
}

void SetCameraBackgroundView::SetBackgroundReplaceUiVisible(bool visible) {
  // We don't want to show the SetCameraBackgroundView if there is no recently
  // used background; instead, the webui is shown.
  if (visible && recently_used_background_view_->children().empty()) {
    controller_->CreateBackgroundImage();
    return;
  }

  SetVisible(visible);

  // Unselect all recently image buttons if this view is invisible.
  if (!visible) {
    for (auto& button : recently_used_background_view_->children()) {
      views::AsViewClass<RecentlyUsedImageButton>(button)->SetSelected(false);
    }
  }
}

BEGIN_METADATA(SetCameraBackgroundView)
END_METADATA

}  // namespace ash::video_conference
