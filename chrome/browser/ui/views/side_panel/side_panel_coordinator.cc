// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"

#include <memory>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/strcat.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/feature_engagement/tracker_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/side_panel/companion/companion_utils.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_key.h"
#include "chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_model.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/extensions/extensions_toolbar_container.h"
#include "chrome/browser/ui/views/frame/browser_actions.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/search_companion/search_companion_side_panel_coordinator.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/browser/ui/views/side_panel/side_panel_combobox_model.h"
#include "chrome/browser/ui/views/side_panel/side_panel_content_proxy.h"
#include "chrome/browser/ui/views/side_panel/side_panel_header.h"
#include "chrome/browser/ui/views/side_panel/side_panel_toolbar_container.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/browser/ui/views/toolbar/pinned_toolbar_actions_container.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/lens/lens_features.h"
#include "components/strings/grit/components_strings.h"
#include "ui/actions/action_id.h"
#include "ui/base/interaction/element_tracker.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/combobox_model.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/window_open_disposition.h"
#include "ui/color/color_id.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/gfx/vector_icon_utils.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/separator.h"
#include "ui/views/interaction/element_tracker_views.h"
#include "ui/views/layout/flex_layout_view.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/vector_icons.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"

namespace {

const char kGlobalSidePanelRegistryKey[] = "global_side_panel_registry_key";

constexpr int kSidePanelContentContainerViewId = 42;
constexpr int kSidePanelContentWrapperViewId = 43;

SidePanelEntry::Id GetDefaultEntry() {
  return SidePanelEntry::Id::kBookmarks;
}

void ConfigureControlButton(views::ImageButton* button) {
  button->SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  views::InstallCircleHighlightPathGenerator(button);

  int minimum_button_size = ChromeLayoutProvider::Get()->GetDistanceMetric(
      ChromeDistanceMetric::DISTANCE_SIDE_PANEL_HEADER_BUTTON_MINIMUM_SIZE);
  button->SetMinimumImageSize(
      gfx::Size(minimum_button_size, minimum_button_size));

  if (features::IsSidePanelPinningEnabled()) {
    button->SetProperty(
        views::kMarginsKey,
        gfx::Insets().set_left(ChromeLayoutProvider::Get()->GetDistanceMetric(
            ChromeDistanceMetric::
                DISTANCE_SIDE_PANEL_HEADER_INTERIOR_MARGIN_HORIZONTAL)));

  } else {
    button->SetProperty(
        views::kMarginsKey,
        gfx::Insets().set_left(ChromeLayoutProvider::Get()->GetDistanceMetric(
            views::DistanceMetric::DISTANCE_RELATED_BUTTON_HORIZONTAL)));
  }
  button->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification().WithAlignment(views::LayoutAlignment::kEnd));
}

std::unique_ptr<views::ToggleImageButton> CreatePinToggleButton(
    base::RepeatingClosure pressed_callback) {
  auto button =
      std::make_unique<views::ToggleImageButton>(std::move(pressed_callback));
  views::ConfigureVectorImageButton(button.get());
  ConfigureControlButton(button.get());
  button->SetTooltipText(
      l10n_util::GetStringUTF16(IDS_SIDE_PANEL_HEADER_PIN_BUTTON_TOOLTIP));
  button->SetToggledTooltipText(
      l10n_util::GetStringUTF16(IDS_SIDE_PANEL_HEADER_UNPIN_BUTTON_TOOLTIP));

  int dip_size = ChromeLayoutProvider::Get()->GetDistanceMetric(
      ChromeDistanceMetric::DISTANCE_SIDE_PANEL_HEADER_VECTOR_ICON_SIZE);
  const gfx::VectorIcon& pin_icon = features::IsChromeRefresh2023()
                                        ? kKeepPinChromeRefreshIcon
                                        : views::kPinIcon;
  const gfx::VectorIcon& unpin_icon = features::IsChromeRefresh2023()
                                          ? kKeepPinFilledChromeRefreshIcon
                                          : views::kUnpinIcon;
  views::SetImageFromVectorIconWithColorId(
      button.get(), pin_icon, kColorSidePanelHeaderButtonIcon,
      kColorSidePanelHeaderButtonIconDisabled, dip_size);
  const ui::ImageModel& normal_image = ui::ImageModel::FromVectorIcon(
      unpin_icon, kColorSidePanelHeaderButtonIcon, dip_size);
  const ui::ImageModel& disabled_image = ui::ImageModel::FromVectorIcon(
      unpin_icon, kColorSidePanelHeaderButtonIconDisabled, dip_size);
  button->SetToggledImageModel(views::Button::STATE_NORMAL, normal_image);
  button->SetToggledImageModel(views::Button::STATE_DISABLED, disabled_image);
  button->SetProperty(views::kElementIdentifierKey,
                      kSidePanelPinButtonElementId);
  return button;
}

std::unique_ptr<views::ImageButton> CreateControlButton(
    views::View* host,
    base::RepeatingClosure pressed_callback,
    const gfx::VectorIcon& icon,
    const std::u16string& tooltip_text,
    ui::ElementIdentifier view_id,
    int dip_size) {
  auto button = views::CreateVectorImageButtonWithNativeTheme(
      pressed_callback, icon, dip_size, kColorSidePanelHeaderButtonIcon,
      kColorSidePanelHeaderButtonIconDisabled);
  button->SetTooltipText(tooltip_text);
  ConfigureControlButton(button.get());
  button->SetProperty(views::kElementIdentifierKey, view_id);

  return button;
}

std::unique_ptr<views::ImageView> CreateIcon() {
  std::unique_ptr<views::ImageView> icon = std::make_unique<views::ImageView>();
  const int horizontal_margin =
      ChromeLayoutProvider::Get()->GetDistanceMetric(
          ChromeDistanceMetric::
              DISTANCE_SIDE_PANEL_HEADER_INTERIOR_MARGIN_HORIZONTAL) *
      2;
  icon->SetProperty(views::kMarginsKey,
                    gfx::Insets().set_left(horizontal_margin));
  return icon;
}

std::unique_ptr<views::Label> CreateTitle() {
  std::unique_ptr<views::Label> title = std::make_unique<views::Label>(
      std::u16string(), views::style::CONTEXT_LABEL,
      views::style::STYLE_HEADLINE_5);

  title->SetEnabledColorId(kColorSidePanelEntryTitle);
  title->SetSubpixelRenderingEnabled(false);
  const int horizontal_margin =
      ChromeLayoutProvider::Get()->GetDistanceMetric(
          ChromeDistanceMetric::
              DISTANCE_SIDE_PANEL_HEADER_INTERIOR_MARGIN_HORIZONTAL) *
      2;
  title->SetProperty(views::kMarginsKey,
                     gfx::Insets().set_left(horizontal_margin));
  title->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                               views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded)
          .WithAlignment(views::LayoutAlignment::kStart));
  return title;
}

using PopulateSidePanelCallback = base::OnceCallback<void(
    SidePanelEntry* entry,
    std::optional<std::unique_ptr<views::View>> content_view)>;

