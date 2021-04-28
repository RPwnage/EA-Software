package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeroActionDescriptors;
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
 * Test an un-released entitlement has an info block with a release date text
 *
 * @author cdeaconu
 */
public class OAUnreleasedEntitlementInfoblock extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3064034)
    @Test(groups = {"gdp", "full_regression"})
    public void testUnreleasedEntitlementInfoblock(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getRandomAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.ANTHEM);
        
        logFlowPoint("Log into Origin."); // 1
        logFlowPoint("Navigate to GDP of an un-released entitlement that has a release date."); // 2
        logFlowPoint("Verify an info icon is showing."); // 3
        logFlowPoint("Verify 'Release date: <date text>' text is showing on the right of the info icon."); // 4
        
         // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 3
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockIconVisible(), true);
        
        // 4
        boolean isInfoBlockTextRightOfIcon = gdpHeroActionDescriptors.verifyFirstInfoBlockTextIsRightOfIcon();
        boolean isInfoBlockContainingKeywords = gdpHeroActionDescriptors.verifyFirstInfoBlockTextContainsKeywords(gdpHeroActionDescriptors.UNRELEASE_ENTITLEMENT_INFO_BLOCK_KEYWORDS);
        logPassFail(isInfoBlockTextRightOfIcon && isInfoBlockContainingKeywords, true);
     
        softAssertAll();
    }
}