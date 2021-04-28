package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
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
 * Test 'Play first trial' block available
 * 
 * @author mdobre
 */
public class OAInfoBlockPlayFirstTrialAvailable extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3064038)
    @Test(groups = {"gdp", "full_regression"})
    public void testInfoBlockPlayFirstTrialAvailable(ITestContext context) throws Exception{
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_18);
        final String entitlementId = entitlement.getOfferId();
        
        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Purchase Origin Access."); // 2
        logFlowPoint("Navigate to GDP of a game that has 'Play First Trial' available."); // 3
        logFlowPoint("Verify a 'Play First Trial now available' text is showing between the primary and secondary button."); //4
        
         // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        
        // 3
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 4
        logPassFail(new GDPActionCTA(driver).verifyPlayFirstTrialText(), true);
        
        softAssertAll();
    }
}