package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.store.ReviewOrderPage;
import com.ea.originx.automation.lib.pageobjects.store.ThankYouPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test subscriber discount through the purchase flow
 *
 * @author cdeaconu
 */
public class OASubscriberDiscountPresentForPurchases extends EAXVxTestTemplate{
    
    @TestRail(caseId = 11098)
    @Test(groups = {"originaccess", "full_regression"})
    public void testSubscriberDiscountPresentForPurchases (ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BATTLEFIELD_TITANFALL_ULTIMATE_BUNDLE);
        
        logFlowPoint("Login as a newly registered user."); // 1
        logFlowPoint("Purchase 'Origin Access'."); // 2
        logFlowPoint("Navigate to GDP of a bundle entitlement."); // 3
        logFlowPoint("Start the checkout flow and verify that the subscriber discount is applied to the purchase."); // 4
        logFlowPoint("Proceed through the checkout flow and verify that the same percentage is applied to all offers being purchased."); // 5
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        
        // 3
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 4
        new GDPActionCTA(driver).clickBuyCTA();
        MacroPurchase.handlePaymentInfoPage(driver);
        ReviewOrderPage reviewOrderPage = new ReviewOrderPage(driver);
        reviewOrderPage.waitForPageToLoad();
        boolean isOriginAccessDiscountVisible = reviewOrderPage.verifyOriginAccessDiscountIsVisible();
        boolean isOriginalPriceStrikedThrough = reviewOrderPage.verifyOriginalPriceIsStrikedThrough();
        boolean isPriceAfterDiscountVisible = reviewOrderPage.verifyPriceAfterDiscountDisplayed();
        logPassFail(isOriginAccessDiscountVisible && isOriginalPriceStrikedThrough && isPriceAfterDiscountVisible, true);
        
        // 5
        String totalPriceOnReviewPage = reviewOrderPage.getPrice();
        reviewOrderPage.clickPayNow();
        ThankYouPage thankYouPage = new ThankYouPage(driver);
        thankYouPage.waitForThankYouPageToLoad();
        String totalPriceOnThankYouPage = StringHelper.formatDoubleToPriceAsString(thankYouPage.getTotalAmount());
        logPassFail(StringHelper.matchesIgnoreCase(totalPriceOnReviewPage, totalPriceOnThankYouPage), true);

        softAssertAll();
    }
}