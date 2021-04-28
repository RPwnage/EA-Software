package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
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
 * Test clicking 'Buy from price text' drop-down option for a game thats owned
 * through subscription redirect the user to 'Offer Selection Page'
 *
 * @author cdeaconu
 */
public class OAPrimaryDropDownBuySubscriber extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3129012)
    @Test(groups = {"gdp", "full_regression"})
    public void testPrimaryDropDownBuySubscriber(ITestContext context) throws Exception{
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.BF4_IN_LIBRARY_THROUGH_SUBSCRIPTION);
        
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
         
        logFlowPoint("Log into Origin with a subscriber account."); // 1
        logFlowPoint("Navigate to GDP of a released entitlement that has the lesser edition owned through subscription."); // 2
        logFlowPoint("Click on the drop-down section and verify 'Buy from <price text>' option is showing in the drop-down menu."); // 3
        logFlowPoint("Click on the 'Buy from <price text>' option and verify the page redirects to 'Offer Selection Page'."); // 4
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 3
        GDPActionCTA gDPActionCTA = new GDPActionCTA(driver);
        logPassFail(gDPActionCTA.verifyDropdownBuyNowVisible(), true);
        
        // 4
        gDPActionCTA.selectDropdownBuy();
        logPassFail(new OfferSelectionPage(driver).verifyOfferSelectionPageReached(), true);
        
        softAssertAll();
    }
}