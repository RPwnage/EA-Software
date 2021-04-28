package com.ea.originx.automation.scripts.client;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to verify Origin behavior when client killed
 *
 * @author palui
 */
public class OAKillOrigin extends EAXVxTestTemplate {

    @TestRail(caseId = 9468)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testKillOrigin(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        String originProcess = OriginClientData.ORIGIN_PROCESS_NAME;
        UserAccount userAccount = AccountManager.getRandomAccount();
        String username = userAccount.getUsername();

        logFlowPoint("Launch Origin and login as user: " + username); //1
        logFlowPoint("Kill Origin with Windows taskkill"); //2
        logFlowPoint("Re-launch Origin and re-login as user: " + username); //3

        //1
        WebDriver driver = startClientObject(context, client);
        boolean loginOK = MacroLogin.startLogin(driver, userAccount);
        boolean originLaunchOK = ProcessUtil.isProcessRunning(client, originProcess);
        if (loginOK && originLaunchOK) {
            logPass("Verified Origin launches and login successful as user: " + username);
        } else {
            logFailExit("Failed: Cannot launch Origin or login as user: " + username);
        }

        //2
        client.stop();
        if (ProcessUtil.killProcess(client, originProcess)) {
            logPass("Verified Origin has been killed using Windows taskkill");
        } else {
            logFailExit("Failed: Cannot kill Origin using Windows taskkill");
        }

        //3
        driver = startClientObject(context, client);
        loginOK = MacroLogin.startLogin(driver, userAccount);
        originLaunchOK = ProcessUtil.isProcessRunning(client, originProcess);
        if (loginOK && originLaunchOK) {
            logPass("Verified Origin re-launches and re-login successful as user: " + username);
        } else {
            logFailExit("Failed: Cannot re-launch Origin or re-login as user: " + username);
        }

        softAssertAll();
    }
}