// SidePanelContentSwappingContainer is used as the content wrapper for views
// hosted in the side panel. This uses the SidePanelContentProxy to check if or
// wait for a SidePanelEntry's content view to be ready to be shown then only
// swaps the views when the content is ready. If the SidePanelContextProxy
// doesn't exist, the content is swapped immediately.
class SidePanelContentSwappingContainer : public views::View {
  METADATA_HEADER(SidePanelContentSwappingContainer, views::View)

 public:
  explicit SidePanelContentSwappingContainer(bool show_immediately_for_testing)
      : show_immediately_for_testing_(show_immediately_for_testing) {
    SetUseDefaultFillLayout(true);
    SetBackground(
        views::CreateThemedSolidBackground(kColorSidePanelBackground));
    SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                                 views::MaximumFlexSizeRule::kUnbounded));
    SetID(kSidePanelContentWrapperViewId);
  }

  ~SidePanelContentSwappingContainer() override {
    ResetLoadingEntryIfNecessary();
  }

  void RequestEntry(SidePanelEntry* entry, PopulateSidePanelCallback callback) {
    DCHECK(entry);
    ResetLoadingEntryIfNecessary();
    auto content_view = entry->GetContent();
    SidePanelContentProxy* content_proxy =
        SidePanelUtil::GetSidePanelContentProxy(content_view.get());
    if (content_proxy->IsAvailable() || show_immediately_for_testing_) {
      std::move(callback).Run(entry, std::move(content_view));
    } else {
      entry->CacheView(std::move(content_view));
      loading_entry_ = entry;
      loaded_callback_ = std::move(callback);
      content_proxy->SetAvailableCallback(
          base::BindOnce(&SidePanelContentSwappingContainer::RunLoadedCallback,
                         base::Unretained(this)));
    }
  }

  void ResetLoadingEntryIfNecessary() {
    if (loading_entry_) {
      loading_entry_->ResetLoadTimestamp();
      if (loading_entry_->CachedView()) {
        // The available callback here is used for showing the entry once it has
        // loaded. We need to reset this to make sure it is not triggered to be
        // shown once available.
        SidePanelUtil::GetSidePanelContentProxy(loading_entry_->CachedView())
            ->ResetAvailableCallback();
      }
    }
    loading_entry_ = nullptr;
  }

  void SetNoDelaysForTesting(bool no_delays_for_testing) {
    show_immediately_for_testing_ = no_delays_for_testing;
  }

  SidePanelEntry* loading_entry() const { return loading_entry_; }

 private:
  void RunLoadedCallback() {
    DCHECK(!loaded_callback_.is_null());
    SidePanelEntry* entry = loading_entry_;
    loading_entry_ = nullptr;
    std::move(loaded_callback_).Run(entry, std::nullopt);
  }

  // When true, don't delay switching panels.
  bool show_immediately_for_testing_;
  // If the SidePanelEntry is ever discarded by the SidePanelCoordinator then we
  // are always either immediately switching to a different entry (where this
  // value would be reset) or closing the side panel (where this would be
  // destroyed).
  raw_ptr<SidePanelEntry> loading_entry_ = nullptr;
  PopulateSidePanelCallback loaded_callback_;
};

BEGIN_METADATA(SidePanelContentSwappingContainer)
END_METADATA

}  // namespace

SidePanelCoordinator::SidePanelCoordinator(BrowserView* browser_view)
    : browser_view_(browser_view) {
  if (!features::IsSidePanelPinningEnabled()) {
    combobox_model_ = std::make_unique<SidePanelComboboxModel>(browser_view_);
  } else {
    pinned_model_observation_.Observe(
        PinnedToolbarActionsModel::Get(browser_view_->GetProfile()));
    // When the SidePanelPinning feature is enabled observe changes to the
    // pinned actions so we can update the pin button appropriately.
    // TODO(b/310910098): Observe the PinnedToolbarActionModel instead when
    // pinned extensions are fully merged into it.
    extensions_model_observation_.Observe(
        ToolbarActionsModel::Get(browser_view_->browser()->profile()));
  }

  auto global_registry = std::make_unique<SidePanelRegistry>();
  global_registry_ = global_registry.get();
  registry_observations_.AddObservation(global_registry_);
  browser_view->browser()->SetUserData(kGlobalSidePanelRegistryKey,
                                       std::move(global_registry));

  browser_view_->browser()->tab_strip_model()->AddObserver(this);

  SidePanelUtil::PopulateGlobalEntries(browser_view->browser(),
                                       global_registry_);
  if (features::IsChromeRefresh2023()) {
    browser_view_->unified_side_panel()->AddHeaderView(CreateHeader());
  }

  if (features::IsSidePanelPinningEnabled()) {
    browser_view_->MaybeShowStartupFeaturePromo(
        feature_engagement::kIPHSidePanelGenericMenuFeature);
  }
}

SidePanelCoordinator::~SidePanelCoordinator() {
  browser_view_->browser()->tab_strip_model()->RemoveObserver(this);
  view_state_observers_.Clear();
}

// static
SidePanelRegistry* SidePanelCoordinator::GetGlobalSidePanelRegistry(
    Browser* browser) {
  return static_cast<SidePanelRegistry*>(
      browser->GetUserData(kGlobalSidePanelRegistryKey));
}

void SidePanelCoordinator::OnToolbarPinnedActionsChanged() {
  UpdateHeaderPinButtonState();
}

actions::ActionItem* SidePanelCoordinator::GetActionItem(
    SidePanelEntry::Key entry_key) {
  BrowserActions* const browser_actions =
      BrowserActions::FromBrowser(browser_view_->browser());
  if (entry_key.id() == SidePanelEntryId::kExtension) {
    std::optional<actions::ActionId> extension_action_id =
        actions::ActionIdMap::StringToActionId(entry_key.ToString());
    CHECK(extension_action_id.has_value());
    actions::ActionItem* const action_item =
        actions::ActionManager::Get().FindAction(
            extension_action_id.value(), browser_actions->root_action_item());
    CHECK(action_item);
    return action_item;
  }

  std::optional<actions::ActionId> action_id =
      SidePanelEntryIdToActionId(entry_key.id());
  CHECK(action_id.has_value());
  return actions::ActionManager::Get().FindAction(
      action_id.value(), browser_actions->root_action_item());
}

void SidePanelCoordinator::Show(
    std::optional<SidePanelEntry::Id> entry_id,
    std::optional<SidePanelUtil::SidePanelOpenTrigger> open_trigger) {
  if (entry_id.has_value()) {
    Show(SidePanelEntry::Key(entry_id.value()), open_trigger);
  } else {
    Show(GetLastActiveEntryKey().value_or(
             SidePanelEntry::Key(GetDefaultEntry())),
         open_trigger);
  }
}

void SidePanelCoordinator::Show(
    SidePanelEntry::Key entry_key,
    std::optional<SidePanelUtil::SidePanelOpenTrigger> open_trigger) {
  Show(GetEntryForKey(entry_key), open_trigger);
}

