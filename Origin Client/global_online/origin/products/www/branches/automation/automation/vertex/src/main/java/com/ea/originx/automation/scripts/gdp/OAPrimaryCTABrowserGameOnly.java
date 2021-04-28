package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import java.util.ArrayList;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests 'Play now' CTA for browser only game
 *
 * @author cdeaconu
 */
public class OAPrimaryCTABrowserGameOnly extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3064199)
    @Test(groups = {"gdp", "full_regression"})
    public void testPrimaryCTABrowserGameOnly(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getRandomAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.COMMAND_AND_CONQUER_TIBERIUM_ALLIANCES);
        final String browserGameURL = "www.tiberiumalliances.com";
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        
        logFlowPoint("Launch Origin and login."); // 1
        logFlowPoint("Navigate to a GDP of a browser only entitlement."); // 2
        logFlowPoint("Verify the 'Play Now' primary CTA is showing."); // 3
        logFlowPoint("Verify there is a pop-out icon in the 'Play Now' primary CTA"); // 4
        if (!isClient) {
            logFlowPoint("Click on the 'Play Now' primary CTA and verify the game page opens."); // 5
        }
       
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyPlayNowCTAVisible(), true);
        
        // 4
        logPassFail(gdpActionCTA.verifyPlayNowCtaPopOutIconVisible(), true);
        
        // 5
        gdpActionCTA.clickPlayNowCTA();
        // waits for the new tab to oppen
        Waits.sleep(2000);
        ArrayList<String> tabs = null;
        if (!isClient) {
            tabs = new ArrayList<>(driver.getWindowHandles());
            driver.switchTo().window(tabs.get(1)); // switching to newly opened tab
            logPassFail(driver.getCurrentUrl().contains(browserGameURL), true);
        }
        
        softAssertAll();
    }
}