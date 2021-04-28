package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

public class OAPreorderMultipleEditionGame extends EAXVxTestTemplate {

    public enum TEST_TYPE {
        SIGNED_IN,
        ANONYMOUS_USER
    }

    public void testPreorderMultipleEditionGame(ITestContext context, OAPreorderMultipleEditionGame.TEST_TYPE type) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.QA_OFFER_2_UNRELEASED_STANDARD);
        final String entitlementName = entitlement.getName();

        if (type == TEST_TYPE.SIGNED_IN) {
            logFlowPoint("Login as a newly registered account"); //1
        }

        logFlowPoint("Navigate to the 'Game Detail' page of an unreleased game"); //2
        logFlowPoint("Click the 'Preorder' button on 'Game Detail' page, verify 'Offer Selection' page loads and click 'Preorder' primary button for the standard edition"); //3
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logFlowPoint("Verify the login page appears and login to continue"); //4
        }

        logFlowPoint("Complete the purchase flow"); // 5
        logFlowPoint("Click on the 'View in Library' primary button and verify the page is navigated to 'Game Library'."); // 6
        logFlowPoint("Verify the purchased game is now in 'Game Library'."); // 7

        //1
        WebDriver driver = startClientObject(context, client);
        if (type == TEST_TYPE.SIGNED_IN) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        //2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        //3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        gdpActionCTA.clickPreorderCTA();
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        logPassFail(offerSelectionPage.verifyOfferSelectionPageReached(), true);

        //4
        offerSelectionPage.clickPrimaryButton(entitlement.getOcdPath());
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        //5
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);

        //6
        gdpActionCTA.clickViewInLibraryCTA();
        GameLibrary gameLibrary = new GameLibrary(driver);
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //7
        logPassFail(gameLibrary.isGameTileVisibleByName(entitlementName), true);

        softAssertAll();
    }
}
