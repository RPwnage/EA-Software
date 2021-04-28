package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test 'View all editions' option is showing in the drop-down for an account
 * that owns the highest edition through subscription
 *
 * @author cdeaconu
 */
public class OAViewAllEditions extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3064028)
    @Test(groups = {"gdp", "full_regression"})
    public void testViewAllEditions(ITestContext context) throws Exception{
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount  = AccountManagerHelper.getEntitledUserAccountWithCountry("Canada", EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM));
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_DIGITAL_DELUXE);

        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Navigate to GDP of a released entitlement."); // 2
        logFlowPoint("Verify 'View all editions' option is showing in the drop-down menu."); // 3
        logFlowPoint("Click on the 'View all editions' option and verify the page redirects to the 'Offer Selection Page'."); //4
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyViewAllEditionsVisible(), true);
        
        // 4
        gdpActionCTA.selectDropdownViewAllEditions();
        logPassFail(new OfferSelectionPage(driver).verifyOfferSelectionPageReached(), true);
        softAssertAll();
    }
}