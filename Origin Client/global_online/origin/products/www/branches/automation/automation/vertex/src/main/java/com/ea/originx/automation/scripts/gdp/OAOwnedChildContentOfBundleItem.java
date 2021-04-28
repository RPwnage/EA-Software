package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.dialog.PurchasePreventionDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeroActionDescriptors;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.originx.automation.lib.pageobjects.store.ReviewOrderPage;
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
 * Test a purchase warning dialog appears for an owned child content of a Bundle
 * entitlement
 *
 * @author cdeaconu
 */
public class OAOwnedChildContentOfBundleItem extends EAXVxTestTemplate{
    
    @TestRail(caseId = 11912)
    @Test(groups = {"gdp", "full_regression"})
    public void testOwnedChildContentOfBundleItem(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo firstEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.TITANFALL_2_ULTIMATE_EDITION);
        final EntitlementInfo secondEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BATTLEFIELD_1_REVOLUTION);
        final EntitlementInfo bundleEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BATTLEFIELD_TITANFALL_ULTIMATE_BUNDLE);
        final String firstEntitlementName = firstEntitlement.getName();
        final String secondEntitlementName = secondEntitlement.getName();
        
        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Navigate to a GDP of an entitlement that is included in a 'Bundle Offer' and purchase it."); // 2
        logFlowPoint("Navigate to the 'Bundle Offer' GDP (which includes the purchased product)."); // 3
        logFlowPoint("Click on the 'Buy Now' CTA and verify a warning modal appears."); // 4
        logFlowPoint("Verify that the warning modal title specifies the user already owns an item in the 'Bundle Offer'."); // 5
        logFlowPoint("Verify the user can close the warning modal and cancel the purchase flow."); // 6
        logFlowPoint("Verify the user can continue and complete the purchase flow."); // 7
        logFlowPoint("Complete purchase and verify that the user is entitled to all products within the Bundle Offer (including the individual product)."); // 8
        logFlowPoint("Verify the user owns the 'Bundle Offert'."); // 9
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroPurchase.purchaseEntitlement(driver, firstEntitlement), true);
        
        // 3
        logPassFail(MacroGDP.loadGdpPage(driver, bundleEntitlement), true);
        
        // 4
        new GDPActionCTA(driver).clickBuyCTA();
        PurchasePreventionDialog purchasePreventionDialog = new PurchasePreventionDialog(driver);
        logPassFail(purchasePreventionDialog.waitIsVisible(), true);
        
        // 5
        logPassFail(purchasePreventionDialog.verifyTitleContainBundleItem(), true);
        
        // 6
        logPassFail(purchasePreventionDialog.verifyCloseButtonVisible(), true);
        
        // 7
        logPassFail(purchasePreventionDialog.verifyPurchaseAnywayButtonVisible(), true);
        
        // 8
        purchasePreventionDialog.clickPurchaseAnywayCTA();
        MacroPurchase.completePurchase(driver);
        ReviewOrderPage reviewOrderPage = new ReviewOrderPage(driver);
        boolean isFirstEntitlementVisible = reviewOrderPage.verifyEntitlementExistsForReviewingOrder(firstEntitlementName);
        boolean isSecondEntitlementVisible = reviewOrderPage.verifyEntitlementExistsForReviewingOrder(secondEntitlementName);
        logPassFail(isFirstEntitlementVisible && isSecondEntitlementVisible, true);
        MacroPurchase.handleThankYouPage(driver);
        
        // 9
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockTextMatchesExpected(GDPHeroActionDescriptors.FIRST_INFO_BLOCK_OWNED_CONTENT), true);
        
        softAssertAll();
    }
}