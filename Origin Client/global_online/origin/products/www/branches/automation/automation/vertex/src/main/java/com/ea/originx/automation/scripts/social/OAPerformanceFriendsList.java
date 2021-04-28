package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.pageobjects.social.SocialHubMinimized;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the event clean up/performance impact of opening and closing the
 * friends list
 *
 * @author sbentley
 */
public class OAPerformanceFriendsList extends EAXVxTestTemplate {

    @TestRail(caseId = 160183)
    @Test(groups = {"social", "client_only", "performance"})
    public void testPerformanceFriendsList(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        int numberOfTests = 50;

        UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.MAX_FRIENDS);

        logFlowPoint("Log into Origin Client with an account that has friends");    //1
        for (int i = 0; i < numberOfTests; ++i) {
            logFlowPoint("Open Social Hub");    //2
            logFlowPoint("Minimize Social Hub");    //3
        }

        final WebDriver driver = startClientObject(context, client);

        //1
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin as " + userAccount.getUsername());
        } else {
            logFailExit("Could not log into Origin with account " + userAccount.getUsername());
        }

        SocialHub socialHub = new SocialHub(driver);
        SocialHubMinimized minimizedSocialHub = new SocialHubMinimized(driver);

        for (int i = 0; i < numberOfTests; ++i) {
            minimizedSocialHub.restoreSocialHub();

            //2
            if (socialHub.verifySocialHubVisible()) {
                logPass("Social Hub Succesfully re-opened");
            } else {
                logFailExit("Could not open Social Hub");
            }

            socialHub.clickOnSocialHubHeader();

            //3
            if (minimizedSocialHub.verifyMinimized()) {
                logPass("Successfully minimized Social Hub");
            } else {
                logFailExit("Could minimize Social Hub");
            }
        }

        softAssertAll();
    }

}
