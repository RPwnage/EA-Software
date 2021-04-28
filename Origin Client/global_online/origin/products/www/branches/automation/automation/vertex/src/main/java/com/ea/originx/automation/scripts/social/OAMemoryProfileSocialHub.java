package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.pageobjects.social.SocialHubMinimized;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Memory profiling test that repeatedly expands and collapses the 'Social Hub' window
 * in order to expose memory leaks
 *
 * @author jdickens
 */

public class OAMemoryProfileSocialHub extends EAXVxTestTemplate {

    @Test(groups = {"social", "memory_profile"})
    public void testPerformanceSocialHub(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount user = AccountManagerHelper.getTaggedUserAccount(AccountTags.X_PERFORMANCE);
        user.cleanFriends();
        UserAccount friendAccountA = AccountManager.getRandomAccount();
        UserAccount friendAccountB = AccountManager.getRandomAccount();
        UserAccountHelper.addFriends(user, friendAccountA, friendAccountB);

        logFlowPoint("Login to Origin with a performance account"); // 1
        for (int i = 0; i < 15; i++) {
            logFlowPoint("Expand and collapse the 'Social Hub' and then perform garbage collection"); // 2 - 16
        }

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin with user " + user.getUsername());
        } else {
            logFailExit("Failed to log into Origin with the user " + user.getUsername());
        }

        // 2 - 16
        boolean isSocialHubVisible = false;
        boolean isSocialHubCollapsed = false;
        boolean isGarbageCollectionSuccessful = false;
        for (int i = 0; i < 15; i++) {
            // Open/Restore the 'Social Hub' window
            SocialHub socialHub = new SocialHub(driver);
            SocialHubMinimized socialHubMinimized = new SocialHubMinimized(driver);
            socialHubMinimized.restoreSocialHub();
            isSocialHubVisible = socialHub.verifySocialHubVisible();

            // Collapse the 'Social Hub' window
            socialHub.clickMinimizeSocialHub();
            isSocialHubCollapsed = !socialHub.verifySocialHubVisible();

            // Perform garbage collection
            isGarbageCollectionSuccessful = garbageCollect(driver, context);

            if (isSocialHubVisible && isSocialHubCollapsed && isGarbageCollectionSuccessful) {
                logPass("Successfully expanded and collapsed the 'Social Hub' and then performed garbage collection");
            } else {
                logFail("Failed to expand and/or collapse the 'Social Hub' window, or perform garbage collection");
            }
        }

        softAssertAll();
    }
}