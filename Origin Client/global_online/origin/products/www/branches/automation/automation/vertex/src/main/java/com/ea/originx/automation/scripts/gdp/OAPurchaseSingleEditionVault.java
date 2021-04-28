package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
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

import javax.crypto.Mac;

/**
 * Test purchase a single edition vault entitlement
 *
 * @author cdeaconu
 */
public class OAPurchaseSingleEditionVault extends EAXVxTestTemplate {
    
    public enum TEST_TYPE {
        NON_SUBSCRIBER,
        ANONYMOUS_USER
    }
    
    public void testPurchaseSingleEditionVault(ITestContext context, TEST_TYPE type) throws Exception {
    
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.MINI_METRO);
        final String entitlementName = entitlement.getName();
        
        if (type == TEST_TYPE.NON_SUBSCRIBER) {
            logFlowPoint("Log into Origin as newly registered account."); // 1
        }
        logFlowPoint("Navigate to GDP of a single edition vault entitlement."); // 2
        logFlowPoint("Verify 'Get the Game' CTA is visible and click on it."); // 3
        logFlowPoint("Verify the page redirects to 'Access Interstitial' page and click 'Buy with <price>' CTA"); // 4
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logFlowPoint("Log in as a newly registered user."); // 5
        }
        logFlowPoint("Complete the purchase flow and close the thank you modal."); // 6
        logFlowPoint("Verify the primary button changes to 'View in Library'."); // 7
        logFlowPoint("Click on the 'View in Library' primary button and verify the page is navigated to 'Game Library'."); // 8
        logFlowPoint("Verify the purchased game is now in 'Game Library'."); // 9
        
        WebDriver driver = startClientObject(context, client);

        // 1
        if (type == TEST_TYPE.NON_SUBSCRIBER) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyGetTheGameCTAVisible(), true);
        gdpActionCTA.clickGetTheGameCTA();
        
        // 4
        AccessInterstitialPage accessInterstitialPage =  new AccessInterstitialPage(driver);
        logPassFail(accessInterstitialPage.verifyOSPInterstitialPageReached(), true);
        accessInterstitialPage.clickBuyGameCTA();
        
        // 5
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }
        
        // 6
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        if(type == TEST_TYPE.NON_SUBSCRIBER && OSInfo.getEnvironment().equals("production")) {
            paymentInformationPage.switchToPaymentByPaypalAndSkipToReviewOrderPage();
            MacroPurchase.completePurchaseByPaypalOffCode(driver, entitlement.getName(), userAccount.getEmail());
            MacroPurchase.handleThankYouPage(driver);
        } else {
            logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);
        }
        // 7
        logPassFail(gdpActionCTA.verifyViewInLibraryCTAVisible(), true);
        
        // 8
        gdpActionCTA.clickViewInLibraryCTA();
        GameLibrary gameLibrary = new GameLibrary(driver);
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);
        
        // 9
        logPassFail(gameLibrary.isGameTileVisibleByName(entitlementName), true);
        
        softAssertAll();
    }
}