void SidePanelCoordinator::AddSidePanelViewStateObserver(
    SidePanelViewStateObserver* observer) {
  view_state_observers_.AddObserver(observer);
}

void SidePanelCoordinator::RemoveSidePanelViewStateObserver(
    SidePanelViewStateObserver* observer) {
  view_state_observers_.RemoveObserver(observer);
}

void SidePanelCoordinator::SetSidePanelButtonTooltipText(
    std::u16string tooltip_text) {
  auto* toolbar = browser_view_->toolbar();
  // On Progressive web apps, the toolbar can be null when opening the side
  // panel. This check is added as a added safeguard.
  if (toolbar && toolbar->GetSidePanelButton()) {
    toolbar->GetSidePanelButton()->SetTooltipText(tooltip_text);
  }
}

void SidePanelCoordinator::Close() {
  if (!IsSidePanelShowing() ||
      browser_view_->unified_side_panel()->IsClosing()) {
    return;
  }

  if (current_entry_ &&
      browser_view_->toolbar()->pinned_toolbar_actions_container()) {
    NotifyPinnedContainerOfActiveStateChange(current_entry_->key(), false);
  }
  if (views::View* content_container = GetContentContainerView()) {
    content_container->SetProperty(
        kSidePanelContentStateKey,
        static_cast<std::underlying_type_t<SidePanelContentState>>(
            SidePanelContentState::kReadyToHide));
  }
}

void SidePanelCoordinator::Toggle() {
  if (IsSidePanelShowing() &&
      !browser_view_->unified_side_panel()->IsClosing()) {
    Close();
  } else {
    std::optional<SidePanelEntry::Id> entry_id = std::nullopt;
    if (browser_view_->browser()->window()->IsFeaturePromoActive(
            feature_engagement::kIPHPowerBookmarksSidePanelFeature)) {
      entry_id = std::make_optional(SidePanelEntry::Id::kBookmarks);
    }
    Show(entry_id, SidePanelUtil::SidePanelOpenTrigger::kToolbarButton);
  }
}

void SidePanelCoordinator::Toggle(
    SidePanelEntryKey key,
    SidePanelUtil::SidePanelOpenTrigger open_trigger) {
  // If an entry is already showing in the sidepanel, the sidepanel
  // should be closed.
  if (IsSidePanelEntryShowing(key) &&
      !browser_view_->unified_side_panel()->IsClosing()) {
    Close();
    return;
  }

  // If the entry is the loading entry and is toggled,
  // it should also be closed. This handles quick double clicks
  // to close the sidepanel.
  if (IsSidePanelShowing()) {
    SidePanelContentSwappingContainer* content_wrapper =
        static_cast<SidePanelContentSwappingContainer*>(
            GetContentContainerView()->GetViewByID(
                kSidePanelContentWrapperViewId));

    if (content_wrapper->loading_entry() == GetEntryForKey(key)) {
      content_wrapper->ResetLoadingEntryIfNecessary();
      Close();
      return;
    }
  }

  Show(key, open_trigger);
}

void SidePanelCoordinator::OpenInNewTab() {
  if (!GetContentContainerView() || !current_entry_) {
    return;
  }

  GURL new_tab_url = current_entry_->GetOpenInNewTabURL();
  if (!new_tab_url.is_valid())
    return;

  SidePanelUtil::RecordNewTabButtonClicked(current_entry_->key().id());
  content::OpenURLParams params(new_tab_url, content::Referrer(),
                                WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                ui::PAGE_TRANSITION_AUTO_BOOKMARK,
                                /*is_renderer_initiated=*/false);
  browser_view_->browser()->OpenURL(params, /*navigation_handle_callback=*/{});
  Close();
}

void SidePanelCoordinator::UpdatePinState() {
  Profile* const profile = browser_view_->GetProfile();
  if (!features::IsSidePanelPinningEnabled()) {
    PrefService* const pref_service = profile->GetPrefs();
    if (pref_service) {
      const bool current_state = pref_service->GetBoolean(
          prefs::kSidePanelCompanionEntryPinnedToToolbar);
      pref_service->SetBoolean(prefs::kSidePanelCompanionEntryPinnedToToolbar,
                               !current_state);
      SearchCompanionSidePanelCoordinator::SetAccessibleNameForToolbarButton(
          browser_view_, /*is_open=*/true);
      base::RecordComputedAction(base::StrCat(
          {"SidePanel.Companion.", !current_state ? "Pinned" : "Unpinned",
           ".BySidePanelHeaderButton"}));
    }

    return;
  }

  // Signal that the user has used the Pin feature.
  browser_view_->NotifyFeatureEngagementEvent(
      feature_engagement::events::kSidePanelPinned);

  // Close IPH for side panel pinning, if shown.
  browser_view_->CloseFeaturePromo(
      feature_engagement::kIPHSidePanelGenericPinnableFeature,
      user_education::EndFeaturePromoReason::kFeatureEngaged);

  std::optional<actions::ActionId> action_id =
      GetActionItem(current_entry_->key())->GetActionId();
  CHECK(action_id.has_value());

  bool updated_pin_state = false;

  // TODO(b/310910098): Clean condition up once/if ToolbarActionModel and
  // PinnedToolbarActionModel are merged together.
  if (const std::optional<extensions::ExtensionId> extension_id =
          current_entry_->key().extension_id();
      extension_id.has_value()) {
    ToolbarActionsModel* const actions_model =
        ToolbarActionsModel::Get(profile);

    updated_pin_state = !actions_model->IsActionPinned(*extension_id);
    actions_model->SetActionVisibility(*extension_id, updated_pin_state);
  } else {
    PinnedToolbarActionsModel* const actions_model =
        PinnedToolbarActionsModel::Get(profile);

    updated_pin_state = !actions_model->Contains(action_id.value());
    actions_model->UpdatePinnedState(action_id.value(), updated_pin_state);
  }

  SidePanelUtil::RecordPinnedButtonClicked(current_entry_->key().id(),
                                           updated_pin_state);
  header_pin_button_->GetViewAccessibility().AnnounceText(
      l10n_util::GetStringUTF16(updated_pin_state ? IDS_SIDE_PANEL_PINNED
                                                  : IDS_SIDE_PANEL_UNPINNED));
}

std::optional<SidePanelEntry::Id> SidePanelCoordinator::GetCurrentEntryId()
    const {
  return current_entry_
             ? std::optional<SidePanelEntry::Id>(current_entry_->key().id())
             : std::nullopt;
}

bool SidePanelCoordinator::IsSidePanelShowing() const {
  if (auto* side_panel = browser_view_->unified_side_panel()) {
    return side_panel->GetVisible();
  }
  return false;
}

bool SidePanelCoordinator::IsSidePanelEntryShowing(
    const SidePanelEntry::Key& entry_key) const {
  return IsSidePanelShowing() && current_entry_ &&
         current_entry_->key() == entry_key;
}

