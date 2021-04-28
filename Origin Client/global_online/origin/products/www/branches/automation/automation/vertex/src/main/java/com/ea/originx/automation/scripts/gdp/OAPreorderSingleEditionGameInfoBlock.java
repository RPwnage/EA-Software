package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeroActionDescriptors;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
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
 * Test a pre-ordered entitlement has an info block with a check marck and a
 * 'pre-order game' text
 *
 * @author cdeaconu
 */
public class OAPreorderSingleEditionGameInfoBlock extends EAXVxTestTemplate {
    
    @TestRail(caseId = 3064034)
    @Test(groups = {"gdp", "full_regression"})
    public void testUnreleasedEntitlementInfoblock(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.GDP_PRE_RELEASE_SINGLE_EDITION);
        
        logFlowPoint("Log into Origin."); // 1
        logFlowPoint("Navigate to the GDP of an unreleased single edition game available for pre-order."); // 2
        logFlowPoint("Click 'Preorder' button and complete the purchase flow."); // 3
        logFlowPoint("Verify a green check mark icon is showing"); // 4
        logFlowPoint("Verify 'You pre-ordered the <edition text> of this game.' text is showing on the right of the green check mark icon."); // 5
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 3
        new GDPActionCTA(driver).clickPreorderCTA();
        AccessInterstitialPage accessInterstitialPage = new AccessInterstitialPage(driver);
        accessInterstitialPage.verifyOSPInterstitialPageReached();
        accessInterstitialPage.clickPreOrderOSPCTA();
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        offerSelectionPage.verifyOfferSelectionPageReached();
        offerSelectionPage.clickPrimaryButton(entitlement.getOcdPath());
        logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);
        
        // 4
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockIconGreen(), true);
        
        // 5
        boolean isInfoBlockTextRightOfIcon = gdpHeroActionDescriptors.verifyFirstInfoBlockTextIsRightOfIcon();
        boolean isInfoBlockContainingKeywords = gdpHeroActionDescriptors.verifyFirstInfoBlockTextContainsKeywords(GDPHeroActionDescriptors.PREORDER_INFO_BLOCK_KEYWORDS);
        logPassFail(isInfoBlockTextRightOfIcon && isInfoBlockContainingKeywords, true);
        
        softAssertAll();
    }
}