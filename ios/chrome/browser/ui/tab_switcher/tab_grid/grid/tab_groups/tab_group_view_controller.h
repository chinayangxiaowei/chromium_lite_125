// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_GRID_GRID_TAB_GROUPS_TAB_GROUP_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_GRID_GRID_TAB_GROUPS_TAB_GROUP_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/tab_switcher/tab_grid/grid/tab_groups/tab_group_consumer.h"

@class BaseGridViewController;
class TabGroup;
@protocol TabGroupsCommands;
@protocol TabGroupMutator;

// Tab group view controller displaying one group.
@interface TabGroupViewController : UIViewController <TabGroupConsumer>

// Mutator used to send notification to the tab group  model.
@property(nonatomic, weak) id<TabGroupMutator> mutator;

// The embedded grid view controller.
@property(nonatomic, readonly) BaseGridViewController* gridViewController;

// Initiates a TabGroupViewController with `handler` to handle user action,
// `lightTheme` to YES to have a light theme, `tabGroup` to get tab group
// information.
- (instancetype)initWithHandler:(id<TabGroupsCommands>)handler
                     lightTheme:(BOOL)lightTheme
                       tabGroup:(const TabGroup*)tabGroup;

// Methods handling the presentation animation of this view controller.
- (void)prepareForPresentationWithSmallMotions:(BOOL)smallMotions;
- (void)animateTopElementsPresentation;
- (void)animateGridPresentation;
- (void)fadeBlurIn;

// Methods handling the dismissal animation of this view controller.
- (void)animateDismissal;
- (void)fadeBlurOut;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_GRID_GRID_TAB_GROUPS_TAB_GROUP_VIEW_CONTROLLER_H_
