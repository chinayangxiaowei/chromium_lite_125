// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.touch_to_fill.payments;

import static org.chromium.chrome.browser.touch_to_fill.payments.TouchToFillPaymentMethodProperties.CreditCardProperties.ITEM_COLLECTION_INFO;
import static org.chromium.chrome.browser.touch_to_fill.payments.TouchToFillPaymentMethodProperties.CreditCardProperties.ON_CREDIT_CARD_CLICK_ACTION;
import static org.chromium.chrome.browser.touch_to_fill.payments.TouchToFillPaymentMethodProperties.FooterProperties.SCAN_CREDIT_CARD_CALLBACK;
import static org.chromium.chrome.browser.touch_to_fill.payments.TouchToFillPaymentMethodProperties.FooterProperties.SHOULD_SHOW_SCAN_CREDIT_CARD;
import static org.chromium.chrome.browser.touch_to_fill.payments.TouchToFillPaymentMethodProperties.IbanProperties.ON_IBAN_CLICK_ACTION;
import static org.chromium.chrome.browser.touch_to_fill.payments.TouchToFillPaymentMethodProperties.FooterProperties.SHOW_PAYMENT_METHOD_SETTINGS_CALLBACK;
import static org.chromium.chrome.browser.touch_to_fill.payments.TouchToFillPaymentMethodProperties.HeaderProperties.IMAGE_DRAWABLE_ID;
import static org.chromium.chrome.browser.touch_to_fill.payments.TouchToFillPaymentMethodProperties.ItemType.CREDIT_CARD;
import static org.chromium.chrome.browser.touch_to_fill.payments.TouchToFillPaymentMethodProperties.ItemType.IBAN;
import static org.chromium.chrome.browser.touch_to_fill.payments.TouchToFillPaymentMethodProperties.ItemType.FILL_BUTTON;
import static org.chromium.chrome.browser.touch_to_fill.payments.TouchToFillPaymentMethodProperties.SHEET_ITEMS;
import static org.chromium.chrome.browser.touch_to_fill.payments.TouchToFillPaymentMethodProperties.VISIBLE;

import android.content.Context;
import android.graphics.drawable.Drawable;

import androidx.annotation.IntDef;
import androidx.annotation.VisibleForTesting;

import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.browser.autofill.AutofillUiUtils;
import org.chromium.chrome.browser.autofill.PersonalDataManager.CreditCard;
import org.chromium.chrome.browser.autofill.PersonalDataManager.Iban;
import org.chromium.chrome.browser.touch_to_fill.common.BottomSheetFocusHelper;
import org.chromium.chrome.browser.touch_to_fill.common.FillableItemCollectionInfo;
import org.chromium.chrome.browser.touch_to_fill.payments.TouchToFillPaymentMethodComponent.Delegate;
import org.chromium.chrome.browser.touch_to_fill.payments.TouchToFillPaymentMethodProperties.FooterProperties;
import org.chromium.chrome.browser.touch_to_fill.payments.TouchToFillPaymentMethodProperties.HeaderProperties;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController.StateChangeReason;
import org.chromium.components.payments.InputProtector;
import org.chromium.ui.modelutil.MVCListAdapter.ListItem;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.url.GURL;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;
import java.util.function.Function;

/**
 * Contains the logic for the TouchToFillPaymentMethod component. It sets the state of the model and
 * reacts to events like clicks.
 */
class TouchToFillPaymentMethodMediator {
    /**
     * The final outcome that closes the Touch To Fill sheet.
     *
     * Entries should not be renumbered and numeric values should never be reused. Needs to stay
     * in sync with TouchToFill.CreditCard.Outcome in enums.xml.
     */
    @IntDef({
        TouchToFillCreditCardOutcome.CREDIT_CARD,
        TouchToFillCreditCardOutcome.VIRTUAL_CARD,
        TouchToFillCreditCardOutcome.MANAGE_PAYMENTS,
        TouchToFillCreditCardOutcome.SCAN_NEW_CARD,
        TouchToFillCreditCardOutcome.DISMISS
    })
    @Retention(RetentionPolicy.SOURCE)
    @interface TouchToFillCreditCardOutcome {
        int CREDIT_CARD = 0;
        int VIRTUAL_CARD = 1;
        int MANAGE_PAYMENTS = 2;
        int SCAN_NEW_CARD = 3;
        int DISMISS = 4;
        int MAX_VALUE = DISMISS;
    }

