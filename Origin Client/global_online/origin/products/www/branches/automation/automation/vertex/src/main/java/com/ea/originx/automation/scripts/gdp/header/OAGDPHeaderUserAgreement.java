package com.ea.originx.automation.scripts.gdp.header;

import com.ea.originx.automation.lib.helpers.TestScriptHelper;
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
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;


/**
 * Test EA legal page is opens in a new tab or in the browser depending on platform
 * when clicking EA User Agreement link
 *
 * @author dchalasani
 */
public class OAGDPHeaderUserAgreement extends EAXVxTestTemplate{
    @TestRail(caseId = 3242470)
    @Test(groups = {"gdp", "full_regression"})
    public void testOAGDPHeaderUserAgreement(ITestContext context) throws Exception{

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_18);

        boolean isClient = ContextHelper.isOriginClientTesing(context);

        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Navigate to GDP of a game that is published by EA."); // 2
        logFlowPoint("Click on the EA User Agreement link and verify EA legal website opens in a " +
                "new tab(Browser) or browser(Client)"); // 3

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 3
        GDPHeader gdpHeader = new GDPHeader(driver);
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);

        if (isClient) {
            //3a
            TestScriptHelper.killBrowsers(client);
            gdpHeader.clickEAUserAgreement();
            logPassFail(Waits.pollingWaitEx(() -> TestScriptHelper.verifyBrowserOpen(client)), true);
        } else { //3b
            int oldTabsCount = driver.getWindowHandles().size();
            gdpHeader.clickEAUserAgreement();
            int newTabsCount = driver.getWindowHandles().size();
            logPassFail(newTabsCount == oldTabsCount + 1, true);
        }
        softAssertAll();
    }
}
