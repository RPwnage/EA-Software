package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test purchase a multiple edition vault entitlement
 *
 * @author mdobre
 */
public class OAPurchaseMultipleEditionVault extends EAXVxTestTemplate {

    public enum TEST_TYPE {
        NON_SUBSCRIBER,
        ANONYMOUS_USER
    }

    public void testPurchaseMultipleEditionVault(ITestContext context, OAPurchaseMultipleEditionVault.TEST_TYPE type) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String entitlementName = entitlement.getName();

        if (type == TEST_TYPE.NON_SUBSCRIBER) {
            logFlowPoint("Log into Origin as newly registered account."); // 1
        }
        logFlowPoint("Navigate to GDP of a multiple edition vault entitlement and verify 'Get the Game' CTA is visible."); // 2
        logFlowPoint("Click the 'Buy' option and verify the user is navigated to the OSP Page."); // 3
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logFlowPoint("Log in as a newly registered user."); // 4
        }
        logFlowPoint("Complete the purchase flow and verify the 'View in Library' CTA is visible."); // 5
        logFlowPoint("Click on the 'View in Library' primary button and verify the page is navigated to 'Game Library'."); // 6
        logFlowPoint("Verify the purchased game is now in 'Game Library'."); // 7

        WebDriver driver = startClientObject(context, client);

        // 1
        if (type == OAPurchaseMultipleEditionVault.TEST_TYPE.NON_SUBSCRIBER) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        // 2
        MacroGDP.loadGdpPage(driver, entitlement);
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyGetTheGameCTAVisible(), true);
        gdpActionCTA.clickGetTheGameCTA();

        //3
        AccessInterstitialPage accessInterstitialPage = new AccessInterstitialPage(driver);
        logPassFail(accessInterstitialPage.verifyOSPInterstitialPageReached(), true);
        accessInterstitialPage.clickBuyGameOSPCTA();

        //4
        if (type == OAPurchaseMultipleEditionVault.TEST_TYPE.ANONYMOUS_USER) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        //5
        new OfferSelectionPage(driver).clickPrimaryButton(entitlement.getOcdPath());
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        if (OSInfo.getEnvironment().equalsIgnoreCase("production")) {
            paymentInformationPage.switchToPaymentByPaypalAndSkipToReviewOrderPage();
            MacroPurchase.completePurchaseByPaypalOffCode(driver, entitlement.getName(), userAccount.getEmail());
            MacroPurchase.handleThankYouPage(driver);
        } else {
            logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);
        }

        //6
        gdpActionCTA.clickViewInLibraryCTA();
        GameLibrary gameLibrary = new GameLibrary(driver);
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //7
        logPassFail(gameLibrary.isGameTileVisibleByName(entitlementName), true);

        softAssertAll();
    }
}