    @VisibleForTesting
    static final String TOUCH_TO_FILL_OUTCOME_HISTOGRAM =
            "Autofill.TouchToFill.CreditCard.Outcome2";

    @VisibleForTesting
    static final String TOUCH_TO_FILL_INDEX_SELECTED =
            "Autofill.TouchToFill.CreditCard.SelectedIndex";

    @VisibleForTesting
    static final String TOUCH_TO_FILL_NUMBER_OF_CARDS_SHOWN =
            "Autofill.TouchToFill.CreditCard.NumberOfCardsShown";

    // TODO(crbug/1383487): Remove the Context from the Mediator.
    private Context mContext;
    private TouchToFillPaymentMethodComponent.Delegate mDelegate;
    private PropertyModel mModel;
    private List<CreditCard> mCards;
    private List<Iban> mIbans;
    private BottomSheetFocusHelper mBottomSheetFocusHelper;

    private InputProtector mInputProtector = new InputProtector();

    void initialize(
            Context context,
            Delegate delegate,
            PropertyModel model,
            BottomSheetFocusHelper bottomSheetFocusHelper) {
        assert delegate != null;
        mContext = context;
        mDelegate = delegate;
        mModel = model;
        mBottomSheetFocusHelper = bottomSheetFocusHelper;
    }

    void showSheet(
            CreditCard[] cards,
            boolean shouldShowScanCreditCard,
            Function<TouchToFillPaymentMethodProperties.CardImageMetaData, Drawable>
                    cardImageFunction) {
        mInputProtector.markShowTime();

        assert cards != null;
        mCards = Arrays.asList(cards);

        ModelList sheetItems = mModel.get(SHEET_ITEMS);
        sheetItems.clear();

        for (int i = 0; i < cards.length; ++i) {
            CreditCard card = cards[i];
            final PropertyModel model =
                    createCardModel(
                            card,
                            new FillableItemCollectionInfo(i + 1, cards.length),
                            cardImageFunction);
            sheetItems.add(new ListItem(CREDIT_CARD, model));
        }

        if (cards.length == 1) {
            // Use the credit card model as the property model for the fill button too
            assert sheetItems.get(0).type == CREDIT_CARD;
            sheetItems.add(new ListItem(FILL_BUTTON, sheetItems.get(0).model));
        }

        sheetItems.add(0, buildHeader(hasOnlyLocalCards(cards)));
        sheetItems.add(buildFooterForCreditCard(shouldShowScanCreditCard));

        mBottomSheetFocusHelper.registerForOneTimeUse();
        mModel.set(VISIBLE, true);

        RecordHistogram.recordCount100Histogram(TOUCH_TO_FILL_NUMBER_OF_CARDS_SHOWN, cards.length);
    }

    public void showSheet(Iban[] ibans) {
        mInputProtector.markShowTime();

        assert ibans != null;
        mIbans = Arrays.asList(ibans);

        ModelList sheetItems = mModel.get(SHEET_ITEMS);
        sheetItems.clear();

        for (int i = 0; i < ibans.length; ++i) {
            Iban iban = ibans[i];
            final PropertyModel model = createIbanModel(iban);
            sheetItems.add(new ListItem(IBAN, model));
        }

        if (ibans.length == 1) {
            // Use the IBAN model as the property model for the fill button too.
            assert sheetItems.get(0).type == IBAN;
            sheetItems.add(new ListItem(FILL_BUTTON, sheetItems.get(0).model));
        }

        sheetItems.add(0, buildHeader(/* hasOnlyLocalPaymentMethods= */ true));
        sheetItems.add(buildFooterForIban());

        mBottomSheetFocusHelper.registerForOneTimeUse();
        mModel.set(VISIBLE, true);
        // TODO(b/332193789): Add IBAN-related metrics.
    }

