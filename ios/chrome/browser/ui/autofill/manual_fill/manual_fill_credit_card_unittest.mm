// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/autofill/manual_fill/manual_fill_credit_card.h"

#import "testing/platform_test.h"

using ManualFillCreditCardiOSTest = PlatformTest;

autofill::CreditCard::RecordType LOCAL_CARD_RECORD_TYPE =
    autofill::CreditCard::RecordType::kLocalCard;

// Tests that a credential is correctly created.
TEST_F(ManualFillCreditCardiOSTest, Creation) {
  NSString* GUID = @"1234-5678-abcd";
  NSString* network = @"Viza";
  NSString* bankName = @"Bank of 'Merica";
  NSString* cardHolder = @"Fred Itcard";
  NSString* number = @"4321 1234 4321 1234";
  NSString* obfuscatedNumber = @"**** **** **** 1234";
  NSString* expirationYear = @"19";
  NSString* expirationMonth = @"10";
  ManualFillCreditCard* card =
      [[ManualFillCreditCard alloc] initWithGUID:GUID
                                         network:network
                             issuerNetworkIconID:1
                                        bankName:bankName
                                      cardHolder:cardHolder
                                          number:number
                                obfuscatedNumber:obfuscatedNumber
                                  expirationYear:expirationYear
                                 expirationMonth:expirationMonth
                                      recordType:LOCAL_CARD_RECORD_TYPE
                                 canFillDirectly:true];
  EXPECT_TRUE(card);
  EXPECT_TRUE([GUID isEqualToString:card.GUID]);
  EXPECT_TRUE([network isEqualToString:card.network]);
  EXPECT_EQ(card.issuerNetworkIconID, 1);
  EXPECT_TRUE([cardHolder isEqualToString:card.cardHolder]);
  EXPECT_TRUE([number isEqualToString:card.number]);
  EXPECT_TRUE([obfuscatedNumber isEqualToString:card.obfuscatedNumber]);
  EXPECT_TRUE([expirationYear isEqualToString:card.expirationYear]);
  EXPECT_TRUE([expirationMonth isEqualToString:card.expirationMonth]);
  EXPECT_EQ(card.recordType, LOCAL_CARD_RECORD_TYPE);
}

// Test equality between credit cards.
TEST_F(ManualFillCreditCardiOSTest, Equality) {
  NSString* GUID = @"1234-5678-abcd";
  NSString* network = @"Viza";
  NSString* bankName = @"Bank of 'Merica";
  NSString* cardHolder = @"Fred Itcard";
  NSString* number = @"4321 1234 4321 1234";
  NSString* obfuscatedNumber = @"**** **** **** 1234";
  NSString* expirationYear = @"19";
  NSString* expirationMonth = @"10";

  ManualFillCreditCard* card =
      [[ManualFillCreditCard alloc] initWithGUID:GUID
                                         network:network
                             issuerNetworkIconID:1
                                        bankName:bankName
                                      cardHolder:cardHolder
                                          number:number
                                obfuscatedNumber:obfuscatedNumber
                                  expirationYear:expirationYear
                                 expirationMonth:expirationMonth
                                      recordType:LOCAL_CARD_RECORD_TYPE
                                 canFillDirectly:true];

  ManualFillCreditCard* equalCard =
      [[ManualFillCreditCard alloc] initWithGUID:GUID
                                         network:network
                             issuerNetworkIconID:1
                                        bankName:bankName
                                      cardHolder:cardHolder
                                          number:number
                                obfuscatedNumber:obfuscatedNumber
                                  expirationYear:expirationYear
                                 expirationMonth:expirationMonth
                                      recordType:LOCAL_CARD_RECORD_TYPE
                                 canFillDirectly:true];

  EXPECT_TRUE([card isEqual:equalCard]);

  ManualFillCreditCard* differentGuidCredential =
      [[ManualFillCreditCard alloc] initWithGUID:@"wxyz-8765-4321"
                                         network:network
                             issuerNetworkIconID:1
                                        bankName:bankName
                                      cardHolder:cardHolder
                                          number:number
                                obfuscatedNumber:obfuscatedNumber
                                  expirationYear:expirationYear
                                 expirationMonth:expirationMonth
                                      recordType:LOCAL_CARD_RECORD_TYPE
                                 canFillDirectly:true];
  EXPECT_FALSE([card isEqual:differentGuidCredential]);

  // Guid is the main differentiator, and as long as the guids are equal,
  // the other fields do not matter.
}
