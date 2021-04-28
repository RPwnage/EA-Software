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
 * Test info block icon and text for an entitlement that has an edition owned
 * through subscription
 *
 * @author cdeaconu
 */
public class OAInfoBlockOwnThroughSubscription extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3064039)
    @Test(groups = {"gdp", "full_regression"})
    public void testInfoBlockOwnThroughSubscription(ITestContext context) throws Exception{
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.BF4_IN_LIBRARY_THROUGH_SUBSCRIPTION);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
         
        logFlowPoint("Log into Origin with a subscriber account."); // 1
        logFlowPoint("Navigate to GDP of a released entitlement that has the edition owned through subscription."); // 2
        logFlowPoint("Verify a green check mark icon is showing."); // 3
        logFlowPoint("Verify the 'You have this game through Origin Access.' text is showing on the right of the icon."); // 4
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 3
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        boolean isInfoBlockCheckMarckIconVisible = gdpHeroActionDescriptors.verifyFirstInfoBlockIconVisible();
        boolean isInfoBlockCheckMarckIconGreen = gdpHeroActionDescriptors.verifyFirstInfoBlockIconGreen();
        logPassFail(isInfoBlockCheckMarckIconVisible && isInfoBlockCheckMarckIconGreen, false);
        
        // 4
        boolean isInfoBlockTextMatch = gdpHeroActionDescriptors.verifyFirstInfoBlockTextMatchesExpected(GDPHeroActionDescriptors.ORIGIN_ACCESS_GAME_AVAILABLE);
        boolean isInfoBlockTextRightOfIcon = gdpHeroActionDescriptors.verifyFirstInfoBlockTextIsRightOfIcon();
        logPassFail(isInfoBlockTextMatch && isInfoBlockTextRightOfIcon, false);
        
        softAssertAll();
    }
}