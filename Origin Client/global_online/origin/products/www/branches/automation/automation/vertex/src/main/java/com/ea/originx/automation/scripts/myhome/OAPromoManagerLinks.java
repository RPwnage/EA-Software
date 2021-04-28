package com.ea.originx.automation.scripts.myhome;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroCommon;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

import java.util.List;

/**
 * Test if any of words on last segment of current URL matches with any of words
 * on last segment of URL in href tag
 *
 * @author rocky
 */
public class OAPromoManagerLinks extends EAXVxTestTemplate {

    @TestRail(caseId = 1016722)
    @Test(groups = {"myhome", "services_smoke", "browser_only"})
    public void testPromoManagerLinks(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final AccountTags tag = AccountTags.THREE_ENTITLEMENTS;
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(tag);

        logFlowPoint("Launch Origin and login as user who has some friends and entitlements"); // 1
        logFlowPoint("Verify all links in home page does not return error"); // 2

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged in as " + userAccount.getUsername());
        } else {
            logFailExit("Failed to log in as " + userAccount.getUsername());
        }

        // 2
        new NavigationSidebar(driver).gotoDiscoverPage();
        List<String> brokenLinkList = MacroCommon.verifyAllHyperLinksInPage(driver);
        if (brokenLinkList.isEmpty()) {
            logPass("All link match with final URL opened");
        } else {
            logFailExit("Not all links match with final URL opened : " + brokenLinkList);
        }
        softAssertAll();
    }
}
