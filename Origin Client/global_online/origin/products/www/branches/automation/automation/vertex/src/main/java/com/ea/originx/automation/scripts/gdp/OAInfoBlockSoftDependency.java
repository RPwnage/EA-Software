package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
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
import static com.ea.vx.originclient.templates.OATestBase.sleep;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test first info block multiple entitlements required
 * @author mdobre
 */
public class OAInfoBlockSoftDependency extends EAXVxTestTemplate {
    
    @TestRail(caseId = 3064045)
    @Test(groups = {"gdp", "full_regression"})
    public void testInfoBlockSoftDependency(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_MY_FIRST_PET);
        final EntitlementInfo baseEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);
        final EntitlementInfo dlcEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_CATS_AND_DOGS);
        final UserAccount userAccount = AccountManager.getUnentitledUserAccount();
        
        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Navigate to the GDP of the entitlement."); //2
        logFlowPoint("Verify an info icon is showing."); //3
        logFlowPoint("Verify the 'To use this, you must have: <base game text> and <soft dependent dlc name>.' text is showing on the right of the icon."); //4
        logFlowPoint("Verify the base game name is a link."); //5
        logFlowPoint("Click on the base game link and verify the page redirects to the base game's Game Details page."); //6
        logFlowPoint("Verify the soft dependent dlc name is a link."); //7
        logFlowPoint("Click on the soft dependent dlc name and verify the page redirects to the dlc game's Game Details page"); //8
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        //2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        //3
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockIconVisible(), false);
        
        //4
        boolean isTextContainingGames = gdpHeroActionDescriptors.verifyFirstInfoBlockRequiredEntitlements(baseEntitlement.getName(), dlcEntitlement.getName());
        boolean isTextInTheRightOfTheIcon = gdpHeroActionDescriptors.verifyFirstInfoBlockTextIsRightOfIcon();
        logPassFail(isTextContainingGames && isTextInTheRightOfTheIcon, true);
        
        //5
        logPassFail(gdpHeroActionDescriptors.verifyGivenLinkFirstInfoBlockVisible(1), true);
        
        //6
        gdpHeroActionDescriptors.clickGivenLinkFirstInfoBlock(1);
        sleep(3000); // wait for GDP page of the base game to load
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(Waits.pollingWait(() -> gdpActionCTA.verifyGameTitle(baseEntitlement.getParentName())), true);
        
        //7
        // navigate back to the first entitlement GDP
        driver.navigate().back();
        logPassFail(Waits.pollingWait(() -> gdpHeroActionDescriptors.verifyGivenLinkFirstInfoBlockVisible(2)), true);
    
        //8
        gdpHeroActionDescriptors.clickGivenLinkFirstInfoBlock(2);
        sleep(3000); // wait for GDP page of the dlc game to load
        logPassFail(Waits.pollingWait(() -> gdpActionCTA.verifyGameTitle(dlcEntitlement.getParentName())), true);
        
        softAssertAll();
    }
}