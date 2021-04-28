package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase a multiple edition vault entitlement with a subscriber account
 *
 * @author mdobre
 */
public class OAPurchaseMultipleEditionVaultSubscriber extends EAXVxTestTemplate {

    @TestRail(caseId = 3068473)
    @Test(groups = {"gdp", "full_regression", "release_smoke"})
    public void testPurchaseMultipleEditionVaultSubscriber(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String entitlementName = entitlement.getName();

        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Purchase Origin Access."); // 2
        logFlowPoint("Navigate to GDP of a multiple edition vault entitlement and verify 'Add to Game Library' CTA is visible."); // 3
        logFlowPoint("Click the 'Buy' option and verify the user is navigated to the OSP Page."); // 4
        logFlowPoint("Complete the purchase flow and verify the 'View in Library' CTA is visible."); // 5
        logFlowPoint("Click on the 'View in Library' primary button and verify the page is navigated to 'Game Library'."); // 6
        logFlowPoint("Verify the purchased game is now in 'Game Library'."); // 7

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        if (OSInfo.getEnvironment().equalsIgnoreCase("production")) {
            new MainMenu(driver).selectRedeemProductCode();
            logPassFail(MacroRedeem.redeemOABasicSubscription(driver, userAccount.getEmail()), true);
        } else {
            logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        }

        // 3
        MacroGDP.loadGdpPage(driver, entitlement);
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyAddToLibraryVaultGameCTAVisible(), true);

        //4
        gdpActionCTA.selectDropdownBuy();
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        logPassFail(offerSelectionPage.verifyOfferSelectionPageReached(), true);

        //5
        offerSelectionPage.clickPrimaryButton(entitlement.getOcdPath());
        if (OSInfo.getEnvironment().equalsIgnoreCase("production")) {
            MacroPurchase.handlePaymentInfoPageForProdPurchases(driver, entitlementName, userAccount.getEmail());
            MacroPurchase.handleThankYouPage(driver);
        }
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
