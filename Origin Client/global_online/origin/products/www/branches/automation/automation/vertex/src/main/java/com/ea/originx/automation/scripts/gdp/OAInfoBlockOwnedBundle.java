package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeroActionDescriptors;
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
 * Test Info Block for owned bundle.
 * 
 * @author mdobre
 */
public class OAInfoBlockOwnedBundle extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3064044)
    @Test(groups = {"gdp", "full_regression"})
    public void testInfoBlockOwnedBundle(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BATTLEFIELD_TITANFALL_ULTIMATE_BUNDLE);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.ACCOUNT_WITH_BUNDLE);
        
        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Navigate to the GDP and verify a green mark icon is showing."); //2
        logFlowPoint("Verify the 'You have this content.' text is showing on the right of the icon."); //3
        
         // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        //2
        MacroGDP.loadGdpPage(driver, entitlement);
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        boolean isFirstInfoBlockIconVisible = gdpHeroActionDescriptors.verifyFirstInfoBlockIconVisible();
        boolean isFirstInfoBlockIconGreen = gdpHeroActionDescriptors.verifyFirstInfoBlockIconGreen();
        logPassFail(isFirstInfoBlockIconVisible && isFirstInfoBlockIconGreen, true);
        
        //3
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockTextMatchesExpected(GDPHeroActionDescriptors.FIRST_INFO_BLOCK_OWNED_CONTENT), true);
        
        softAssertAll();
    }
}