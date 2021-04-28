package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.store.ReviewOrderPage;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the discount applies for an origin access user while sending a gift
 *
 * NEEDS UPDATE TO GDP
 *
 * @author nvarthakavi
 */
public class OAGiftingSubscriber extends EAXVxTestTemplate {

    @TestRail(caseId = 11792)
    @Test(groups = {"gifting", "full_regression", "int_only"})
    public void testGiftingSubscriber(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount senderAccount = AccountManagerHelper.getTaggedUserAccountWithCountry(AccountTags.SUBSCRIPTION_ACTIVE, OriginClientConstants.COUNTRY_CANADA);
        final UserAccount receiverAccount = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_CANADA);
        final String senderAccountName = senderAccount.getUsername();
        final String receiverAccountName = receiverAccount.getUsername();
        EntitlementInfo baseGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String baseGameName = baseGame.getName();
        final String baseGameOfferId = baseGame.getOfferId();

        logFlowPoint(String.format("Launch Origin and login as user '%s' and navigate to the PDP '%s'", senderAccountName, baseGameName)); // 1
        logFlowPoint(String.format("Verify the discount is mentioned in PDP page of '%s' for an origin access user '%s' ", baseGameName, senderAccountName)); // 2
        logFlowPoint(String.format("Select the Gift this Game and Select the user '%s' to send as a gift and complete the gift message page", receiverAccountName)); // 3
        logFlowPoint(String.format("On Clicking the next button on gift message dialog,verify in the review order page the discount is mentioned for an origin access user", senderAccountName)); // 4
        logFlowPoint(String.format("On Clicking the pay now button on review order page,verify in the thank you page the discount is mentioned for an origin access user", senderAccountName)); // 5

        final WebDriver driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, "/can/en-us");
        MacroLogin.startLogin(driver, receiverAccount);
        new MiniProfile(driver).selectSignOut();

        // clean add the sender and receiver as friends
        senderAccount.cleanFriends();
        senderAccount.addFriend(receiverAccount);

        // 1
        if (MacroLogin.startLogin(driver, senderAccount)) {
            logPass("Verified login successful to user account: " + senderAccountName);
        } else {
            logFailExit("Failed: Cannot login to user account: " + senderAccountName);
        }

        // 2
//       // PDPHeroActionCTA pdpHeroCTA = new PDPHeroActionCTA(driver);
//        // PDPHeroActionDescriptors pdpHeroDescriptors = new PDPHeroActionDescriptors(driver);
//       // pdpHeroCTA.clickYouCanPurchaseLink();
//      //  double pricePDP = pdpHeroCTA.getDiscountAmount();
//      //  boolean isDiscountPDP = pdpHeroDescriptors.verifyOriginAccessDiscountIsVisible();
//        if (isDiscountPDP) {
//            logPass("Verified the discount is visible in PDP page of" + baseGameName);
//        } else {
//            logFailExit("Failed: The discount is not visible in PDP page of" + baseGameName);
//        }

        // 3
        if (MacroGifting.prepareGiftPurchase(driver, baseGameName, baseGameOfferId, receiverAccountName, "This is a gift")) {
            logPass("Successfully started the gift flow for" + baseGameName);
        } else {
            logFailExit("Failed to start the gift flow for " + baseGameName);
        }

        // 4
        MacroPurchase.handlePaymentInfoPage(driver);
        ReviewOrderPage reviewOrderPage = new ReviewOrderPage(driver);
//        boolean isPriceMatches = pricePDP == reviewOrderPage.getBaseAmount();
//        boolean isTotalCost = reviewOrderPage.verifyTotalCost();
//        boolean isDiscountReview = reviewOrderPage.verifyOriginAccessDiscountIsVisible();
//        double totalCost = StringHelper.extractNumberFromText(reviewOrderPage.getPrice());
//        if (isDiscountReview && isPriceMatches && isTotalCost) {
//            logPass("Verified the discount is visible in Review Order page of" + baseGameName);
//        } else {
//            logFail("Failed: The discount is not visible in Review Order page of" + baseGameName);
//        }

        // 5
//        boolean isDiscountVisibleThankYou = totalCost == new ThankYouPage(driver).getTotalAmount();
//        if (isDiscountVisibleThankYou) {
//            logPass("Verified the discount is visible in Thank You page of" + baseGameName);
//        } else {
//            logFailExit("Failed: The discount is not visible in Thank You page of" + baseGameName);
//        }

        softAssertAll();

    }
}
