package com.ea.originx.automation.scripts.client;

import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.resources.OriginClientData;
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
 *  Tests the items present in Origin Menu
 *
 * @author cbalea
 */
public class OAMenuOrigin extends EAXVxTestTemplate{
    @TestRail(caseId = 9759)
    @Test(groups = {"client", "client_only", "full_regression", "long_smoke"})
    public void testMenuOrigin(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Launch and log into Origin"); //1
        logFlowPoint("Verify 'Application Settings' is in the Origin drop down and clicking on it loads the 'Application Settings' page"); //2
        logFlowPoint("Verify 'EA Account and Billing' is in the Origin drop down and clicking on it loads the 'EA Account and Billing' page in an external browser"); //3
        logFlowPoint("Verify 'Order History' is in the Origin drop down and clicking on it loads the 'Order History' page in an external browser"); //4
        logFlowPoint("Verify 'Reload Page' is in the Origin drop down and clicking on it refreshes the page"); //5
        logFlowPoint("Verify 'Go Offline / Online' is in the Origin drop down and that clicking on it changes the client's online status to offline / online"); //6
        logFlowPoint("Verify 'Sign Out' is in the Origin drop down and clicking on it signs the user out"); //7
        logFlowPoint("Verify 'Exit' is in the Origin drop down and clicking on it closes the client"); //8

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user " + userAccount.getUsername());
        } else {
            logFailExit("Could not log into Origin with the user " + userAccount.getUsername());
        }

        //2
        MainMenu mainMenu = new MainMenu(driver);
        boolean applicationSettings = mainMenu.verifyItemEnabledInOrigin("Application Settings");
        mainMenu.selectApplicationSettings();
        boolean applicationSettingsReached = new AppSettings(driver).verifyAppSettingsReached();
        if(applicationSettings && applicationSettingsReached){
            logPass("Verified 'Application Settings' is displayed and clicking it 'Application Settings' page is reached");
        } else {
            logFailExit("Failed to verify 'Application Settings' in the Origin drop down or the page wasn't reached");
        }

        //3
        boolean accountAndBilling = mainMenu.verifyItemEnabledInOrigin("EA Account and Billing\u2026");
        mainMenu.selectAccountBilling();
        boolean accountBillingOpened = Waits.pollingWaitEx(() -> TestScriptHelper.verifyBrowserOpen(client), 15000, 1000, 0);
        TestScriptHelper.killBrowsers(client);
        if(accountAndBilling && accountBillingOpened){
            logPass("Verified 'EA Account and Billing' is displayed in the Origin drop down and clicking it opens an external browser page");
        } else {
            logFailExit("Failed to verify 'EA Account and Billing' in the Origin drop down or an external browser page was not launched");
        }

        //4
        boolean orderHistory = mainMenu.verifyItemEnabledInOrigin("Order History");
        mainMenu.selectOrderHistory();
        boolean orderHistoryOpened = Waits.pollingWaitEx(() -> TestScriptHelper.verifyBrowserOpen(client), 15000, 1000, 0);
        TestScriptHelper.killBrowsers(client);
        if(orderHistory && orderHistoryOpened){
            logPass("Verified 'Order History' is displayed in the Origin drop down and clicking it opens an external browser page");
        } else {
            logFailExit("Failed to verify 'Order History' in the Origin drop down or an external browser page was not launched");
        }

        //5
        boolean refreshButton = mainMenu.verifyItemEnabledInOrigin("Reload Page");
        boolean refreshSuccessful = mainMenu.verifyPageRefreshSuccessful();
        if (refreshButton && refreshSuccessful) {
            logPass("Verified 'Refresh' is displayed in the Origin drop down and clicking it refreshes the page");
        } else {
            logFail("Failed to verify 'Refresh' in the Origin drop down or the page was not refreshed");
        }

        //6
        boolean goOffline = mainMenu.verifyItemEnabledInOrigin("Go Offline");
        mainMenu.selectGoOffline();
        sleep(1000); // wait for offline message to appear in the client
        boolean goOnline = mainMenu.verifyItemEnabledInOrigin("Go Online");
        if(goOffline && goOnline){
            logPass("Verified 'Go Online' and 'Go Offline' are displayed in the Origin drop down");
        } else {
            logFailExit("Failed to verify 'Go Online' or 'Go Offline' in the Origin drop down");
        }

        //7
        boolean logout = mainMenu.verifyItemEnabledInOrigin("Sign Out");
        mainMenu.selectLogOut();
        boolean loginPage = Waits.pollingWaitEx(() -> new LoginPage(driver).verifyLoginPageVisible());
        if(loginPage && logout){
            logPass("Verified 'Sign Out' is displayed in the Origin drop down and clicking it brings the user to the login page");
        } else {
            logFailExit("Failed to verify 'Sign Out' in the Origin drop down or the login page was not displayed");
        }

        //8
        MacroLogin.startLogin(driver, userAccount);
        boolean exit = mainMenu.verifyItemEnabledInOrigin("Exit");
        mainMenu.selectExit();
        boolean originProcess = Waits.pollingWaitEx(() -> !ProcessUtil.isProcessRunning(client, OriginClientData.ORIGIN_PROCESS_NAME), 15000,1000,0);
        if(exit && originProcess){
            logPass("Verified 'Exit' is displayed in the Origin drop down and clicking it closes the client");
        } else {
            logFail("Failed to verify 'Exit' is displayed in the Origin drop down or the client did not close");
        }

        softAssertAll();
    }
}