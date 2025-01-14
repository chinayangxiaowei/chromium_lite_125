// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_switcher/tab_grid/toolbars/tab_grid_toolbars_mediator.h"

#import "ios/chrome/browser/ui/menu/action_factory.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_grid/toolbars/tab_grid_bottom_toolbar.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_grid/toolbars/tab_grid_toolbars_configuration.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_grid/toolbars/tab_grid_toolbars_grid_delegate.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_grid/toolbars/tab_grid_top_toolbar.h"

@implementation TabGridToolbarsMediator {
  // Configuration that provides all buttons to display.
  TabGridToolbarsConfiguration* _configuration;
  TabGridToolbarsConfiguration* _previousConfiguration;
  id<TabGridToolbarsGridDelegate> _buttonsDelegate;

  // YES if buttons are disabled.
  BOOL _isDisabled;
}

#pragma mark - GridToolbarsMutator

- (void)setToolbarConfiguration:(TabGridToolbarsConfiguration*)configuration {
  if (_isDisabled) {
    // Handle page change during drag and drop.
    _previousConfiguration = configuration;
    return;
  }

  _configuration = configuration;

  self.topToolbarConsumer.page = configuration.page;
  self.topToolbarConsumer.mode = configuration.mode;
  self.bottomToolbarConsumer.page = configuration.page;
  self.bottomToolbarConsumer.mode = configuration.mode;

  // TODO(crbug.com/1457146): Add all buttons management.
  [self configureSelectionModeButtons];

  // Configures titles.
  self.topToolbarConsumer.selectedTabsCount = _configuration.selectedItemsCount;
  self.bottomToolbarConsumer.selectedTabsCount =
      _configuration.selectedItemsCount;
  if (_configuration.selectAllButton) {
    [self.topToolbarConsumer configureSelectAllButtonTitle];
  } else {
    [self.topToolbarConsumer configureDeselectAllButtonTitle];
  }

  [self configureEditOrUndoButton];

  [self.bottomToolbarConsumer
      setNewTabButtonEnabled:_configuration.newTabButton];

  [self.topToolbarConsumer setDoneButtonEnabled:_configuration.doneButton];
  [self.bottomToolbarConsumer setDoneButtonEnabled:_configuration.doneButton];

  [self.topToolbarConsumer setSearchButtonEnabled:_configuration.searchButton];
}

- (void)setToolbarsButtonsDelegate:(id<TabGridToolbarsGridDelegate>)delegate {
  _buttonsDelegate = delegate;
  self.topToolbarConsumer.buttonsDelegate = delegate;
  self.bottomToolbarConsumer.buttonsDelegate = delegate;
}

- (void)setButtonsEnabled:(BOOL)enabled {
  // Do not do anything if the state do not change.
  if (enabled != _isDisabled) {
    return;
  }

  if (enabled) {
    // Set the disabled boolean before modifiying the toolbar configuration
    // because the configuration setup is skipped when disabled.
    _isDisabled = NO;
    [self setToolbarConfiguration:_previousConfiguration];
  } else {
    _previousConfiguration = _configuration;
    [self setToolbarConfiguration:
              [TabGridToolbarsConfiguration
                  disabledConfigurationForPage:TabGridPageRegularTabs]];
    // Set the disabled boolean after modifiying the toolbar configuration
    // because the configuration setup is skipped when disabled.
    _isDisabled = YES;
  }
}

#pragma mark - Private

// Helpers to configure all selection mode buttons.
- (void)configureSelectionModeButtons {
  [self.bottomToolbarConsumer
      setShareTabsButtonEnabled:_configuration.shareButton];

  [self.bottomToolbarConsumer setAddToButtonEnabled:_configuration.addToButton];
  if (_configuration.addToButton) {
    [self.bottomToolbarConsumer
        setAddToButtonMenu:_configuration.addToButtonMenu];
  }

  [self.bottomToolbarConsumer
      setCloseTabsButtonEnabled:_configuration.closeSelectedTabsButton];

  [self.topToolbarConsumer
      setSelectAllButtonEnabled:_configuration.selectAllButton ||
                                _configuration.deselectAllButton];
}

// Helpers to determine which button should be selected between "Edit" or "Undo"
// and if the "Edit" button should be enabled.
// TODO(crbug.com/1457146): Send buttons configuration directly to the correct
// consumer instead of send information to object when it is not necessary.
- (void)configureEditOrUndoButton {
  [self.topToolbarConsumer useUndoCloseAll:_configuration.undoButton];
  [self.bottomToolbarConsumer useUndoCloseAll:_configuration.undoButton];

  // TODO(crbug.com/1457146): Separate "Close All" and "Undo".
  [self.topToolbarConsumer
      setCloseAllButtonEnabled:_configuration.closeAllButton ||
                               _configuration.undoButton];
  [self.bottomToolbarConsumer
      setCloseAllButtonEnabled:_configuration.closeAllButton ||
                               _configuration.undoButton];

  BOOL shouldEnableEditButton =
      _configuration.closeAllButton || _configuration.selectTabsButton;

  [self configureEditButtons];
  [self.bottomToolbarConsumer setEditButtonEnabled:shouldEnableEditButton];
  [self.topToolbarConsumer setEditButtonEnabled:shouldEnableEditButton];
}

// Configures buttons that are available under the edit menu.
- (void)configureEditButtons {
  BOOL shouldEnableEditButton =
      _configuration.closeAllButton || _configuration.selectTabsButton;

  UIMenu* menu = nil;
  if (shouldEnableEditButton) {
    ActionFactory* actionFactory = [[ActionFactory alloc]
        initWithScenario:kMenuScenarioHistogramTabGridEdit];
    __weak id<TabGridToolbarsGridDelegate> weakButtonDelegate =
        _buttonsDelegate;
    NSMutableArray<UIMenuElement*>* menuElements =
        [@[ [actionFactory actionToCloseAllTabsWithBlock:^{
          [weakButtonDelegate closeAllButtonTapped:nil];
        }] ] mutableCopy];
    // Disable the "Select All" option from the edit button when there are no
    // tabs in the regular tab grid. "Close All" can still be called if there
    // are inactive tabs.
    if (_configuration.selectTabsButton) {
      [menuElements addObject:[actionFactory actionToSelectTabsWithBlock:^{
                      [weakButtonDelegate selectTabsButtonTapped:nil];
                    }]];
    }

    menu = [UIMenu menuWithChildren:menuElements];
  }

  [self.topToolbarConsumer setEditButtonMenu:menu];
  [self.bottomToolbarConsumer setEditButtonMenu:menu];
}

@end
