package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FirewallUtil;
import com.ea.vx.originclient.helpers.OriginClientLogReader;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to verify Origin client attempts to reconnect while it is blocked at the
 * Firewall
 * <p>
 * Refactor to match up Test Rail - C9709
 *
 * @author palui
 */
public class OANetworkOfflineReconnect extends EAXVxTestTemplate {

    @TestRail(caseId = 9709)
    @Test(groups = {"client_only", "full_regression"})
    public void testNetworkOfflineReconnect(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getRandomAccount();
        final String userName = userAccount.getUsername();

        final OriginClientLogReader logReader = new OriginClientLogReader(OriginClientData.LOG_PATH);

        logFlowPoint("Launch Origin client and login as user: " + userName); //1
        logFlowPoint("Block Origin client from the network using Windows Firewall"); //2
        logFlowPoint("Verify Origin client attempts several reconnects within 30 seconds"); //2
        logFlowPoint("Unblock Origin client at the Windows Firewall and verify it reconnects"); //4

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful as user: " + userName);
        } else {
            logFailExit("Failed: Cannot login as user: " + userName);
        }

        //2
        logReader.keyValuesSinceLastScan(client, OriginClientData.CLIENT_LOG, "connectionAttempts"); // scan Client log to skip any previous reconnect attempts
        boolean blockOriginOK = FirewallUtil.blockOriginNoOfflineMode(client);
        MacroSocial.restoreAndExpandFriends(driver);
        SocialHub socialHub = new SocialHub(driver);
        boolean presenceOffline = Waits.pollingWait(socialHub::verifyPresenceDotOffline, 30000, 1000, 10000);
        if (blockOriginOK && presenceOffline) {
            logPass("Verified Origin client has been disconnected using the firewall");
        } else {
            logFailExit("Failed: Cannot blocked Origin client from the network using the firewall");
        }

        //3
        sleep(30000); // pause for Origin to attempt to reconect a few times
        int reconnectAttempts = logReader.keyValuesSinceLastScan(client, OriginClientData.CLIENT_LOG, "connectionAttempts");
        if (reconnectAttempts >= 2) {
            logPass(String.format("Verified Origin client attempts %d times (in around 30 secs) to reconnect", reconnectAttempts));
        } else {
            logFail(String.format("Failed: Origin client attempts only %d (<2) times (in around 30 secs) to reconnect", reconnectAttempts));
        }

        //4
        boolean allowOriginOK = FirewallUtil.allowOrigin(client);
        boolean presenceOnline = Waits.pollingWait(socialHub::verifyPresenceDotOnline, 30000, 1000, 5000);
        if (allowOriginOK && presenceOnline) {
            logPass("Verified Origin client reconnects after unblocking at the firewall");
        } else {
            logFailExit("Failed: Origin client does not reconnect after unblocking at the firewall");
        }

        softAssertAll();
    }
}
