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
 * Tests an un-owned released entitlement has a message indicating the
 * entitlement is available with premier
 *
 * @author cbalea
 */
public class OAInfoBlockIncludedPremier extends EAXVxTestTemplate {

    @TestRail(caseId = 3486233)
    @Test(groups = {"gdp", "full_regression"})
    public void testInfoBlockIncludedPremier(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.GPD_PREMIER_PRE_RELEASE_GAME01_STANDARD);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.PREMIER_SUBSCRIPTION_ACTIVE_NO_ENTITLEMENTS);

        logFlowPoint("Log into Origin as a premier subscriber"); // 1
        logFlowPoint("Navigate to the GDP of an entitlement which is not owned"); // 2
        logFlowPoint("Verify an included in premier membership infoblock is showing"); // 3
        logFlowPoint("Verify there is a green check mark on the left of the infoblock text"); // 4

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 3
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockTextContainsKeywords(GDPHeroActionDescriptors.INFO_BLOCK_PREMIER_KEYWORDS), true);

        // 4
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockIconVisible(), true);

        softAssertAll();
    }
}