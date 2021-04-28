package com.ea.originx.automation.scripts.client;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroNetworkOffline;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.OfflineFlyout;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to verify the 'Offline Mode' indicator at the right side of the menu bar
 * when Origin Client is Network Offline
 *
 * @author palui
 */
public class OANetworkOfflineIndicator extends EAXVxTestTemplate {

    @TestRail(caseId = 9677)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testNetworkOfflineIndicator(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();
        String userName = userAccount.getUsername();

        logFlowPoint("Launch Origin client and login as user: " + userName); //1
        logFlowPoint("Enter Origin client into Network Offline"); //2
        logFlowPoint("Verify there is a button labelled 'OFFLINE MODE' at the top-right of the menu bar"); //3
        logFlowPoint("Verify an 'Offline Flyout' appears with the correct title and message"); //4
        logFlowPoint("Exit Origin client from Network Offline"); //5
        logFlowPoint("Verify 'Offline Flyout' has been closed"); //6
        logFlowPoint("Verify 'Offline Mode' button is no longer visible"); //7

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful as user: " + userName);
        } else {
            logFailExit("Failed: Cannot login as user: " + userName);
        }

        //2
        if (MacroNetworkOffline.enterOriginNetworkOffline(driver)) {
            logPass("Verified Origin enters into Network Offline");
        } else {
            logFailExit("Failed: Origin does not enter into Network Offline");
        }

        //3
        MainMenu mainMenu = new MainMenu(driver);
        // Already checked 'Offline Mode' button in macro. Recheck for this test case to match TFC
        if (mainMenu.verifyOfflineModeButtonText()) {
            logPass("Verified 'Offline Mode' button labelled 'OFFLINE MODE' is visible at the menu bar");
        } else {
            logFailExit("Failed: 'Offline Mode' button is not visible at the menu bar");
        }

        //4
        mainMenu.clickOfflineModeButton();
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60); // Go back to SPA page - required after checking menu bar items
        OfflineFlyout offlineFlyout = new OfflineFlyout(driver);
        boolean offlineFlyoutOK = offlineFlyout.verifyOfflineFlyoutTitle() && offlineFlyout.verifyOfflineFlyoutMessage();
        if (offlineFlyoutOK) {
            logPass("Verified 'Offline Flyout' is visible with correct title and message");
        } else {
            logFailExit("Failed: 'Offline Flyout' is not visible or has incorrect title/message");
        }

        //5
        if (MacroNetworkOffline.exitOriginNetworkOffline(driver)) {
            logPass("Verified Origin exits from Network Offline and is now Online");
        } else {
            logFailExit("Failed: Origin does not exit from Network Offline");
        }

        //6
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60); // Go back to SPA page - required after exiting Network Offline
        if (!offlineFlyout.verifyOfflineFlyoutTitle()) {
            logPass("Verified 'Offline Flyout' has been closed");
        } else {
            logFailExit("Failed: 'Offline Flyout' is still visible");
        }

        //7
        if (!mainMenu.verifyOfflineModeButtonVisible()) {
            logPass("Verified 'Offline Mode' button is no longer visible");
        } else {
            logFailExit("Failed: 'Offline Mode' button is still visible");
        }

        softAssertAll();
    }
}
