package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroGifting;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
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
 * Tests that the interstitial modal does not appear when the user is attempting to purchase a DLC for
 * and unopened base game.
 *
 * @author esdecastro
 */
public class OAPurchaseDLCUnopenedBaseGame extends EAXVxTestTemplate {

    @TestRail(caseId = 2505137)
    @Test(groups = {"gifting", "full_regression"})
    public void testPurchaseDLCUnopenBaseGame(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount senderAccount = AccountManagerHelper.registerNewThrowAwayAccountRetryThroughREST(OriginClientConstants.COUNTRY_CANADA, 5);
        final UserAccount receiverAccount = AccountManagerHelper.registerNewThrowAwayAccountRetryThroughREST(OriginClientConstants.COUNTRY_CANADA, 5);

        senderAccount.cleanFriends();
        senderAccount.addFriend(receiverAccount);

        EntitlementInfo baseGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);
        EntitlementInfo gameDLC = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_CATS_AND_DOGS);

        String senderUsername = senderAccount.getUsername();
        String receiverUsername = receiverAccount.getUsername();

        logFlowPoint("Login to Origin with the sender account: " + senderUsername); // 1
        logFlowPoint("Gift the base game to receiver account: " + receiverUsername); // 2
        logFlowPoint("Login to Origin with an account " + receiverUsername + " that has a base game unopened"); // 3
        logFlowPoint("Navigate to a giftable DLC GDP page"); // 4
        logFlowPoint("Click on the 'Buy' button and verify that the checkout dialog appears and does not open the interstitial dialog"); // 5

        final WebDriver driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, "/can/en-us");

        // 1
        logPassFail(MacroLogin.startLogin(driver, senderAccount), true);

        // 2
        logPassFail(MacroGifting.purchaseGiftForFriend(driver, baseGame, receiverUsername), true);
        new MiniProfile(driver).selectSignOut();

        // 3
        logPassFail(MacroLogin.startLogin(driver, receiverAccount), true);

        // 4
        logPassFail(MacroGDP.loadGdpPage(driver, gameDLC), true);

        // 5
        new GDPActionCTA(driver).clickBuyCTA();
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        logPassFail(paymentInformationPage.verifyPaymentInformationReached(), true);

        softAssertAll();
    }
}
