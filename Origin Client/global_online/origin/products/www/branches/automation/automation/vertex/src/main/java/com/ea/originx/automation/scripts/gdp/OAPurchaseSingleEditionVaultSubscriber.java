package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.dialog.VaultPurchaseWarning;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
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
 * Test purchase a single edition vault entitlement with a subscriber account
 *
 * @author cdeaconu
 */
public class OAPurchaseSingleEditionVaultSubscriber extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3002229)
    @Test(groups = {"gdp", "full_regression", "release_smoke"})
    public void testPurchaseSingleEditionVaultSubscriber(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.MINI_METRO);
        final String entitlementName = entitlement.getName();
        
        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Purchase Origin Access."); // 2
        logFlowPoint("Navigate to GDP of a single edition vault entitlement."); // 3
        logFlowPoint("Click the drop-down on the primary button and select 'Buy with <price>' option."); // 4
        logFlowPoint("Complete the purchase flow and close the thank you modal."); // 5
        logFlowPoint("Verify the primary button changes to 'View in Library'."); // 6
        logFlowPoint("Click on the 'View in Library' primary button and verify the page is navigated to 'Game Library'."); // 7
        logFlowPoint("Verify the purchased game is now in 'Game Library'."); // 8
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        if (OSInfo.getEnvironment().equalsIgnoreCase("production")) {
            new MainMenu(driver).selectRedeemProductCode();
            logPassFail(MacroRedeem.redeemOABasicSubscription(driver, userAccount.getEmail()), true );
        } else {
            logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        }
        
        // 3
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 4
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        gdpActionCTA.selectDropdownBuy();
        new VaultPurchaseWarning(driver).clickPurchaseAnywaysCTA();
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        logPassFail(paymentInformationPage.verifyPaymentInformationReached(), true);
        
        // 5
        if (OSInfo.getEnvironment().equalsIgnoreCase("production")) {
            paymentInformationPage.switchToPaymentByPaypalAndSkipToReviewOrderPage();
            MacroPurchase.completePurchaseByPaypalOffCode(driver, entitlement.getName(), userAccount.getEmail());
            MacroPurchase.handleThankYouPage(driver);
        } else {
            logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);
        }
        
        // 6
        logPassFail(gdpActionCTA.verifyViewInLibraryCTAVisible(), true);
        
        // 7
        gdpActionCTA.clickViewInLibraryCTA();
        GameLibrary gameLibrary = new GameLibrary(driver);
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);
        
        // 8
        logPassFail(gameLibrary.isGameTileVisibleByName(entitlementName), true);
        
        softAssertAll();
    }
}
