package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
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
 * Test Info Block Requires UPlay
 *
 * @author mdobre
 */
public class OAInfoBlockRequiresUPlay extends EAXVxTestTemplate {

    @TestRail(caseId = 3064047)
    @Test(groups = {"gdp", "full_regression"})
    public void testInfoBlockRequiresUPlay(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        boolean isClient = ContextHelper.isOriginClientTesing(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.WATCH_DOGS);
        final UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Log into Origin with an existing user."); // 1
        logFlowPoint("Navigate to the GDP of the entitlement."); //2
        logFlowPoint("Verify an info icon is showing."); //3
        logFlowPoint("Verify the 'This content requires UPlay' text is showing on the right of the icon."); //4
        logFlowPoint("Verify the 'UPlay' in the text is a link."); //5
        if (isClient) {
            logFlowPoint("Click on the base game link and verify the UPlay website opens a new window in the default browser."); //6a
        } else {
            logFlowPoint("Click on the base game link and verify the UPlay website opens a new tab."); //6b
        }
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        //3
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockIconVisible(), false);

        //4
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockTextContainsKeywords(GDPHeroActionDescriptors.FIRST_INFO_BLOCK_REQUIRED_UPLAY), true);

        //5
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockLinkVisible(), true);

        //6
        if (isClient) {
            //6a
            TestScriptHelper.killBrowsers(client);
            gdpHeroActionDescriptors.clickFirstInfoBlockLink();
            logPassFail(Waits.pollingWaitEx(() -> TestScriptHelper.verifyBrowserOpen(client)), true);
        } else { //6b
            int oldTabsCount = driver.getWindowHandles().size();
            gdpHeroActionDescriptors.clickFirstInfoBlockLink();
            int newTabsCount = driver.getWindowHandles().size();
            logPassFail(newTabsCount == oldTabsCount + 1, true);
        }

        softAssertAll();
    }
}