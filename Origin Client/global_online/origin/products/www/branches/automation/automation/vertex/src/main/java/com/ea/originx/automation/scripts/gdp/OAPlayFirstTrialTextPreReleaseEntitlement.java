package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
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
 * Test a 'Pre-Release' entitlement 'Play first trial' text is placed between
 * primary and secondary CTA
 *
 * @author cdeaconu
 */
public class OAPlayFirstTrialTextPreReleaseEntitlement extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3064037)
    @Test(groups = {"gdp", "full_regression"})
    public void testPlayFirstTrialTextPreReleaseEntitlement(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE_NO_ENTITLEMENTS);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.GDP_PRE_RELEASE_TEST_PLAY_FIRST_TRIAL);
        
        logFlowPoint("Launch Origin and login with a basic subscriber account."); // 1
        logFlowPoint("Navigate to a GDP of a pre-release entitlement."); // 2
        logFlowPoint("Verify 'Play First Trial pre-load now available' string is showing between the primary and secondary button."); // 3
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        boolean isTextBelowPrimaryCTA = gdpActionCTA.verifyPlayFirstTrialTextIsVisibleBelowPrimaryCTA();
        boolean isTextAboveSecondaryCTA = gdpActionCTA.verifyPlayFirstTrialTextIsVisibleAboveSecondaryCTA();
        logPassFail(isTextBelowPrimaryCTA && isTextAboveSecondaryCTA, true);
        
        softAssertAll();
    }
}