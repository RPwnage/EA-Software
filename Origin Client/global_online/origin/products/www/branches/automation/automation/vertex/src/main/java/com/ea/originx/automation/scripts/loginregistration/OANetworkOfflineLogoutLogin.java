package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FirewallUtil;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroNetworkOffline;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that Origin can successfully logout while disconnected from the network
 * and log back in once reconnected
 *
 * @author JMittertreiner
 */
public class OANetworkOfflineLogoutLogin extends EAXVxTestTemplate {

    @TestRail(caseId = 39466)
    @Test(groups = {"loginregistration", "client_only", "full_regression"})
    public void testNetworkOfflineLogoutLogin(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Log into Origin with " + userAccount.getUsername()); //1
        logFlowPoint("Disconnect origin from the network"); //2
        logFlowPoint("Log out of Origin"); //3
        logFlowPoint("Reconnect Origin to the internet"); //4
        logFlowPoint("Verify the user can log into origin"); //5
        logFlowPoint("Verify Origin is in online mode"); //6

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with " + userAccount.getUsername());
        } else {
            logFailExit("Could not log into Origin with " + userAccount.getUsername());
        }

        // 2
        if (MacroNetworkOffline.enterOriginNetworkOffline(driver)) {
            logPass("Successfully disconnected Origin from the network");
        } else {
            logFailExit("Could not disconnect Origin from the network");
        }

        // Logging out too soon from the client sends you to an error page on the
        // the login screen. We'll sleep a bit here before continuing
        // See ORPLAT-8262
        sleep(15000);

        // 3
        if (new MainMenu(driver).selectLogOut() != null) {
            logPass("Successfully logged out of Origin");
        } else {
            logFailExit("Could not log out of Origin");
        }

        // 4
        if (FirewallUtil.allowOrigin(client)) {
            logPass("Successfully reconnected Origin to the network");
        } else {
            logFailExit("Could not reconnect Origin to the network");
        }

        // 5
        // As Origin is recovering from offline, use networkOfflineClientLogin to prevent timeouts
        if (MacroLogin.networkOfflineClientLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with " + userAccount.getUsername());
        } else {
            logFailExit("Could not log into Origin with " + userAccount.getUsername());
        }

        // 6
        MainMenu mainMenu = new MainMenu(driver);
        boolean originNetworkOnline = Waits.pollingWait(() -> !mainMenu.verifyOfflineModeButtonText(), 30000, 1000, 5000);
        if (originNetworkOnline) {
            logPass("Verified client is online after re-login");
        } else {
            logFailExit("Client is not online after re-login");
        }

        client.stop();
        softAssertAll();
    }
}
