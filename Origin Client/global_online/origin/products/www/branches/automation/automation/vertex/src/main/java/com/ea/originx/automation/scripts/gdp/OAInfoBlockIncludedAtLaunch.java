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
 * Tests the info block for an un-released entitlement available through premier
 * *
 * @author cbalea
 */
public class OAInfoBlockIncludedAtLaunch extends EAXVxTestTemplate {

    @TestRail(caseId = 3486232)
    @Test(groups = {"gdp", "full_regression"})
    public void testInfoBlockIncludedAtLaunch(ITestContext context) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.PREMIER_SUBSCRIPTION_ACTIVE_NO_ENTITLEMENTS);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.GDP_PREMIER_PRE_RELEASE_GAME02_STANDARD);

        logFlowPoint("Log into Origin"); // 1
        logFlowPoint("Navigate to the GDP of an un-released entitlement which will be available in premier vault"); // 2
        logFlowPoint("Verify there  is a message indicating the entitlement will be included in 'Origin Access Premier'"); // 3
        logFlowPoint("Verify there is a info icon to the left of the message"); // 4

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 3
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockTextContainsKeywords(gdpHeroActionDescriptors.INFO_BLOCK_PREMIER_PRE_RELEASE_KEYWORDS), false);

        //4
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockIconVisible(), true);

        softAssertAll();
    }
}