content::WebContents* SidePanelCoordinator::GetWebContentsForTest(
    SidePanelEntryId id) {
  if (auto* entry = GetEntryForKey(SidePanelEntryKey(id))) {
    entry->CacheView(entry->GetContent());
    if (entry->CachedView()) {
      if (auto* view = entry->CachedView()->GetViewByID(
              SidePanelWebUIView::kSidePanelWebViewId)) {
        return (static_cast<views::WebView*>(view))->web_contents();
      }
    }
  }
  return nullptr;
}

void SidePanelCoordinator::DisableAnimationsForTesting() {
  browser_view_->unified_side_panel()
      ->DisableAnimationsForTesting();  // IN-TEST
}

SidePanelEntry::Id SidePanelCoordinator::GetComboboxDisplayedEntryIdForTesting()
    const {
  CHECK(combobox_model_);
  return combobox_model_->GetKeyAt(header_combobox_->GetSelectedIndex().value())
      .id();
}

SidePanelEntry* SidePanelCoordinator::GetLoadingEntryForTesting() const {
  return GetLoadingEntry();
}

bool SidePanelCoordinator::IsSidePanelEntryShowing(
    const SidePanelEntry* entry) const {
  return IsSidePanelShowing() && current_entry_ &&
         current_entry_.get() == entry;
}

void SidePanelCoordinator::Show(
    SidePanelEntry* entry,
    std::optional<SidePanelUtil::SidePanelOpenTrigger> open_trigger) {
  // Side panel is not supported for non-normal browsers.
  if (!browser_view_->browser()->is_type_normal()) {
    return;
  }

  if (!entry) {
    return;
  }

  if (GetContentContainerView() == nullptr) {
    CHECK(browser_view_->unified_side_panel());
    InitializeSidePanel();
  }

  if (!IsSidePanelShowing()) {
    opened_timestamp_ = base::TimeTicks::Now();
    SidePanelUtil::RecordSidePanelOpen(open_trigger);
    // Record usage for side panel promo.
    feature_engagement::TrackerFactory::GetForBrowserContext(
        browser_view_->GetProfile())
        ->NotifyEvent("side_panel_shown");

    // Close IPH for side panel if shown.
    browser_view_->browser()->window()->CloseFeaturePromo(
        feature_engagement::kIPHReadingListInSidePanelFeature);
    browser_view_->browser()->window()->CloseFeaturePromo(
        feature_engagement::kIPHPowerBookmarksSidePanelFeature);
    browser_view_->browser()->window()->CloseFeaturePromo(
        feature_engagement::kIPHReadingModeSidePanelFeature);

    if (features::IsSidePanelPinningEnabled()) {
      // Close IPH for side panel menu, if shown.
      browser_view_->browser()->window()->CloseFeaturePromo(
          feature_engagement::kIPHSidePanelGenericMenuFeature);
    }
  }

  SidePanelUtil::RecordSidePanelShowOrChangeEntryTrigger(open_trigger);

  SidePanelContentSwappingContainer* content_wrapper =
      static_cast<SidePanelContentSwappingContainer*>(
          GetContentContainerView()->GetViewByID(
              kSidePanelContentWrapperViewId));
  DCHECK(content_wrapper);

  // If we are already loading this entry, do nothing.
  if (content_wrapper->loading_entry() == entry) {
    return;
  }

  // If we are already showing this entry, make sure we prevent any loading
  // entry from showing once the load has finished. Say if we are showing A then
  // trigger B to show but switch back to A while B is still loading (and not
  // yet shown) we want to make sure B will not then be shown when it has
  // finished loading. Note, this does not cancel the triggered load of B, B
  // remains cached.
  if (current_entry_.get() == entry) {
    GetContentContainerView()->SetProperty(
        kSidePanelContentStateKey,
        static_cast<std::underlying_type_t<SidePanelContentState>>(
            SidePanelContentState::kReadyToShow));
    if (content_wrapper->loading_entry()) {
      content_wrapper->ResetLoadingEntryIfNecessary();
    }
    if (browser_view_->toolbar()->pinned_toolbar_actions_container()) {
      NotifyPinnedContainerOfActiveStateChange(entry->key(), true);
    }
    return;
  }

  SidePanelUtil::RecordEntryShowTriggeredMetrics(
      browser_view_->browser(), entry->key().id(), open_trigger);

  content_wrapper->RequestEntry(
      entry, base::BindOnce(&SidePanelCoordinator::PopulateSidePanel,
                            base::Unretained(this)));
}

views::View* SidePanelCoordinator::GetContentContainerView() const {
  if (SidePanel* side_panel = browser_view_->unified_side_panel()) {
    return side_panel->GetViewByID(kSidePanelContentContainerViewId);
  }

  return nullptr;
}

SidePanelEntry* SidePanelCoordinator::GetEntryForKey(
    const SidePanelEntry::Key& entry_key) const {
  if (auto* contextual_entry = GetActiveContextualEntryForKey(entry_key)) {
    return contextual_entry;
  }

  return global_registry_->GetEntryForKey(entry_key);
}

SidePanelEntry* SidePanelCoordinator::GetActiveContextualEntryForKey(
    const SidePanelEntry::Key& entry_key) const {
  return GetActiveContextualRegistry()
             ? GetActiveContextualRegistry()->GetEntryForKey(entry_key)
             : nullptr;
}

SidePanelEntry* SidePanelCoordinator::GetLoadingEntry() const {
  auto* content_container = GetContentContainerView();
  DCHECK(content_container);

  SidePanelContentSwappingContainer* content_wrapper =
      static_cast<SidePanelContentSwappingContainer*>(
          content_container->GetViewByID(kSidePanelContentWrapperViewId));
  DCHECK(content_wrapper);
  return content_wrapper->loading_entry();
}

bool SidePanelCoordinator::IsGlobalEntryShowing(
    const SidePanelEntry::Key& entry_key) const {
  if (!IsSidePanelShowing() || !current_entry_) {
    return false;
  }

  return global_registry_->GetEntryForKey(entry_key) == current_entry_.get();
}

void SidePanelCoordinator::InitializeSidePanel() {
  // TODO(pbos): Make this button observe panel-visibility state instead.
  if (!companion::IsCompanionFeatureEnabled()) {
    SetSidePanelButtonTooltipText(
        l10n_util::GetStringUTF16(IDS_TOOLTIP_SIDE_PANEL_HIDE));
  }

  auto container = std::make_unique<views::FlexLayoutView>();
  // Align views vertically top to bottom.
  container->SetOrientation(views::LayoutOrientation::kVertical);
  container->SetMainAxisAlignment(views::LayoutAlignment::kStart);
  // Stretch views to fill horizontal bounds.
  container->SetCrossAxisAlignment(views::LayoutAlignment::kStretch);
  container->SetID(kSidePanelContentContainerViewId);

  if (!features::IsChromeRefresh2023()) {
    container->AddChildView(CreateHeader());
    container->AddChildView(std::make_unique<views::Separator>())
        ->SetColorId(kColorSidePanelContentAreaSeparator);
  }

  auto content_wrapper = std::make_unique<SidePanelContentSwappingContainer>(
      no_delays_for_testing_);
  container->AddChildView(std::move(content_wrapper));
  // Set to not visible so that the side panel is not shown until content is
  // ready to be shown.
  container->SetVisible(false);

  browser_view_->unified_side_panel()->AddChildView(std::move(container));
}

