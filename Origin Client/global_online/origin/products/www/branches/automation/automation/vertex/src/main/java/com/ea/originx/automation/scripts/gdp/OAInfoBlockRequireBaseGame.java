package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
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
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test Info Block Base Game
 * 
 * @author mdobre
 */
public class OAInfoBlockRequireBaseGame extends EAXVxTestTemplate {
    
    @TestRail(caseId = 3064042)
    @Test(groups = {"gdp", "full_regression"})
    public void testInfoBlockRequireBaseGame(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_18_CURRENCY_PACK);
        final EntitlementInfo baseEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_18);
        final UserAccount userAccount = AccountManager.getRandomAccount();
        
        logFlowPoint("Log into Origin with an existing user."); // 1
        logFlowPoint("Navigate to the GDP of the entitlement."); //2
        logFlowPoint("Verify an info icon is showing."); //3
        logFlowPoint("Verify the 'To use this, you mus have the game: <base game text>.' text is showing on the right of the icon."); //4
        logFlowPoint("Click on the base game link and verify the page redirects to the base game's Game Detail page."); //5
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        //2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        //3
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockIconVisible(), false);
        
        //4
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockTextVisible(), true);
        
        //5
        gdpHeroActionDescriptors.clickFirstInfoBlockLink();
        sleep(2000);
        GDPHeader gDPHeader = new GDPHeader(driver);
        Waits.pollingWait(() -> gDPHeader.verifyGDPHeaderReached());
        logPassFail(gDPHeader.verifyGameTitle(baseEntitlement.getName()), true);
        
        softAssertAll();
    }
}