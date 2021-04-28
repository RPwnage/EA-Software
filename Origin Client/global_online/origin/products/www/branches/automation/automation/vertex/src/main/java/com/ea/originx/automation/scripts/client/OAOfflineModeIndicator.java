package com.ea.originx.automation.scripts.client;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.OfflineFlyout;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to verify the 'Offline Mode' indicator at the right side of the menu bar
 *
 * @author palui
 */
public class OAOfflineModeIndicator extends EAXVxTestTemplate {

    @TestRail(caseId = 9673)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testOfflineModeIndicator(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();
        String userName = userAccount.getUsername();

        logFlowPoint("Launch Origin and login as user: " + userName); //1
        logFlowPoint("Click 'Go Offline' at the Origin menu and verify client is offline"); //2
        logFlowPoint("Verify there is a button labelled 'OFFLINE MODE' at the top-right of the menu bar"); //3
        logFlowPoint("Verify an 'Offline Flyout' appears with the correct title and message"); //4
        logFlowPoint("Click 'Offline Mode' button at the menu bar and verify 'Offline Flyout' closes"); //5
        logFlowPoint("Click 'Offline Mode' button again and verify 'Offline Flyout' re-appears"); //6
        logFlowPoint("Click 'Go Online' at the 'Offline Flyout' and verify client is online"); //7
        logFlowPoint("Verify 'Offline Flyout' has been closed"); //8
        logFlowPoint("Verify 'Offline Mode' button is no longer visible"); //9

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful as user: " + userName);
        } else {
            logFailExit("Failed: Cannot login as user: " + userName);
        }

        //2
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectGoOffline();
        sleep(1000); // wait for Origin menu to update
        boolean mainMenuOffline = mainMenu.verifyItemEnabledInOrigin("Go Online");
        if (mainMenuOffline) {
            logPass("Verified client is offline");
        } else {
            logFailExit("Failed: Client cannot 'Go Offline'");
        }

        //3
        if (mainMenu.verifyOfflineModeButtonText()) {
            logPass("Verified 'Offline Mode' button labelled 'OFFLINE MODE' is visible at the menu bar");
        } else {
            logFailExit("Failed: 'Offline Mode' button labelled 'OFFLINE MODE' is not visible at the menu bar");
        }

        //4
        mainMenu.clickOfflineModeButton();
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60); // Go back to SPA page - required after going offline
        OfflineFlyout offlineFlyout = new OfflineFlyout(driver);
        if (offlineFlyout.verifyOfflineFlyoutTitle() && offlineFlyout.verifyOfflineFlyoutMessage()) {
            logPass("Verified 'Offline Flyout' is visible with correct title and message");
        } else {
            logFailExit("Failed: 'Offline Flyout' is not visible or has incorrect title/message");
        }

        //5
        mainMenu.clickOfflineModeButton();
        sleep(1000);
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60); // Go back to SPA page - required after clicking offline mode button
        if (!offlineFlyout.verifyOfflineFlyoutVisible()) {
            logPass("Verified 'Offline Flyout' closes");
        } else {
            logFailExit("Failed: 'Offline Flyout' is still visible");
        }

        //6
        mainMenu.clickOfflineModeButton();
        sleep(1000);
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60); // Go back to SPA page - required after clicking offline mode button
        if (offlineFlyout.verifyOfflineFlyoutTitle()) {
            logPass("Verified 'Offline Flyout' re-appears");
        } else {
            logFailExit("Failed: 'Offline Flyout' does not re-appear");
        }

        //7
        offlineFlyout.clickGoOnlineButton();
        sleep(1000); // wait for Origin menu to update
        boolean clientIsOnline = Waits.pollingWait(() -> mainMenu.verifyItemEnabledInOrigin("Go Offline"));
        if (clientIsOnline) {
            logPass("Verified client is online");
        } else {
            logFailExit("Failed: Client is not online");
        }

        //8
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60); // Go back to SPA page - required after verifying item in Origin menu
        if (!offlineFlyout.verifyOfflineFlyoutVisible()) {
            logPass("Verified 'Offline Flyout' has been closed");
        } else {
            logFailExit("Failed: 'Offline Flyout' is still visible");
        }

        //9
        if (!mainMenu.verifyOfflineModeButtonVisible(2)) {
            logPass("Verified 'Offline Mode' button is no longer visible");
        } else {
            logFailExit("Failed: 'Offline Mode' button is still visible");
        }

        softAssertAll();
    }
}