void SidePanelCoordinator::PopulateSidePanel(
    SidePanelEntry* entry,
    std::optional<std::unique_ptr<views::View>> content_view) {
  if (!header_combobox_) {
    actions::ActionItem* const action_item = GetActionItem(entry->key());
    UpdatePanelIconAndTitle(
        action_item->GetImage(), action_item->GetText(),
        (entry->key().id() == SidePanelEntryId::kExtension));
  } else {
    // Ensure that the correct combobox entry is selected. This may not be the
    // case if `Show()` was called after registering a contextual entry.
    SetSelectedEntryInCombobox(entry->key());
  }

  auto* content_container = GetContentContainerView();
  DCHECK(content_container);
  auto* content_wrapper =
      content_container->GetViewByID(kSidePanelContentWrapperViewId);
  DCHECK(content_wrapper);
  // |content_wrapper| should have either no child views or one child view for
  // the currently hosted SidePanelEntry.
  DCHECK(content_wrapper->children().size() <= 1);

  // Side panel is opened when the `content_container` is made visible for the
  // first time. The subsequent calls of `PopulateSidePanel()` that have visible
  // `content_container` is to update the content of the side panel.
  const bool opening_side_panel = !content_container->GetVisible();

  content_wrapper->SetVisible(true);
  content_container->SetVisible(true);
  content_container->SetProperty(
      kSidePanelContentStateKey,
      static_cast<std::underlying_type_t<SidePanelContentState>>(
          SidePanelContentState::kReadyToShow));

  if (current_entry_ && content_wrapper->children().size()) {
    auto current_entry_view =
        content_wrapper->RemoveChildViewT(content_wrapper->children().front());
    current_entry_->CacheView(std::move(current_entry_view));
  }
  auto* content = content_wrapper->AddChildView(
      content_view.has_value() ? std::move(content_view.value())
                               : entry->GetContent());
  if (auto* contextual_registry = GetActiveContextualRegistry())
    contextual_registry->ResetActiveEntry();
  auto* previous_entry = current_entry_.get();
  current_entry_ = entry->GetWeakPtr();
  if (browser_view_->toolbar()->pinned_toolbar_actions_container()) {
    NotifyPinnedContainerOfActiveStateChange(entry->key(), true);
    // Notify active state change only if the entry ids for the side panel are
    // different. This is to ensure extensions container isn't notified if we
    // switch between different extensions side panels or between global to
    // contextual side panel of the same extension.
    if (previous_entry && previous_entry->key().id() != entry->key().id()) {
      NotifyPinnedContainerOfActiveStateChange(previous_entry->key(), false);
    }
  } else if (auto* side_panel_container =
                 browser_view_->toolbar()->side_panel_container()) {
    side_panel_container->UpdateSidePanelContainerButtonsState();
  }
  entry->OnEntryShown();
  if (previous_entry) {
    previous_entry->OnEntryHidden();
  } else {
    content->RequestFocus();
  }
  UpdateNewTabButtonState();
  UpdateHeaderPinButtonState();

  // Notify the observers if the side panel is opened (made visible).
  if (opening_side_panel) {
    for (SidePanelViewStateObserver& view_state_observer :
         view_state_observers_) {
      view_state_observer.OnSidePanelDidOpen();
    }
  }
}

void SidePanelCoordinator::ClearCachedEntryViews() {
  global_registry_->ClearCachedEntryViews();
  TabStripModel* model = browser_view_->browser()->tab_strip_model();
  if (!model)
    return;
  for (int index = 0; index < model->count(); ++index) {
    auto* web_contents =
        browser_view_->browser()->tab_strip_model()->GetWebContentsAt(index);
    if (auto* registry = SidePanelRegistry::Get(web_contents))
      registry->ClearCachedEntryViews();
  }
}

std::optional<SidePanelEntry::Key> SidePanelCoordinator::GetLastActiveEntryKey()
    const {
  // In order of preference, return the active contextual entry, the active
  // global entry, the last active contextual entry, the last active global
  // entry, or nullopt.
  auto* contextual_registry = GetActiveContextualRegistry();

  if (contextual_registry && contextual_registry->active_entry().has_value()) {
    return contextual_registry->active_entry().value()->key();
  }

  if (global_registry_->active_entry().has_value()) {
    return global_registry_->active_entry().value()->key();
  }

  if (contextual_registry &&
      contextual_registry->last_active_entry().has_value()) {
    return contextual_registry->last_active_entry().value()->key();
  }

  if (global_registry_->last_active_entry().has_value()) {
    return global_registry_->last_active_entry().value()->key();
  }

  return std::nullopt;
}

std::optional<SidePanelEntry::Key> SidePanelCoordinator::GetSelectedKey()
    const {
  // If the side panel is not shown then return nullopt.
  if (!header_combobox_ || !IsSidePanelShowing() || !combobox_model_) {
    return std::nullopt;
  }

  // If we are waiting on content swapping delays we want to return the id for
  // the entry we are attempting to swap to.
  if (const auto* entry = GetLoadingEntry()) {
    return entry->key();
  }

  // If we are not waiting on content swapping we want to return the active
  // selected entry id.
  return combobox_model_->GetKeyAt(
      header_combobox_->GetSelectedIndex().value());
}

SidePanelRegistry* SidePanelCoordinator::GetActiveContextualRegistry() const {
  if (auto* web_contents =
          browser_view_->browser()->tab_strip_model()->GetActiveWebContents()) {
    return SidePanelRegistry::Get(web_contents);
  }
  return nullptr;
}