    void hideSheet() {
        onDismissed(BottomSheetController.StateChangeReason.NONE);
    }

    public void onDismissed(@StateChangeReason int reason) {
        // TODO(b/332193789): Add IBAN-related metrics.
        if (!mModel.get(VISIBLE)) return; // Dismiss only if not dismissed yet.
        mModel.set(VISIBLE, false);
        boolean dismissedByUser =
                reason == StateChangeReason.SWIPE
                        || reason == StateChangeReason.BACK_PRESS
                        || reason == StateChangeReason.TAP_SCRIM;
        mDelegate.onDismissed(dismissedByUser);
        if (dismissedByUser) {
            RecordHistogram.recordEnumeratedHistogram(
                    TOUCH_TO_FILL_OUTCOME_HISTOGRAM,
                    TouchToFillCreditCardOutcome.DISMISS,
                    TouchToFillCreditCardOutcome.MAX_VALUE + 1);
        }
    }

    public void scanCreditCard() {
        mDelegate.scanCreditCard();
        recordTouchToFillOutcomeHistogram(TouchToFillCreditCardOutcome.SCAN_NEW_CARD);
    }

    public void showPaymentMethodSettings() {
        mDelegate.showPaymentMethodSettings();
        recordTouchToFillOutcomeHistogram(TouchToFillCreditCardOutcome.MANAGE_PAYMENTS);
    }

    public void onSelectedCreditCard(CreditCard card) {
        if (!mInputProtector.shouldInputBeProcessed()) return;
        mDelegate.suggestionSelected(card.getGUID(), card.getIsVirtual());
        recordTouchToFillOutcomeHistogram(
                card.getIsVirtual()
                        ? TouchToFillCreditCardOutcome.VIRTUAL_CARD
                        : TouchToFillCreditCardOutcome.CREDIT_CARD);
        RecordHistogram.recordCount100Histogram(TOUCH_TO_FILL_INDEX_SELECTED, mCards.indexOf(card));
    }

    public void onSelectedIban(Iban iban) {}

    private PropertyModel createCardModel(
            CreditCard card,
            FillableItemCollectionInfo itemCollectionInfo,
            Function<TouchToFillPaymentMethodProperties.CardImageMetaData, Drawable>
                    cardImageFunction) {
        int drawableId = card.getIssuerIconDrawableId();
        GURL artUrl =
                AutofillUiUtils.shouldShowCustomIcon(card.getCardArtUrl(), card.getIsVirtual())
                        ? card.getCardArtUrl()
                        : new GURL("");
        TouchToFillPaymentMethodProperties.CardImageMetaData cardImageMetaData =
                new TouchToFillPaymentMethodProperties.CardImageMetaData(drawableId, artUrl);
        PropertyModel.Builder creditCardModelBuilder =
                new PropertyModel.Builder(
                                TouchToFillPaymentMethodProperties.CreditCardProperties
                                        .NON_TRANSFORMING_CREDIT_CARD_KEYS)
                        .withTransformingKey(
                                TouchToFillPaymentMethodProperties.CreditCardProperties.CARD_IMAGE,
                                cardImageFunction,
                                cardImageMetaData)
                        .with(TouchToFillPaymentMethodProperties.CreditCardProperties.NETWORK_NAME, "")
                        .with(
                                TouchToFillPaymentMethodProperties.CreditCardProperties.CARD_NAME,
                                card.getCardNameForAutofillDisplay())
                        .with(
                                TouchToFillPaymentMethodProperties.CreditCardProperties.CARD_NUMBER,
                                card.getObfuscatedLastFourDigits())
                        .with(ON_CREDIT_CARD_CLICK_ACTION, () -> this.onSelectedCreditCard(card))
                        .with(ITEM_COLLECTION_INFO, itemCollectionInfo);

        // If a card has a nickname, the network name should also be announced, otherwise the name
        // of the card will be the network name and it will be announced.
        if (!card.getBasicCardIssuerNetwork()
                .equals(card.getCardNameForAutofillDisplay().toLowerCase(Locale.getDefault()))) {
            creditCardModelBuilder.with(
                    TouchToFillPaymentMethodProperties.CreditCardProperties.NETWORK_NAME,
                    card.getBasicCardIssuerNetwork());
        }

        // For virtual cards, show the "Virtual card" label on the second line, and for non-virtual
        // cards, show the expiration date.
        if (card.getIsVirtual()) {
            creditCardModelBuilder.with(
                    TouchToFillPaymentMethodProperties.CreditCardProperties.VIRTUAL_CARD_LABEL,
                    mContext.getString(R.string.autofill_virtual_card_number_switch_label));
        } else {
            creditCardModelBuilder.with(
                    TouchToFillPaymentMethodProperties.CreditCardProperties.CARD_EXPIRATION,
                    card.getFormattedExpirationDate(mContext));
        }
        return creditCardModelBuilder.build();
    }

