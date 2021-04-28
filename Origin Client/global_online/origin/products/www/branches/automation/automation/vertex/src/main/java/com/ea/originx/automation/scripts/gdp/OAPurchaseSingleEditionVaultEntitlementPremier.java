package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.dialog.VaultPurchaseWarning;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase of a single edition vault entitlement with a basic subscriber account
 *
 * @author mdobre
 */
public class OAPurchaseSingleEditionVaultEntitlementPremier extends EAXVxTestTemplate {

    @TestRail(caseId = 3548526)
    @Test(groups = {"originaccess", "full_regression"})
    public void testPurchaseSingleEditionVaultEntitlementPremier(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.MINI_METRO);

        logFlowPoint("Launch and log into 'Origin' with a newly registered user."); // 1
        logFlowPoint("Purchase 'Origin Basic Access'."); //2
        logFlowPoint("Navigate to the GDP of a released vault game with a single edition."); // 3
        logFlowPoint("Click on the primary CTA drop-down and verify a 'Buy...' option is showing."); // 4
        logFlowPoint("Click on the 'Buy...' option and verify vault purchase warning modal appears."); // 5
        logFlowPoint("Click on 'Purchase anyways' CTA and verify the checkout flow starts."); // 6
        logFlowPoint("Complete the checkout flow and verify the 'Thank You' modal appears."); // 7
        logFlowPoint("Close the 'Thank You' modal and verify 'View in Library' CTA is showing."); // 8
        logFlowPoint("Click on the 'View in Library' CTA and verify the page redirects to the 'Game Library' and opens the OGD."); // 9
        logFlowPoint("Verify the purchased game is now in the 'Game Library'."); //10

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);

        //3
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        //4
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyDropdownBuyNowVisible(), true);

        //5
        gdpActionCTA.selectDropdownBuy();
        VaultPurchaseWarning vaultPurchaseWarning = new VaultPurchaseWarning(driver);
        logPassFail(vaultPurchaseWarning.waitIsVisible(), true);

        //6
        vaultPurchaseWarning.clickPurchaseAnywaysCTA();
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        logPassFail(paymentInformationPage.verifyPaymentInformationReached(), true);

        //7
        logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);

        //8
        logPassFail(gdpActionCTA.verifyViewInLibraryCTAPresent(), true);

        //9
        gdpActionCTA.clickViewInLibraryCTA();
        GameLibrary gameLibrary = new GameLibrary(driver);
        boolean isGameLibraryReached = gameLibrary.verifyGameLibraryPageReached();
        GameSlideout gameSlideout = new GameSlideout(driver);
        gameSlideout.waitForSlideout();
        boolean isOGDVisible = gameSlideout.verifyGameTitle(entitlement.getName());
        logPassFail(isGameLibraryReached && isOGDVisible, true);

        //10
        gameSlideout.clickSlideoutCloseButton();
        logPassFail(gameLibrary.isGameTileVisibleByName(entitlement.getName()), true);

        softAssertAll();
    }
}