std::unique_ptr<views::View> SidePanelCoordinator::CreateHeader() {
  auto header = std::make_unique<SidePanelHeader>();
  auto* const layout =
      header->SetLayoutManager(std::make_unique<views::FlexLayout>());

  // Set alignments for horizontal (main) and vertical (cross) axes.
  layout->SetMainAxisAlignment(views::LayoutAlignment::kStart);
  layout->SetCrossAxisAlignment(views::LayoutAlignment::kCenter);

  // The minimum cross axis size should the expected height of the header.
  constexpr int kDefaultSidePanelHeaderHeight = 40;
  layout->SetMinimumCrossAxisSize(kDefaultSidePanelHeaderHeight);

  if (!combobox_model_) {
    panel_icon_ = header->AddChildView(CreateIcon());
    panel_title_ = header->AddChildView(CreateTitle());
  } else {
    header_combobox_ = header->AddChildView(CreateCombobox());
    header_combobox_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
    header_combobox_->SetProperty(views::kElementIdentifierKey,
                                  kSidePanelComboboxElementId);
  }

  header_pin_button_ =
      header->AddChildView(CreatePinToggleButton(base::BindRepeating(
          &SidePanelCoordinator::UpdatePinState, base::Unretained(this))));
  header_pin_button_->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
  // By default, the button's accessible description is set to the button's
  // tooltip text. For the pin button, we only want the accessible name to be
  // read on accessibility mode since it includes the tooltip text. Thus we set
  // the accessible description.
  header_pin_button_->GetViewAccessibility().SetDescription(
      std::u16string(), ax::mojom::DescriptionFrom::kAttributeExplicitlyEmpty);
  // The icon is later set as visible for side panels that support it.
  header_pin_button_->SetVisible(false);

  header_open_in_new_tab_button_ = header->AddChildView(CreateControlButton(
      header.get(),
      base::BindRepeating(&SidePanelCoordinator::OpenInNewTab,
                          base::Unretained(this)),
      kOpenInNewIcon, l10n_util::GetStringUTF16(IDS_ACCNAME_OPEN_IN_NEW_TAB),
      kSidePanelOpenInNewTabButtonElementId,
      ChromeLayoutProvider::Get()->GetDistanceMetric(
          ChromeDistanceMetric::DISTANCE_SIDE_PANEL_HEADER_VECTOR_ICON_SIZE)));
  header_open_in_new_tab_button_->SetFocusBehavior(
      views::View::FocusBehavior::ALWAYS);
  // The icon is later set as visible for side panels that support it.
  header_open_in_new_tab_button_->SetVisible(false);

  auto* header_close_button = header->AddChildView(CreateControlButton(
      header.get(),
      base::BindRepeating(&SidePanelCoordinator::Close, base::Unretained(this)),
      views::kIcCloseIcon, l10n_util::GetStringUTF16(IDS_ACCNAME_CLOSE),
      kSidePanelCloseButtonElementId,
      ChromeLayoutProvider::Get()->GetDistanceMetric(
          ChromeDistanceMetric::DISTANCE_SIDE_PANEL_HEADER_VECTOR_ICON_SIZE)));
  header_close_button->SetFocusBehavior(views::View::FocusBehavior::ALWAYS);

  return header;
}

std::unique_ptr<views::Combobox> SidePanelCoordinator::CreateCombobox() {
  CHECK(combobox_model_);
  auto combobox = std::make_unique<views::Combobox>(combobox_model_.get());
  combobox->SetMenuSelectionAtCallback(
      base::BindRepeating(&SidePanelCoordinator::OnComboboxChangeTriggered,
                          base::Unretained(this)));
  on_menu_will_show_subscription_ = combobox->AddMenuWillShowCallback(
      base::BindRepeating(&SidePanelCoordinator::OnComboboxMenuWillShow,
                          base::Unretained(this)));
  combobox->SetSelectedIndex(
      combobox_model_->GetIndexForKey((GetLastActiveEntryKey().value_or(
          SidePanelEntry::Key(GetDefaultEntry())))));
  combobox->SetAccessibleName(
      l10n_util::GetStringUTF16(IDS_ACCNAME_SIDE_PANEL_SELECTOR));
  combobox->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                               views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded,
                               /*adjust_height_for_width=*/false)
          .WithAlignment(views::LayoutAlignment::kStart));
  combobox->SetBorderColorId(ui::kColorSidePanelComboboxBorder);
  combobox->SetBackgroundColorId(ui::kColorSidePanelComboboxBackground);
  if (features::IsChromeRefresh2023()) {
    combobox->SetForegroundColorId(kColorSidePanelComboboxEntryTitle);
    combobox->SetForegroundIconColorId(kColorSidePanelComboboxEntryIcon);
    combobox->SetForegroundTextStyle(views::style::STYLE_HEADLINE_5);
  }
  combobox->SetEventHighlighting(true);
  combobox->SetSizeToLargestLabel(false);
  return combobox;
}

bool SidePanelCoordinator::OnComboboxChangeTriggered(size_t index) {
  SidePanelEntry::Key entry_key = combobox_model_->GetKeyAt(index);
  Show(entry_key, SidePanelUtil::SidePanelOpenTrigger::kComboboxSelected);
  views::ElementTrackerViews::GetInstance()->NotifyCustomEvent(
      kSidePanelComboboxChangedCustomEventId, header_combobox_);
  return true;
}

void SidePanelCoordinator::OnComboboxMenuWillShow() {
  SidePanelUtil::RecordComboboxShown();
}

void SidePanelCoordinator::SetSelectedEntryInCombobox(
    const SidePanelEntry::Key& entry_key) {
  CHECK(header_combobox_);
  header_combobox_->SetSelectedIndex(
      combobox_model_->GetIndexForKey(entry_key));
  header_combobox_->SchedulePaint();
}

bool SidePanelCoordinator::ShouldRemoveFromComboboxOnDeregister(
    SidePanelRegistry* deregistering_registry,
    const SidePanelEntry::Key& key) {
  // Remove the entry from the combobox if one of these conditions are met:
  //  - The entry will be deregistered from the global registry and there's no
  //    entry with the same key in the active contextual registry.
  //  - The entry will be deregistered from a contextual registry and there's
  //    no entry with the same key in the global registry.
  bool remove_if_global = deregistering_registry == global_registry_ &&
                          !GetActiveContextualEntryForKey(key);
  bool remove_if_contextual =
      deregistering_registry == GetActiveContextualRegistry() &&
      !global_registry_->GetEntryForKey(key);

  return (combobox_model_ != nullptr) &&
         (remove_if_global || remove_if_contextual);
}

SidePanelEntry* SidePanelCoordinator::GetNewActiveEntryOnDeregister(
    SidePanelRegistry* deregistering_registry,
    const SidePanelEntry::Key& key) {
  // This function should only be called when the side panel view is shown.
  DCHECK(IsSidePanelShowing());

  // Attempt to return an entry in the following fallback order: global entry
  // for `key` if a contextual entry is deregistered > active global entry >
  // null.
  if (deregistering_registry == GetActiveContextualRegistry() &&
      global_registry_->GetEntryForKey(key)) {
    return global_registry_->GetEntryForKey(key);
  }

  return global_registry_->active_entry().value_or(nullptr);
}

SidePanelEntry* SidePanelCoordinator::GetNewActiveEntryOnTabChanged() {
  // This function should only be called when the side panel view is shown.
  DCHECK(IsSidePanelShowing());

  // Attempt to return an entry in the following fallback order:
  //  - the new tab's registry's active entry
  //  - if the active entry's key is registered in the global registry:
  //    - the new tab's registry's entry with the same key
  //    - the global registry's entry with the same key (note that
  //      GetEntryForKey will return this fallback order)
  //  - if there is an active entry in the global registry:
  //    - the new tab's registry's entry with the same key
  //    - the global registry's active entry (note that GetEntryForKey will
  //      return this fallback order)
  //  - no entry (this closes the side panel)
  // Note: GetActiveContextualRegistry() returns the registry for the new tab in
  // this function.
  // Note: If Show() is called with an entry returned by this function, then
  // that entry will be active in its owning registry.
  auto* active_contextual_registry = GetActiveContextualRegistry();
  if (active_contextual_registry &&
      active_contextual_registry->active_entry()) {
    return *active_contextual_registry->active_entry();
  }

  if (current_entry_ &&
      global_registry_->GetEntryForKey(current_entry_->key())) {
    return GetEntryForKey(current_entry_->key());
  }

  return global_registry_->active_entry()
             ? GetEntryForKey((*global_registry_->active_entry())->key())
             : nullptr;
}

