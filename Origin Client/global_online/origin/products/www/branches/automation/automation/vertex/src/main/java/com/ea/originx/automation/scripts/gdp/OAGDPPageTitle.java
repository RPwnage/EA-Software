package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
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
 * Tests a GDP page's default page title
 *
 * @author ivlim
 * @author cbalea
 */
public class OAGDPPageTitle extends EAXVxTestTemplate {

    @TestRail(caseId = 11727)
    @Test(groups = {"gdp", "full_regression"})
    public void testGDPPageTitle(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getRandomAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BATTLEFIELD_1_STANDARD);

        logFlowPoint("Log into Origin with random account"); // 1
        logFlowPoint("Navigate to the GDP for any game"); // 2
        logFlowPoint("Veriy GDP page have 'Game name', 'Platform' and 'Origin' as the default page title"); // 3

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 3
        logPassFail(driver.getTitle().contains(MacroGDP.getExpectedGDPTitle(entitlement)), true);

        softAssertAll();
    }
}
