package com.ea.originx.automation.scripts.storehome;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroNetworkOffline;
import com.ea.originx.automation.lib.macroaction.MacroOfflineMode;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.OfflineFlyout;
import com.ea.originx.automation.lib.pageobjects.common.TakeOverPage;
import com.ea.originx.automation.lib.pageobjects.store.StorePage;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test store page when offline. Both offline mode and network offline are
 * covered
 *
 * @author palui
 */
public class OAOfflineStore extends EAXVxTestTemplate {

    @TestRail(caseId = 39669)
    @Test(groups = {"storehome", "client_only", "full_regression"})
    public void testOfflineStore(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        
        final UserAccount userAccount = AccountManager.getRandomAccount();
        final String username = userAccount.getUsername();

        logFlowPoint("Launch Origin and login as user: " + username); //1
        logFlowPoint("Navigate to the 'Store' page"); //2
        logFlowPoint("Click 'Go Offline' at the Origin menu. Verify client is offline"); //3
        logFlowPoint("Verify 'Store' page is offline"); //4
        logFlowPoint("Click 'Go Online' button at the 'Store' page. Verify client is online"); //5
        logFlowPoint("Verify 'Store' page is online"); //6
        logFlowPoint("Disconnect Origin from the network. Verify client is offline"); //7
        logFlowPoint("Verify 'Store' page is offline"); //8

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful as user: " + username);
        } else {
            logFailExit("Failed: Cannot login as user: " + username);
        }

        //2
        StorePage storePage = new NavigationSidebar(driver).gotoStorePage();
        new TakeOverPage(driver).closeIfOpen();
        if (storePage.verifyStorePageReached()) {
            logPass("Verified 'Store' page opens successfully");
        } else {
            logFailExit("Failed: Cannot open 'Store' page");
        }

        //3
        if (MacroOfflineMode.goOffline(driver)) {
            logPass("Verified client is offline");
        } else {
            logFailExit("Failed: Client cannot 'Go Offline'");
        }

        //4
        // Go to SPA page - required after going offline using the main menu
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60);
        new OfflineFlyout(driver).closeOfflineFlyout(); // Close OfflineFlyout which may interfere with store page
        boolean storeIsOffline = storePage.verifyStorePageOffline() && storePage.verifyStorePageOfflineMessage();
        if (storeIsOffline) {
            logPass("Verified 'Store' page is offline");
        } else {
            logFailExit("Failed: 'Store' page is not offline");
        }

        //5
        storePage.clickGoOnline();
        if (MacroOfflineMode.isOnline(driver)) {
            logPass("Verified client is online");
        } else {
            logFailExit("Failed: Client cannot 'Go Online'");
        }

        //6
        // Go back to SPA page - required verifying online status at the main menu
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60);
        if (storePage.verifyStorePageOnline()) {
            logPass("Verified 'Store' page is online");
        } else {
            logFailExit("Failed: 'Store' page is not online");
        }

        //7
        if (MacroNetworkOffline.enterOriginNetworkOffline(driver)) {
            logPass("Verified Origin disconnected from the network");
        } else {
            logFailExit("Failed: Origin cannot be disconnected from the network");
        }

        //8
        // Go to SPA page - required after entering network offline which checks the main menu
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60);
        storeIsOffline = storePage.verifyStorePageOffline() && storePage.verifyStorePageOfflineMessage();
        if (storeIsOffline) {
            logPass("Verified 'Store' page is offline");
        } else {
            logFailExit("Failed: 'Store' page is not offline");
        }

        softAssertAll();
    }
}