void SidePanelCoordinator::NotifyPinnedContainerOfActiveStateChange(
    SidePanelEntryKey key,
    bool is_active) {
  auto* toolbar_container =
      browser_view_->toolbar()->pinned_toolbar_actions_container();
  CHECK(toolbar_container);

  if (key.id() == SidePanelEntryId::kExtension) {
    browser_view_->toolbar()->extensions_container()->UpdateSidePanelState(
        is_active);
  } else {
    std::optional<actions::ActionId> action_id =
        SidePanelEntryIdToActionId(key.id());
    CHECK(action_id.has_value());
    toolbar_container->UpdateActionState(*action_id, is_active);
  }
}

void SidePanelCoordinator::OnEntryRegistered(SidePanelRegistry* registry,
                                             SidePanelEntry* entry) {
  if (combobox_model_) {
    combobox_model_->AddItem(entry);
    if (IsSidePanelShowing()) {
      SetSelectedEntryInCombobox(GetLastActiveEntryKey().value_or(
          SidePanelEntry::Key(GetDefaultEntry())));
    }
  }

  // If `entry` is a contextual entry and the global entry with the same key is
  // currently being shown, show the new `entry`.
  if (registry == GetActiveContextualRegistry() &&
      IsGlobalEntryShowing(entry->key())) {
    Show(entry, SidePanelUtil::SidePanelOpenTrigger::kExtensionEntryRegistered);
  }
}

void SidePanelCoordinator::OnEntryWillDeregister(SidePanelRegistry* registry,
                                                 SidePanelEntry* entry) {
  std::optional<SidePanelEntry::Key> selected_key = GetSelectedKey();
  if (ShouldRemoveFromComboboxOnDeregister(registry, entry->key())) {
    combobox_model_->RemoveItem(entry->key());
    if (IsSidePanelShowing()) {
      SetSelectedEntryInCombobox(GetLastActiveEntryKey().value_or(
          SidePanelEntry::Key(GetDefaultEntry())));
    }
  }

  // Save the entry's view: if it has a cached view, retrieve it. Otherwise if
  // the entry is shown, get it from the side panel view. This is necessary so
  // the view can be preserved so it won't be destroyed by Close().
  std::unique_ptr<views::View> entry_view =
      entry->CachedView() ? entry->GetContent() : nullptr;

  // Update the current entry to make sure we don't show an entry that is being
  // removed or close the panel if the entry being deregistered is the only one
  // that has been visible.
  if (IsSidePanelShowing() &&
      !browser_view_->unified_side_panel()->IsClosing() && current_entry_ &&
      (current_entry_->key() == entry->key())) {
    // If a global entry is deregistered but a contextual entry with the same
    // key is shown, do nothing.
    if (registry == global_registry_ &&
        GetActiveContextualEntryForKey(entry->key())) {
      entry->CacheView(std::move(entry_view));
      return;
    }

    // Fetch the entry's view from the side panel container if it is shown.
    auto* content_wrapper =
        GetContentContainerView()->GetViewByID(kSidePanelContentWrapperViewId);
    DCHECK(content_wrapper);
    if (content_wrapper->children().size() == 1) {
      entry_view = content_wrapper->RemoveChildViewT(
          content_wrapper->children().front());
      // TODO(crbug.com/40897366): Log the time elapsed between when this view
      // is removed, to when the new active entry's view is shown. This can
      // determine if the user will notice a flash in the side panel in between
      // different entries being shown.
    }

    if (auto* new_active_entry =
            GetNewActiveEntryOnDeregister(registry, entry->key())) {
      Show(new_active_entry,
           SidePanelUtil::SidePanelOpenTrigger::kSidePanelEntryDeregistered);
    } else {
      Close();
    }
  }

  // Cache the deregistering entry's view. This needs to be done after Close()
  // might be called because Close() clears all cached views.
  entry->CacheView(std::move(entry_view));
}

void SidePanelCoordinator::OnEntryIconUpdated(SidePanelEntry* entry) {
  if (combobox_model_) {
    combobox_model_->UpdateIconForEntry(entry);
  }
}

void SidePanelCoordinator::OnRegistryDestroying(SidePanelRegistry* registry) {
  registry_observations_.RemoveObservation(registry);
}

void SidePanelCoordinator::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (!selection.active_tab_changed()) {
    return;
  }
  // Handle removing the previous tab's contextual registry if one exists and
  // update the combobox.
  auto* old_contextual_registry =
      SidePanelRegistry::Get(selection.old_contents);
  if (old_contextual_registry) {
    registry_observations_.RemoveObservation(old_contextual_registry);
    if (combobox_model_) {
      std::vector<SidePanelEntry::Key> contextual_keys_to_remove;
      // Only remove the previous tab's contextual entries from the combobox if
      // they are not in the global registry.
      for (auto const& entry : old_contextual_registry->entries()) {
        if (!global_registry_->GetEntryForKey(entry->key())) {
          contextual_keys_to_remove.push_back(entry->key());
        }
      }
      combobox_model_->RemoveItems(contextual_keys_to_remove);
    }
  }

  // Add the current tab's contextual registry and update the combobox.
  auto* new_contextual_registry =
      SidePanelRegistry::Get(selection.new_contents);
  if (new_contextual_registry) {
    registry_observations_.AddObservation(new_contextual_registry);
    if (combobox_model_) {
      combobox_model_->AddItems(new_contextual_registry->entries());
    }
  }

  // Show an entry in the following fallback order: new contextual registry's
  // active entry > active global entry > none (close the side panel).
  if (IsSidePanelShowing() &&
      !browser_view_->unified_side_panel()->IsClosing()) {
    // Attempt to find a suitable entry to be shown after the tab switch and if
    // one is found, show it.
    if (auto* new_active_entry = GetNewActiveEntryOnTabChanged()) {
      Show(new_active_entry, SidePanelUtil::SidePanelOpenTrigger::kTabChanged);
      if (combobox_model_) {
        SetSelectedEntryInCombobox(new_active_entry->key());
      }
    } else {
      // If there is no suitable entry to be shown after the tab switch, cache
      // the view of the old contextual registry (if it was active), and close
      // the side panel.
      if (old_contextual_registry && old_contextual_registry->active_entry() &&
          *old_contextual_registry->active_entry() == current_entry_.get()) {
        auto* content_wrapper = GetContentContainerView()->GetViewByID(
            kSidePanelContentWrapperViewId);
        DCHECK(content_wrapper);
        DCHECK(content_wrapper->children().size() == 1);
        auto current_entry_view = content_wrapper->RemoveChildViewT(
            content_wrapper->children().front());
        auto* active_entry = old_contextual_registry->active_entry().value();
        active_entry->CacheView(std::move(current_entry_view));
      }
      Close();
    }
  } else if (new_contextual_registry &&
             new_contextual_registry->active_entry().has_value()) {
    Show(new_contextual_registry->active_entry().value(),
         SidePanelUtil::SidePanelOpenTrigger::kTabChanged);
  }
}

