// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_switcher/tab_strip/coordinator/tab_strip_coordinator.h"

#import "base/check_op.h"
#import "ios/chrome/browser/shared/coordinator/layout_guide/layout_guide_util.h"
#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/shared/model/web_state_list/web_state_list.h"
#import "ios/chrome/browser/shared/public/commands/command_dispatcher.h"
#import "ios/chrome/browser/shared/public/commands/tab_strip_commands.h"
#import "ios/chrome/browser/ui/sharing/sharing_coordinator.h"
#import "ios/chrome/browser/ui/sharing/sharing_params.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_grid/grid/tab_groups/create_or_edit_tab_group_coordinator_delegate.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_grid/grid/tab_groups/create_tab_group_coordinator.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_strip/coordinator/tab_strip_mediator.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_strip/ui/swift.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_item.h"

@interface TabStripCoordinator () <CreateOrEditTabGroupCoordinatorDelegate,
                                   TabStripCommands,
                                   TabStripViewControllerDelegate>

// Mediator for updating the TabStrip when the WebStateList changes.
@property(nonatomic, strong) TabStripMediator* mediator;

@property TabStripViewController* tabStripViewController;

@end

@implementation TabStripCoordinator {
  SharingCoordinator* _sharingCoordinator;
  CreateTabGroupCoordinator* _createTabGroupCoordinator;
}

@synthesize baseViewController = _baseViewController;

#pragma mark - ChromeCoordinator

- (instancetype)initWithBrowser:(Browser*)browser {
  DCHECK(browser);
  return [super initWithBaseViewController:nil browser:browser];
}

- (void)start {
  if (self.tabStripViewController)
    return;

  [self.browser->GetCommandDispatcher()
      startDispatchingToTarget:self
                   forProtocol:@protocol(TabStripCommands)];

  ChromeBrowserState* browserState = self.browser->GetBrowserState();
  CHECK(browserState);
  self.tabStripViewController = [[TabStripViewController alloc] init];
  self.tabStripViewController.layoutGuideCenter =
      LayoutGuideCenterForBrowser(self.browser);
  self.tabStripViewController.delegate = self;
  self.tabStripViewController.overrideUserInterfaceStyle =
      browserState->IsOffTheRecord() ? UIUserInterfaceStyleDark
                                     : UIUserInterfaceStyleUnspecified;
  self.tabStripViewController.isIncognito = browserState->IsOffTheRecord();

  self.mediator =
      [[TabStripMediator alloc] initWithConsumer:self.tabStripViewController];
  self.mediator.webStateList = self.browser->GetWebStateList();
  self.mediator.browserState = browserState;
  self.mediator.browser = self.browser;
  self.mediator.tabStripHandler = HandlerForProtocol(
      self.browser->GetCommandDispatcher(), TabStripCommands);

  self.tabStripViewController.mutator = self.mediator;
  self.tabStripViewController.dragDropHandler = self.mediator;
}

- (void)stop {
  [_sharingCoordinator stop];
  _sharingCoordinator = nil;
  [self.mediator disconnect];
  self.mediator = nil;
  [self.browser->GetCommandDispatcher()
      stopDispatchingForProtocol:@protocol(TabStripCommands)];
  self.tabStripViewController = nil;
}

#pragma mark - TabStripCommands

- (void)setNewTabButtonOnTabStripIPHHighlighted:(BOOL)IPHHighlighted {
  [self.tabStripViewController
      setNewTabButtonOnTabStripIPHHighlighted:IPHHighlighted];
}

- (void)showTabStripGroupCreationForTabs:
    (const std::set<web::WebStateID>&)identifiers {
  [self hideTabStripGroupCreation];
  _createTabGroupCoordinator = [[CreateTabGroupCoordinator alloc]
      initTabGroupCreationWithBaseViewController:self.baseViewController
                                         browser:self.browser
                                    selectedTabs:identifiers];
  _createTabGroupCoordinator.delegate = self;
  [_createTabGroupCoordinator start];
}

- (void)showTabStripGroupEditionForGroup:(const TabGroup*)tabGroup {
  [self hideTabStripGroupCreation];
  _createTabGroupCoordinator = [[CreateTabGroupCoordinator alloc]
      initTabGroupEditionWithBaseViewController:self.baseViewController
                                        browser:self.browser
                                       tabGroup:tabGroup];
  _createTabGroupCoordinator.delegate = self;
  [_createTabGroupCoordinator start];
}

- (void)hideTabStripGroupCreation {
  [_createTabGroupCoordinator stop];
  _createTabGroupCoordinator = nil;
}

#pragma mark - CreateOrEditTabGroupCoordinatorDelegate

- (void)createOrEditTabGroupCoordinatorDidDismiss:
    (CreateTabGroupCoordinator*)coordinator {
  id<TabStripCommands> tabStripHandler = HandlerForProtocol(
      self.browser->GetCommandDispatcher(), TabStripCommands);
  [tabStripHandler hideTabStripGroupCreation];
}

#pragma mark - Properties

- (UIViewController*)viewController {
  return self.tabStripViewController;
}

#pragma mark - Public

- (void)hideTabStrip:(BOOL)hidden {
  self.tabStripViewController.view.hidden = hidden;
}

#pragma mark - TabStripViewControllerDelegate

- (void)tabStrip:(TabStripViewController*)tabStrip
       shareItem:(TabSwitcherItem*)item
      originView:(UIView*)originView {
  SharingParams* params =
      [[SharingParams alloc] initWithURL:item.URL
                                   title:item.title
                                scenario:SharingScenario::TabStripItem];
  _sharingCoordinator = [[SharingCoordinator alloc]
      initWithBaseViewController:self.baseViewController
                         browser:self.browser
                          params:params
                      originView:originView];
  [_sharingCoordinator start];
}

@end
