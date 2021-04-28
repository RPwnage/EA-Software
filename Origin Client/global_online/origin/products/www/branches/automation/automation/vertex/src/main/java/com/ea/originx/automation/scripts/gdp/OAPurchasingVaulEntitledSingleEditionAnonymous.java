package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.resources.AccountTags;
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
 * Test purchasing a vault entitled single edition game through 'Access
 * Interstitial' page
 *
 * @author cdeaconu
 */
public class OAPurchasingVaulEntitledSingleEditionAnonymous extends EAXVxTestTemplate{
    
    @TestRail(caseId = 10954)
    @Test(groups = {"gdp", "full_regression" , "browser_only"})
    public void testPurchasingVaulEntitledSingleEditionAnonymous(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE);
        
        logFlowPoint("Navigate to GDP of a single edition game in vault."); // 1
        logFlowPoint("Click on the 'Get the Game' CTA on the GDP hero and verify the user navigates to the 'Origin Access Interstitial' page."); // 2
        logFlowPoint("Click on the 'Buy with Price' CTA and login using a subscriber account which owns a single edition game  through vault."); // 3
        logFlowPoint("Verify after succefully login a purchase flow overlay appears."); // 4
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
       
        // 2
        new GDPActionCTA(driver).clickGetTheGameCTA();
        AccessInterstitialPage accessInterstitialPage = new AccessInterstitialPage(driver);
        accessInterstitialPage.waitForPageToLoad();
        logPassFail(accessInterstitialPage.verifyOSPInterstitialPageReached(), true);
        
        // 3
        accessInterstitialPage.clickBuyGameCTA();
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 4
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        logPassFail(paymentInformationPage.verifyPaymentInformationReached(), true);
        
        softAssertAll();
    }
}