void SidePanelCoordinator::UpdateNewTabButtonState() {
  if (header_open_in_new_tab_button_ && current_entry_) {
    bool has_open_in_new_tab_button =
        current_entry_->SupportsNewTabButton() &&
        current_entry_->GetOpenInNewTabURL().is_valid();
    header_open_in_new_tab_button_->SetVisible(has_open_in_new_tab_button);
  }
}

void SidePanelCoordinator::UpdateHeaderPinButtonState() {
  if (!IsSidePanelShowing() || !current_entry_) {
    return;
  }

  Profile* const profile = browser_view_->GetProfile();
  if (!features::IsSidePanelPinningEnabled()) {
    PrefService* pref_service = profile->GetPrefs();
    if (pref_service && companion::IsCompanionFeatureEnabled()) {
      bool pinned = pref_service->GetBoolean(
          prefs::kSidePanelCompanionEntryPinnedToToolbar);
      header_pin_button_->SetToggled(pinned);
    }
    header_pin_button_->SetVisible(current_entry_->key().id() ==
                                   SidePanelEntry::Id::kSearchCompanion);
    return;
  }

  actions::ActionItem* const action_item = GetActionItem(current_entry_->key());
  std::optional<actions::ActionId> action_id = action_item->GetActionId();
  CHECK(action_id.has_value());

  bool current_pinned_state = false;

  // TODO(b/310910098): Clean condition up once/if ToolbarActionModel and
  // PinnedToolbarActionModel are merged together.
  if (const std::optional<extensions::ExtensionId> extension_id =
          current_entry_->key().extension_id();
      extension_id.has_value()) {
    ToolbarActionsModel* const actions_model =
        ToolbarActionsModel::Get(profile);

    current_pinned_state = actions_model->IsActionPinned(*extension_id);
  } else {
    PinnedToolbarActionsModel* const actions_model =
        PinnedToolbarActionsModel::Get(profile);

    current_pinned_state = actions_model->Contains(action_id.value());
  }

  header_pin_button_->SetToggled(current_pinned_state);
  header_pin_button_->SetVisible(
      !profile->IsIncognitoProfile() && !profile->IsGuestSession() &&
      action_item->GetProperty(actions::kActionItemPinnableKey));

  if (!current_pinned_state) {
    // Show IPH for side panel pinning icon.
    browser_view_->browser()->window()->MaybeShowFeaturePromo(
        feature_engagement::kIPHSidePanelGenericPinnableFeature);
  }
}

void SidePanelCoordinator::SetNoDelaysForTesting(bool no_delays_for_testing) {
  no_delays_for_testing_ = no_delays_for_testing;
  if (auto* content = GetContentContainerView()) {
    static_cast<SidePanelContentSwappingContainer*>(
        content->GetViewByID(kSidePanelContentWrapperViewId))
        ->SetNoDelaysForTesting(no_delays_for_testing_);  // IN-TEST
  }
}

void SidePanelCoordinator::UpdateToolbarButtonHighlight(
    bool side_panel_visible) {
  if (auto* side_panel_button =
          browser_view_->toolbar()->GetSidePanelButton()) {
    side_panel_button->SetHighlighted(side_panel_visible);
    side_panel_button->SetTooltipText(l10n_util::GetStringUTF16(
        side_panel_visible ? IDS_TOOLTIP_SIDE_PANEL_HIDE
                           : IDS_TOOLTIP_SIDE_PANEL_SHOW));
  }
}

void SidePanelCoordinator::UpdatePanelIconAndTitle(const ui::ImageModel& icon,
                                                   const std::u16string& text,
                                                   const bool is_extension) {
  if (is_extension) {
    ui::ImageModel updated_icon = icon;
    if (icon.IsVectorIcon()) {
      updated_icon = ui::ImageModel::FromVectorIcon(
          *icon.GetVectorIcon().vector_icon(), kColorSidePanelEntryIcon,
          icon.GetVectorIcon().icon_size());
    }
    panel_icon_->SetImage(updated_icon);
  }
  panel_icon_->SetVisible(is_extension);
  panel_title_->SetText(text);
}

void SidePanelCoordinator::OnViewVisibilityChanged(views::View* observed_view,
                                                   views::View* starting_from) {
  UpdateToolbarButtonHighlight(observed_view->GetVisible());

  if (!observed_view->GetVisible()) {
    bool closing_global = false;
    if (current_entry_) {
      // Reset current_entry_ first to prevent current_entry->OnEntryHidden()
      // from calling multiple times. This could happen in the edge cases when
      // callback inside current_entry->OnEntryHidden() is calling Close() to
      // trigger race condition.
      auto* current_entry = current_entry_.get();
      current_entry_.reset();
      if (global_registry_->GetEntryForKey(current_entry->key()) ==
          current_entry) {
        closing_global = true;
      }
      current_entry->OnEntryHidden();
    }

    // Reset active entry values for all observed registries and clear cache for
    // everything except remaining active entries (i.e. if another tab has an
    // active contextual entry).
    if (auto* contextual_registry = GetActiveContextualRegistry()) {
      contextual_registry->ResetActiveEntry();
      if (closing_global) {
        // Reset last active entry in contextual registry as global entry should
        // take precedence.
        contextual_registry->ResetLastActiveEntry();
      }
    }
    global_registry_->ResetActiveEntry();
    ClearCachedEntryViews();

    // TODO(pbos): Make this button observe panel-visibility state instead.
    if (!companion::IsCompanionFeatureEnabled()) {
      SetSidePanelButtonTooltipText(
          l10n_util::GetStringUTF16(IDS_TOOLTIP_SIDE_PANEL_SHOW));
    }

    // `OnEntryWillDeregister` (triggered by calling `OnEntryHidden`) may
    // already have deleted the content container, so check that it still
    // exists.
    if (views::View* content_container = GetContentContainerView()) {
      auto* content_wrapper =
          content_container->GetViewByID(kSidePanelContentWrapperViewId);
      if (!content_wrapper->children().empty()) {
        content_wrapper->RemoveChildViewT(content_wrapper->children().front());
      }
    }
    SidePanelUtil::RecordSidePanelClosed(opened_timestamp_);

    for (SidePanelViewStateObserver& view_state_observer :
         view_state_observers_) {
      view_state_observer.OnSidePanelDidClose();
    }
  }
}

void SidePanelCoordinator::OnActionsChanged() {
  if (current_entry_) {
    UpdateHeaderPinButtonState();
  }
}
