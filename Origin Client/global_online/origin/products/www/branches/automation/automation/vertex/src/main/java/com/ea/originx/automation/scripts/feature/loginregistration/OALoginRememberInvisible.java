package com.ea.originx.automation.scripts.feature.loginregistration;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.login.LoginOptions;
import com.ea.originx.automation.lib.pageobjects.login.SecurityQuestionAnswerPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.pageobjects.social.SocialHubMinimized;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test logging in with 'Keep me signed in' and 'Sign in as invisible'
 *
 * @author glivingstone
 */
public class OALoginRememberInvisible extends EAXVxTestTemplate {

    @TestRail(caseId = 3096149)
    @Test(groups = {"loginregistration", "loginregistration_smoke", "client_only", "allfeaturesmoke"})
    public void testLoginRememberInvisible(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Launch the Origin Client"); //1
        logFlowPoint("Sign into Origin with the 'Keep me signed in' and 'Sign in as invisible' Boxes Checked"); //2
        logFlowPoint("Verify the user is Invisible"); //3
        logFlowPoint("Exit the Origin client"); //4
        logFlowPoint("Start the Origin Client and Verify the User is Immediately Signed in"); //5

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (driver != null) {
            logPass("Successfully launched Origin");
        } else {
            logFail("Failed to launch Origin");
        }

        // 2
        if (MacroLogin.startLogin(driver, userAccount, new LoginOptions().setRememberMe(true).setInvisible(true), "", "", "", false, SecurityQuestionAnswerPage.DEFAULT_PRIVACY_VISIBLITY_SETTING)) {
            logPass("Successfully logged into Origin");
        } else {
            logFailExit("Could not log into Origin");
        }

        // 3
        // Check both, just in case social hub is opened all the way
        boolean socialHubInvisible;
        SocialHubMinimized socialHubMinimized = new SocialHubMinimized(driver);
        if (socialHubMinimized.verifyMinimized()) {
            socialHubInvisible = socialHubMinimized.isUserInvisible();
        } else {
            socialHubInvisible = new SocialHub(driver).verifyUserInvisible();
        }
        if (socialHubInvisible) {
            logPass("User is set to invisible when logging in as invisible.");
        } else {
            logFail("User is not set to invisible, despite logging in as invisible.");
        }

        // 4
        new MainMenu(driver).selectExit();
        client.stop();
        if (Waits.pollingWaitEx(() -> !ProcessUtil.isOriginRunning(client))) {
            logPass("Successfully exited Origin.");
        } else {
            logFailExit("Could not exit Origin");
        }

        // 5
        client.stop();
        final WebDriver driver2 = startClientObject(context, client);
        MiniProfile miniProfile = new MiniProfile(driver2);
        if (Waits.pollingWait(() -> miniProfile.verifyUser(userAccount.getUsername()))) {
            logPass("Automatic login to the client was successful.");
        } else {
            logFailExit("Automation login to the client failed.");
        }

        client.stop();
        softAssertAll();
    }
}
