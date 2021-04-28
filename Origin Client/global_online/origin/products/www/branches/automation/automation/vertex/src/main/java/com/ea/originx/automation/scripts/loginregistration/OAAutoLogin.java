package com.ea.originx.automation.scripts.loginregistration;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests user is auto logged in when the 'Remember Me' is checked.
 *
 * @author nvarthakavi
 */
public class OAAutoLogin extends EAXVxTestTemplate {

    @TestRail(caseId = 9473)
    @Test(groups = {"loginregistration", "full_regression", "long_smoke", "client_only"})
    public void testAutoLogin(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();
        String username = userAccount.getUsername();
        String password = userAccount.getPassword();

        logFlowPoint("Launch Origin Client and click the 'Remember Me' checkbox"); //1
        logFlowPoint("Login to any random user and verify the user is successfully signed in"); //2
        logFlowPoint("Exit the Origin client and re-start the Origin client"); //3
        logFlowPoint("Verify the user is immediately signed in and brought to the main page"); //4
        logFlowPoint("Sign out of the client and verify the user is not immediately signed in again"); //5
        logFlowPoint("Verify 'Remember me' is no longer marked"); //6

        //1
        WebDriver driver = startClientObject(context, client);
        boolean isOriginLaunched = ProcessUtil.isOriginRunning(client);
        LoginPage loginPage = new LoginPage(driver);
        loginPage.setRememberMe(true, false);
        boolean isRememberMeChecked = loginPage.isRememberMeChecked();
        if (isOriginLaunched && isRememberMeChecked) {
            logPass("Successfully launched the Origin client and marked the 'remember me' checkbox.");
        } else {
            logFailExit("Failed to launch the Origin client or the 'remember me' checkbox was not checked.");
        }

        //2
        loginPage.quickLogin(username, password);
        if (MacroLogin.verifyMiniProfileUsername(driver, username)) {
            logPass(String.format("Successfully signed in with username: %s", username));
        } else {
            logFailExit(String.format("Failed to sign in with username: %s", username));
        }

        //3
        client.stop();
        boolean isExitedClient = !ProcessUtil.isOriginRunning(client);
        driver = startClientObject(context, client);
        boolean isRestartedSuccessfully = ProcessUtil.isOriginRunning(client);
        if (isExitedClient && isRestartedSuccessfully) {
            logPass("Successfully exited the client and restarted the client.");
        } else {
            logFailExit("Failed to exit and restrat the client. ");
        }

        //4
        new NavigationSidebar(driver).gotoDiscoverPage();
        boolean isDiscoverPage = new DiscoverPage(driver).verifyDiscoverPageReached();
        boolean isCorrectUser = new MiniProfile(driver).verifyUser(username);
        if (isCorrectUser && isDiscoverPage) {
            logPass("The correct user (" + username + ") was automatically signed in to the main page.");
        } else {
            logFailExit("Failed to login to expected user or the user was not directed to the main page");
        }

        //5
        loginPage = new MainMenu(driver).selectLogOut();
        if (loginPage.verifyLoginPageVisible()) {
            logPass("Successfully signed out and verified the user was not automatically signed in.");
        } else {
            logFailExit("Failed to sign out of the client or the user was automatically signed in.");
        }

        //6
        if (!loginPage.isRememberMeChecked()) {
            logPass("The 'remember me' checkbox is no longer marked");
        } else {
            logFailExit("The 'remember me' checkbox is still marked");
        }

        ProcessUtil.killOriginProcess(client);
        //Whenever a test ends in the login page, there is a clean up error happens while quitting the driver.
        // So always we need to end the client process and then start the clean up process to clean the rest of the drivers. 

        softAssertAll();
    }
}
