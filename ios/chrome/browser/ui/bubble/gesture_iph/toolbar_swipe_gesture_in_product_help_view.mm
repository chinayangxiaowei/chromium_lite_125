// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/gesture_iph/toolbar_swipe_gesture_in_product_help_view.h"

#import "base/time/time.h"
#import "ios/chrome/browser/shared/ui/util/rtl_geometry.h"
#import "ios/chrome/browser/shared/ui/util/uikit_ui_util.h"
#import "ios/chrome/browser/ui/bubble/bubble_constants.h"
#import "ios/chrome/browser/ui/bubble/bubble_view.h"
#import "ios/chrome/browser/ui/bubble/gesture_iph/gesture_in_product_help_constants.h"
#import "ios/chrome/browser/ui/bubble/gesture_iph/gesture_in_product_help_view+subclassing.h"
#import "ios/chrome/grit/ios_strings.h"
#import "ui/base/l10n/l10n_util.h"

namespace {

// Initial distance between the gesture indicator and the edge of the screen.
const CGFloat kInitialGestureIndicatorToEdgeSpacing = 16.0f;
// The distance that the gesture indicator should move during the animation.
const CGFloat kGestureIndicatorDistanceAnimated = 100.0f;
// The distance between the dismiss button and the bubble.
const CGFloat kDismissButtonDistanceToBubble = 80.0f;
// Number of animation cycles if the view is bidirectional.
const int kBidirectionalAnimationRepeatCount = 4;

}  // namespace

@implementation ToolbarSwipeGestureInProductHelpView {
  // Constraints to position dismiss button.
  NSArray<NSLayoutConstraint*>* _dismissButtonConstraints;
}

@synthesize bidirectional = _bidirectional;
@synthesize animationRepeatCount = _animationRepeatCount;

- (instancetype)initWithBubbleBoundingSize:(CGSize)bubbleBoundingSize
                                 canGoBack:(BOOL)back
                                   forward:(BOOL)forward {
  CHECK(back || forward);
  self = [super initWithText:l10n_util::GetNSString(IDS_IOS_TAB_STRIP_SWIPE_IPH)
          bubbleBoundingSize:bubbleBoundingSize
              swipeDirection:(back ^ UseRTLLayout())
                                 ? UISwipeGestureRecognizerDirectionRight
                                 : UISwipeGestureRecognizerDirectionLeft
       voiceOverAnnouncement:nil];
  if (self) {
    _dismissButtonConstraints = [self dismissButtonConstraints];
    if (back && forward) {
      _bidirectional = YES;
      _animationRepeatCount = kBidirectionalAnimationRepeatCount;
    } else {
      _bidirectional = NO;
      _animationRepeatCount = [super animationRepeatCount];
    }
  }
  return self;
}

- (void)traitCollectionDidChange:(UITraitCollection*)previousTraitCollection {
  if (self.traitCollection.verticalSizeClass !=
      previousTraitCollection.verticalSizeClass) {
    [self.bubbleView removeFromSuperview];
    [self setInitialBubbleViewWithDirection:
              (BubbleArrowDirection)0 /* this value is not used */
                               boundingSize:self.bounds.size];
    [NSLayoutConstraint deactivateConstraints:_dismissButtonConstraints];
    _dismissButtonConstraints = [self dismissButtonConstraints];
    [NSLayoutConstraint activateConstraints:_dismissButtonConstraints];
    self.topConstraintForBottomEdgeSwipe.active = [self shouldSwipeBottomEdge];
    self.topConstraintForTopEdgeSwipe.active = ![self shouldSwipeBottomEdge];
  }
  [super traitCollectionDidChange:previousTraitCollection];
}

- (void)setInitialBubbleViewWithDirection:(BubbleArrowDirection)direction
                             boundingSize:(CGSize)boundingSize {
  [super setInitialBubbleViewWithDirection:[self shouldSwipeBottomEdge]
                                               ? BubbleArrowDirectionDown
                                               : BubbleArrowDirectionUp
                              boundingSize:boundingSize];
  [self.bubbleView setArrowHidden:NO animated:NO];
}

- (NSLayoutConstraint*)initialGestureIndicatorMarginConstraint {
  CGFloat margin =
      kInitialGestureIndicatorToEdgeSpacing + kGestureIndicatorRadius;
  switch (self.animatingDirection) {
    case UISwipeGestureRecognizerDirectionUp:
    case UISwipeGestureRecognizerDirectionDown:
      NOTREACHED();
      return nil;
    case UISwipeGestureRecognizerDirectionLeft:
    case UISwipeGestureRecognizerDirectionRight:
    default:
      BOOL directionIsLeading =
          self.animatingDirection ==
          (UseRTLLayout() ? UISwipeGestureRecognizerDirectionRight
                          : UISwipeGestureRecognizerDirectionLeft);
      if (directionIsLeading) {
        return [self.gestureIndicator.centerXAnchor
            constraintEqualToAnchor:self.trailingAnchor
                           constant:-margin];
      }
      return [self.gestureIndicator.centerXAnchor
          constraintEqualToAnchor:self.leadingAnchor
                         constant:margin];
  }
}

- (NSLayoutConstraint*)initialGestureIndicatorCenterConstraint {
  return [self.gestureIndicator.centerYAnchor
      constraintEqualToAnchor:[self shouldSwipeBottomEdge] ? self.bottomAnchor
                                                           : self.topAnchor];
}

- (NSArray<NSLayoutConstraint*>*)dismissButtonConstraints {
  if ([self shouldSwipeBottomEdge]) {
    return @[
      [self.dismissButton.centerXAnchor
          constraintEqualToAnchor:self.centerXAnchor],
      [self.dismissButton.bottomAnchor
          constraintEqualToAnchor:self.bubbleView.topAnchor
                         constant:-kDismissButtonDistanceToBubble]
    ];
  }
  return [super dismissButtonConstraints];
}

- (CGFloat)gestureIndicatorAnimatedDistance {
  return kGestureIndicatorDistanceAnimated;
}

- (void)animateBubbleSwipeInReverseDrection:(BOOL)reverse {
  // Bubble should not move during the animation.
  return;
}

- (void)handleInstructedSwipeGesture:
    (GestureInProductHelpGestureRecognizer*)gesture {
  // Swipe action happens outside the bounds of the gesture in-product help
  // view.
  return;
}

- (void)handleDirectionChangeToOppositeDirection {
  [self startAnimationAfterDelay:kDurationBetweenBidirectionalCycles];
}

#pragma mark - Private

// Whether the use should swipe the bottom edge, instead of the top.
- (BOOL)shouldSwipeBottomEdge {
  return IsSplitToolbarMode(self.traitCollection);
}

@end
