package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
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

/**
 * Test purchase a multiple edition non-vault entitlement
 *
 * @author tdhillon
 */
public class OAPurchaseMultipleEditionNonVault extends EAXVxTestTemplate {

    public enum TEST_TYPE {
        SUBSCRIBER,
        NON_SUBSCRIBER,
        ANONYMOUS_USER
    }

    public void testPurchaseMultipleEditionNonVault(ITestContext context, TEST_TYPE type) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.ASSASSIN_CREED_SYNDICATE);
        final String entitlementName = entitlement.getParentName();
        final String entitlementOfferId = entitlement.getOfferId();

        if (type != TEST_TYPE.ANONYMOUS_USER) {
            logFlowPoint("Log into Origin as newly registered account."); // 1
        }
        if (type == TEST_TYPE.SUBSCRIBER) {
            logFlowPoint("Purchase Origin Access."); // 2
        }
        logFlowPoint("Navigate to GDP of a multiple edition non vault entitlement."); // 3
        logFlowPoint("Verify 'Buy with <price>' CTA is visible"); // 4
        logFlowPoint("Click on 'Buy' CTA on GDP page, verify OSP loads and click on 'Buy' CTA for Standard version of the entitlement"); // 5
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logFlowPoint("Login as a newly registered user."); // 6
        }
        logFlowPoint("Complete the purchase flow and close the thank you modal."); // 7
        logFlowPoint("Verify the primary button changes to 'View in Library'."); // 8
        logFlowPoint("Click on the 'View in Library' primary button and verify the page is navigated to 'Game Library'."); // 9
        logFlowPoint("Verify the purchased game is now in 'Game Library'."); // 10

        // 1
        WebDriver driver = startClientObject(context, client);
        if (type != TEST_TYPE.ANONYMOUS_USER) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        // 2
        if (type == TEST_TYPE.SUBSCRIBER) {
            logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        }

        // 3
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 4
        GDPHeader gdpHeader = new GDPHeader(driver);
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        boolean isGdpHeaderPresent = gdpHeader.verifyGDPHeaderReached();
        boolean isGameTitleCorrect = gdpHeader.verifyGameTitle(entitlementName);
        boolean isBuyOSPButtonVisible = gdpActionCTA.isBuyOSPCTAVisible();
        logPassFail(isGdpHeaderPresent && isGameTitleCorrect && isBuyOSPButtonVisible, true);

        // 5
        gdpActionCTA.clickBuyOSPCTA();
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        logPassFail(offerSelectionPage.verifyOfferSelectionPageReached(), true);

        // 6
        offerSelectionPage.clickPrimaryButton(entitlement.getOcdPath());
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        // 7
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);

        // 8
        logPassFail(gdpActionCTA.verifyViewInLibraryCTAVisible(), true);

        // 9
        gdpActionCTA.clickViewInLibraryCTA();
        GameLibrary gameLibrary = new GameLibrary(driver);
        boolean isGameLibraryReached = gameLibrary.verifyGameLibraryPageReached();
        GameSlideout gameSlideout = new GameSlideout(driver);
        gameSlideout.waitForSlideout();
        gameSlideout.clickSlideoutCloseButton();
        logPassFail(isGameLibraryReached, true);

        // 10
        logPassFail(gameLibrary.isGameTileVisibleByOfferID(entitlementOfferId), true);

        softAssertAll();
    }
}
