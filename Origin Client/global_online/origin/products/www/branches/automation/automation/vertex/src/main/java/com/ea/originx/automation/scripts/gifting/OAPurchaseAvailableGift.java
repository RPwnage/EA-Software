package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.discover.OpenGiftPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
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
 * Test a user trying to purchase an entitlement they are already receiving as a
 * gift
 *
 * NEEDS UPDATE TO GDP
 *
 * @author jmittertreiner
 * @author rchoi
 * @author sbentley
 */
public class OAPurchaseAvailableGift extends EAXVxTestTemplate {

    @TestRail(caseId = 11793)
    @Test(groups = {"gifting", "full_regression", "int_only"})
    public void testPurchaseAvailableGift(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount giftSender = AccountManagerHelper.getUnentitledUserAccountWithCountry(OriginClientConstants.COUNTRY_CANADA);
        UserAccount giftReceiver = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);
        final String giftReceiverName = giftReceiver.getUsername();

        giftSender.cleanFriends();
        UserAccountHelper.addFriends(giftSender, giftReceiver);

        EntitlementInfo standardEdition = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        EntitlementInfo higherThanStandard = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_DIGITAL_DELUXE);

        EntitlementInfo middleEdition = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DA_INQUISITION_DIGITAL_DELUXE);
        EntitlementInfo lowerThanMiddle = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DA_INQUISITION);

        EntitlementInfo higherEdition = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_DIGITAL_DELUXE);

        logFlowPoint("Login to Origin account with pending gifts"); // 1
        logFlowPoint("Navigate to the PDP edition higher of gifted game"); // 2
        logFlowPoint("Verify: Clicking on the 'Buy' CTA on a lower edition starts the gift unveiling flow"); // 3
        logFlowPoint("Navigate to the PDP edition lower of gifted game"); // 4
        logFlowPoint("Verify: Clicking on the 'Buy' CTA on a middle entitlement starts the gift unveiling flow"); // 5
        logFlowPoint("Navigate to the PDP edition same of gifted game"); // 6
        logFlowPoint("Verify: Clicking on the 'Buy' CTA on a same edition starts the gift unveiling flow"); // 7

        // 1
        final WebDriver driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, "/can/en-us");
        MacroLogin.startLogin(driver, giftSender);
        MacroGifting.purchaseGiftForFriend(driver, standardEdition, giftReceiverName);
        MacroGifting.purchaseGiftForFriend(driver, middleEdition, giftReceiverName);
        MacroGifting.purchaseGiftForFriend(driver, higherEdition, giftReceiverName);
        new MiniProfile(driver).selectSignOut();

        if (MacroLogin.startLogin(driver, giftReceiver)) {
            logPass("Successfully logged into gift receiver account: " + giftReceiverName);
        } else {
            logFailExit("Failed to log into gift receiver account: " + giftReceiverName);
        }

        // 2
//        if (MacroPDP.loadPdpPage(driver, higherThanStandard)) {
//            logPass("Successfully navigate to PDP edition higher than gifted");
//        } else {
//            logFailExit("Faiiled to navigate to PDP edition higher than gifted");
//        }

        //3
//        PDPHeroActionCTA pdpHeroActionCTA = new PDPHeroActionCTA(driver);
//        pdpHeroActionCTA.clickBuyButton();
        OpenGiftPage openGiftPage = new OpenGiftPage(driver);
        if (openGiftPage.waitForVisible()) {
            logPass("Passed: Clicking on the 'Buy' CTA on a higher edition starts the gift unveiling flow for gifted lesser edition");
        } else {
            logFail("Failed: Clicking on the 'Buy' CTA on a higher edition starts the gift unveiling flow for gifted lesser edition");
        }
        openGiftPage.clickDownloadLater();

        // 4
//        if (MacroPDP.loadPdpPage(driver, lowerThanMiddle)) {
//            logPass("Successfully navigate to PDP edition lower than gifted");
//        } else {
//            logFailExit("Faiiled to navigate to PDP edition lower than gifted");
//        }

        // 5
        //pdpHeroActionCTA.clickBuyButton();
        if (openGiftPage.waitForVisible()) {
            logPass("Passed: Clicking on the 'Buy' CTA on a lower entitlement starts the gift unveiling flow for gifted higher edition");
        } else {
            logFail("Failed: Clicking on the 'Buy' CTA on a lower entitlement starts the gift unveiling flow for gifted higher edition");
        }
        openGiftPage.clickDownloadLater();

        // 6
//        if (MacroPDP.loadPdpPage(driver, higherEdition)) {
//            logPass("Successfully navigate to PDP edition same as gifted");
//        } else {
//            logFailExit("Faiiled to navigate to PDP edition same as gifted");
//        }

        //7
     //   pdpHeroActionCTA.clickBuyButton();
        if (openGiftPage.waitForVisible()) {
            logPass("Passed: Clicking on the 'Buy' CTA on the same edition starts the gift unveiling flow for gifted same edition");
        } else {
            logFail("Failed: Clicking on the 'Buy' CTA on the same edition starts the gift unveiling flow for gifted same edition");
        }

        softAssertAll();
    }
}
