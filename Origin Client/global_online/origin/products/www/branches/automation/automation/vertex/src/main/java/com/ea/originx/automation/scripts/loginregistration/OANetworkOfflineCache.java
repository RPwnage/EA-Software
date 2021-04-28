package com.ea.originx.automation.scripts.loginregistration;

import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FirewallUtil;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.login.LoginOptions;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that users are able to login while offline iff the local cache exists
 *
 * @author jmittertreiner
 */
public class OANetworkOfflineCache extends EAXVxTestTemplate {

    @TestRail(caseId = 11551)
    @Test(groups = {"loginregistration", "client_only", "full_regression"})
    public void testNetworkOfflineCache(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Disconnect Origin from the internet and delete the login cache (" + TestScriptHelper.getOfflineCachePath(client) + ")"); // 1
        logFlowPoint("Start Origin and verify that you cannot login"); // 2
        logFlowPoint("Reconnect to the internet and login as user " + userAccount); // 3
        logFlowPoint("Verify that the cache (" + TestScriptHelper.getOfflineCachePath(client) + ") is regenerated"); // 4
        logFlowPoint("Logout and disconnect from the internet"); // 5
        logFlowPoint("Verify that you can login as " + userAccount); // 6

        // The client won't launch for the first time without internet,
        // so launch it, then immediately kill it for the script
        WebDriver driver = startClientObject(context, client);
        LoginPage loginPage = new LoginPage(driver);
        Waits.pollingWait(loginPage::isOnLoginPage);
        loginPage.close();
        client.stop();

        // 1
        // No need to delete the cache here since it is done during test preparation
        if (FirewallUtil.blockOrigin(client)) {
            logPass("Successfully blocked Origin from the network and delete "
                    + TestScriptHelper.getOfflineCachePath(client));
        } else {
            logFailExit("Failed to block Origin from the network");
        }

        driver = startClientObject(context, client);

        // 2
        loginPage = new LoginPage(driver);
        new LoginPage(driver).quickLogin(userAccount.getUsername(), userAccount.getPassword(), new LoginOptions().setNoWaitForDiscoverPage(true));
        if (loginPage.verifyOfflineErrorVisible()) {
            logPass("Verified that you cannot login to Origin");
        } else {
            logFail("The user is able to login to Origin but they should not be able to");
        }

        // 3
        FirewallUtil.allowOrigin(client);
        loginPage.close();

        client.stop();
        driver = startClientObject(context, client);
        loginPage = new LoginPage(driver);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully re-enabled the network and logged in");
        } else {
            logFailExit("Failed to re-enable the network and log in");
        }

        // 4
        if (TestScriptHelper.verifyOfflineCacheExists(client)) {
            logPass(TestScriptHelper.getOfflineCachePath(client) + " was successfully regenerated");
        } else {
            logFailExit("Cache was not regenerated");
        }

        // 5
        new MainMenu(driver).selectLogOut();
        Waits.pollingWait(loginPage::isOnLoginPage, 10000, 1000, 0);
        loginPage.close();
        client.stop();
        FirewallUtil.blockOrigin(client);
        driver = startClientObject(context, client);
        loginPage = new LoginPage(driver);
        Waits.pollingWait(loginPage::isOnLoginPage, 10000, 1000, 0);
        if (Waits.pollingWait(loginPage::isSiteStripeMessageVisible, 60000, 1000, 0)) {
            logPass("Successfully logged out and disconnected from the internet");
        } else {
            logFailExit("Failed to log out and disconnect from the internet");
        }

        // 6
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully was able to log in after disabling the internet");
        } else {
            logFailExit("Was able to log in after disabling the internet");
        }

        client.stop();
        softAssertAll();
    }
}