    private PropertyModel createIbanModel(Iban iban) {
        PropertyModel.Builder ibanModelBuilder =
                new PropertyModel.Builder(
                                TouchToFillPaymentMethodProperties.IbanProperties
                                        .NON_TRANSFORMING_IBAN_KEYS)
                        .with(
                            TouchToFillPaymentMethodProperties.IbanProperties.IBAN_VALUE,
                                iban.getLabel())
                        .with(
                            TouchToFillPaymentMethodProperties.IbanProperties.IBAN_NICKNAME,
                                iban.getNickname())
                        .with(ON_IBAN_CLICK_ACTION, () -> this.onSelectedIban(iban));
        return ibanModelBuilder.build();
    }

    private ListItem buildHeader(boolean hasOnlyLocalPaymentMethods) {
        return new ListItem(
                TouchToFillPaymentMethodProperties.ItemType.HEADER,
                new PropertyModel.Builder(HeaderProperties.ALL_KEYS)
                        .with(
                                IMAGE_DRAWABLE_ID,
                                hasOnlyLocalPaymentMethods
                                        ? R.drawable.fre_product_logo
                                        : R.drawable.google_pay)
                        .build());
    }

    private ListItem buildFooterForCreditCard(boolean hasScanCardButton) {
        return new ListItem(
                TouchToFillPaymentMethodProperties.ItemType.FOOTER,
                new PropertyModel.Builder(FooterProperties.ALL_KEYS)
                        .with(SHOULD_SHOW_SCAN_CREDIT_CARD, hasScanCardButton)
                        .with(SCAN_CREDIT_CARD_CALLBACK, this::scanCreditCard)
                        .with(
                                SHOW_PAYMENT_METHOD_SETTINGS_CALLBACK,
                                this::showPaymentMethodSettings)
                        .build());
    }

    private ListItem buildFooterForIban() {
        return new ListItem(
                TouchToFillPaymentMethodProperties.ItemType.FOOTER,
                new PropertyModel.Builder(FooterProperties.ALL_KEYS)
                        .with(
                                SHOW_PAYMENT_METHOD_SETTINGS_CALLBACK,
                                this::showPaymentMethodSettings)
                        .build());
    }

    private static boolean hasOnlyLocalCards(CreditCard[] cards) {
        for (CreditCard card : cards) {
            if (!card.getIsLocal()) return false;
        }
        return true;
    }

    private static void recordTouchToFillOutcomeHistogram(
            @TouchToFillCreditCardOutcome int outcome) {
        RecordHistogram.recordEnumeratedHistogram(
                TOUCH_TO_FILL_OUTCOME_HISTOGRAM,
                outcome,
                TouchToFillCreditCardOutcome.MAX_VALUE + 1);
    }

    void setInputProtectorForTesting(InputProtector inputProtector) {
        mInputProtector = inputProtector;
    }
}
