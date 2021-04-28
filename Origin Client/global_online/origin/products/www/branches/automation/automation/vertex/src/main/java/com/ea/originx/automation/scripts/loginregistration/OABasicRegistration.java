package com.ea.originx.automation.scripts.loginregistration;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test for basic registration of adult and underage accounts.
 *
 * @author glivingstone
 */
public class OABasicRegistration extends EAXVxTestTemplate {

    @TestRail(caseId = 3121265)
    @Test(groups = {"loginregistration", "long_smoke", "loginregistration_smoke"})
    public void testBasicRegistration(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        boolean isClient = ContextHelper.isOriginClientTesing(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        sleep(1); // To make the two accounts have different usernames.
        UserAccount underageAccount = AccountManagerHelper.createNewThrowAwayAccount(12);

        logFlowPoint("Register and Login as User: " + userAccount.getUsername()); //1
        if (isClient) {
            logFlowPoint("Log out of Origin"); //2
            logFlowPoint("Register and Login as User: " + underageAccount.getUsername()); //3
        }

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login to Origin client successful as user: " + userAccount.getUsername());
        } else {
            logFailExit("Failed: Cannot launch Origin client or login as user: " + userAccount.getUsername());
        }

        //2
        if (isClient) {
            new MainMenu(driver).selectExit();
            if (Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, "Origin.exe"))) {
                logPass("Successfully exited Origin.");
            } else {
                logFailExit("Could not exit Origin.");
            }

            //3
            client.stop();
            driver = startClientObject(context, client);
            if (MacroLogin.startLogin(driver, underageAccount)) {
                logPass("Verified login to Origin client successful as user: " + underageAccount.getUsername());
            } else {
                logFailExit("Failed: Cannot launch Origin client or login as user: " + underageAccount.getUsername());
            }
        }

        softAssertAll();
    }
}
