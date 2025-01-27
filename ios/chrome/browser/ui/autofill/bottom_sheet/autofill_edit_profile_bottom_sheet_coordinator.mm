// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/autofill/bottom_sheet/autofill_edit_profile_bottom_sheet_coordinator.h"

#import "components/autofill/core/browser/data_model/autofill_profile.h"
#import "ios/chrome/browser/autofill/model/bottom_sheet/autofill_bottom_sheet_tab_helper.h"
#import "ios/chrome/browser/autofill/model/personal_data_manager_factory.h"
#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/shared/model/web_state_list/web_state_list.h"
#import "ios/chrome/browser/shared/ui/table_view/table_view_navigation_controller.h"
#import "ios/chrome/browser/ui/autofill/autofill_country_selection_table_view_controller.h"
#import "ios/chrome/browser/ui/autofill/autofill_profile_edit_mediator.h"
#import "ios/chrome/browser/ui/autofill/autofill_profile_edit_mediator_delegate.h"
#import "ios/chrome/browser/ui/autofill/autofill_profile_edit_table_view_controller.h"
#import "ios/chrome/browser/ui/autofill/bottom_sheet/autofill_edit_profile_bottom_sheet_table_view_controller.h"
#import "ios/chrome/browser/ui/autofill/cells/country_item.h"

@interface AutofillEditProfileBottomSheetCoordinator () <
    AutofillProfileEditMediatorDelegate>
@end

@implementation AutofillEditProfileBottomSheetCoordinator {
  // Profile to be edited.
  std::unique_ptr<autofill::AutofillProfile> _autofillProfile;

  // Navigation controller presented by this coordinator.
  TableViewNavigationController* _navigationController;

  // TVC for displaying the bottom sheet.
  AutofillEditProfileBottomSheetTableViewController* _viewController;

  // Mediator and view controller used to display the edit view.
  AutofillProfileEditTableViewController*
      _autofillProfileEditTableViewController;
  AutofillProfileEditMediator* _autofillProfileEditMediator;

  raw_ptr<autofill::PersonalDataManager> _personalDataManager;
}

- (instancetype)initWithBaseViewController:
                    (UINavigationController*)viewController
                                   browser:(Browser*)browser {
  self = [super initWithBaseViewController:viewController browser:browser];
  if (self) {
    ChromeBrowserState* browserState = browser->GetBrowserState();

    // Address Save Prompt is not shown in the incognito mode.
    CHECK(!browserState->IsOffTheRecord());
    _personalDataManager =
        autofill::PersonalDataManagerFactory::GetForBrowserState(browserState);

    web::WebState* webState = browser->GetWebStateList()->GetActiveWebState();
    AutofillBottomSheetTabHelper* bottomSheetTabHelper =
        AutofillBottomSheetTabHelper::FromWebState(webState);

    _autofillProfile = bottomSheetTabHelper->address_profile_for_edit();
    CHECK(_autofillProfile);
  }
  return self;
}

#pragma mark - ChromeCoordinator

- (void)start {
  _autofillProfileEditMediator = [[AutofillProfileEditMediator alloc]
         initWithDelegate:self
      personalDataManager:_personalDataManager
          autofillProfile:_autofillProfile.get()
              countryCode:nil
        isMigrationPrompt:NO];

  // Bottom sheet table VC
  AutofillEditProfileBottomSheetTableViewController* editModalViewController =
      [[AutofillEditProfileBottomSheetTableViewController alloc]
          initWithStyle:UITableViewStylePlain];

  // View controller that lays down the table views for the edit profile view.
  _autofillProfileEditTableViewController =
      [[AutofillProfileEditTableViewController alloc]
          initWithDelegate:_autofillProfileEditMediator
                 userEmail:@""
                controller:editModalViewController
              settingsView:NO];
  _autofillProfileEditMediator.consumer =
      _autofillProfileEditTableViewController;
  // `editModalViewController` lays down the bottom sheet view and communicates
  // with `_autofillProfileEditTableViewController` via
  // `AutofillProfileEditHandler` protocol.
  // `_autofillProfileEditTableViewController` is responsible for loading the
  // model and dealing with the table view user interactions.
  editModalViewController.handler = _autofillProfileEditTableViewController;

  _viewController = editModalViewController;

  _navigationController =
      [[TableViewNavigationController alloc] initWithTable:_viewController];
  _navigationController.modalPresentationStyle = UIModalPresentationFormSheet;
  _navigationController.modalTransitionStyle =
      UIModalTransitionStyleCoverVertical;

  [self.baseViewController presentViewController:_navigationController
                                        animated:YES
                                      completion:nil];
}

- (void)stop {
  [super stop];
  [_navigationController.presentingViewController
      dismissViewControllerAnimated:YES
                         completion:nil];
  _viewController = nil;
  _autofillProfileEditMediator = nil;
}

#pragma mark - AutofillProfileEditMediatorDelegate

- (void)autofillEditProfileMediatorDidFinish:
    (AutofillProfileEditMediator*)mediator {
  // TODO(crbug.com/1482269): Implement.
}

- (void)willSelectCountryWithCurrentlySelectedCountry:(NSString*)country
                                          countryList:(NSArray<CountryItem*>*)
                                                          allCountries {
  // TODO(crbug.com/1482269): Implement.
}

- (void)didSaveProfile {
  // TODO(crbug.com/1482269): Implement.
}

@end
