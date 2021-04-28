package com.ea.originx.automation.scripts.client;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.TakeOverPage;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the Discover Page when it is in offline mode
 *
 * @author lscholte
 */
public class OADiscoverOfflineMode extends EAXVxTestTemplate {

    @TestRail(caseId = 9679)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testDiscoverOffline(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Launch and login to Origin with " + userAccount.getUsername()); //1
        logFlowPoint("Navigate to the Discover Page"); //2
        logFlowPoint("Select 'Go Offline' in the Origin menu"); //3
        logFlowPoint("Verify there is an indication that the user is offline"); //4
        logFlowPoint("Verify there is a message telling the user how to go back online"); //5
        logFlowPoint("Select 'Go Online' from the Origin menu"); //6
        logFlowPoint("Verify the Discover Page is automatically refreshed after going back online"); //7

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user");
        } else {
            logFailExit("Failed to log into Origin with the user");

        }

        //2
        DiscoverPage discoverPage = new NavigationSidebar(driver).gotoDiscoverPage();
        new TakeOverPage(driver).closeIfOpen();
        if (discoverPage.verifyDiscoverPageReached()) {
            logPass("Successfully navigated to the Discover Page");
        } else {
            logFailExit("Failed to navigate to the Discover Page");
        }

        //3
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectGoOffline();
        if (mainMenu.verifyOfflineModeButtonText()) {
            logPass("Successfully entered offline mode");
        } else {
            logFailExit("Failed to enter offline mode");
        }
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60); // Go back to SPA page - required after verifying item in Origin menu

        //4
        if (discoverPage.verifyOfflineMessageExists()) {
            logPass("The message indicating Origin is offline is visible");
        } else {
            logFail("The message indicating Origin is offline is not visible");
        }

        //5
        if (discoverPage.verifyGoBackOnlineMessageVisible()) {
            logPass("The message stating how to go back online is visible");
        } else {
            logFail("The message stating how to go back online is not visible");
        }

        //6
        mainMenu.selectGoOnline();
        if (!mainMenu.verifyOfflineModeButtonText()) {
            logPass("Successfully exited offline mode");
        } else {
            logFailExit("Failed to exit offline mode");
        }
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60); // Go back to SPA page - required after verifying item in Origin menu

        //7
        new TakeOverPage(driver).closeIfOpen();
        if (discoverPage.verifyDiscoverPageReached()) {
            logPass("The Discover Page was refreshed after going back online");
        } else {
            logFail("The Discover Page was not refreshed after going back online");
        }

        softAssertAll();
    }

}
