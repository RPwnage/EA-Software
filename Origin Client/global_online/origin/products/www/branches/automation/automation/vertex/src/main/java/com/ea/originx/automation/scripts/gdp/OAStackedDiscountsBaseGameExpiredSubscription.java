package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.dialog.YouNeedBaseGameDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.store.ReviewOrderPage;
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
 * Tests stacked discounts for base game purchase is applied for an account with
 * an expired subscription
 *
 * @author cdeaconu
 */
public class OAStackedDiscountsBaseGameExpiredSubscription extends EAXVxTestTemplate{
    
    @TestRail(caseId = 10987)
    @Test(groups = {"gdp", "full_regression"})
    public void testInfoBlockOwnThroughPurchase(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_EXPIRED);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SUPER_DUPER_SIMS_DISCOUNTED);
        final String entitlementName = entitlement.getName();
        
        logFlowPoint("Log into Origin with an account with an expired subscription."); // 1
        logFlowPoint("Go to a base game entitlement GDP."); // 2
        logFlowPoint("Start and proceed through the checkout flow."); // 3
        logFlowPoint("Verify that if the game is on sale, the offer level discount is applied first."); // 4
        logFlowPoint("Verify that the subscriber discount isn't applied when the user reaches the checkout(after the offer level discount)."); // 5
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 3
        new GDPActionCTA(driver).clickBuyCTA();
        YouNeedBaseGameDialog youNeedBaseGameDialog = new YouNeedBaseGameDialog(driver, entitlementName);
        youNeedBaseGameDialog.waitIsVisible();
        youNeedBaseGameDialog.clickContinuePurchaseButton();
        MacroPurchase.handlePaymentInfoPage(driver);
        ReviewOrderPage reviewOrderPage = new ReviewOrderPage(driver);
        logPassFail(reviewOrderPage.verifyReviewOrderPageReached(), true);
        
        // 4
        boolean isPriceAfterDiscountDisplayed = reviewOrderPage.verifyPriceAfterDiscountDisplayed();
        boolean isOriginalPriceStrikedThrough = reviewOrderPage.verifyOriginalPriceIsStrikedThrough();
        logPassFail(isPriceAfterDiscountDisplayed && isOriginalPriceStrikedThrough, true);
        
        // 5
        logPassFail(!reviewOrderPage.verifyOriginAccessDiscountIsVisible(), true);
        
        softAssertAll();
    }
}