package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that a game download automatically resumes downloading after logging
 * out then back into Origin
 *
 * @author lscholte
 */
public class OADownloadLogout extends EAXVxTestTemplate {

    @TestRail(caseId = 9876)
    @Test(groups = {"gamedownloader", "client_only", "full_regression", "gamedownloader_smoke"})
    public void testDownloadLogout(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String username = userAccount.getUsername();

        logFlowPoint("Log into Origin as " + username); //1
        logFlowPoint("Navigate to the 'Game Library' page"); //2
        logFlowPoint("Start downloading " + entitlementName); //3
        logFlowPoint("Logout of Origin"); //4
        logFlowPoint("Wait 30 seconds and log back into Origin as " + username); //5
        logFlowPoint("Navigate to the 'Game Library' page"); //6
        logFlowPoint("Verify that " + entitlementName + " automatically resumes downloading"); //7

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user");
        } else {
            logFailExit("Could not log into Origin with the user");
        }

        //2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigated to the 'Game Library' page");
        } else {
            logFailExit("Failed to navigate to the 'Game Library' page");
        }

        //3
        boolean downloaded = MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferId);
        if (downloaded) {
            logPass("Successfully started downloading " + entitlementName);
        } else {
            logFailExit("Failed to start downloading " + entitlementName);
        }

        //4
        LoginPage loginPage = new MainMenu(driver).selectLogOut();
        if (loginPage.isOnLoginPage()) {
            logPass("Successfully logged out of Origin");
        } else {
            logFailExit("Failed to log out of Origin");
        }

        //5
        //Wait 30 seconds as described in the test case steps
        sleep(30000);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged back into Origin after 30 seconds with the user");
        } else {
            logFailExit("Could not log back into Origin after 30 seconds with the user");
        }

        //6
        gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigated to the 'Game Library' page");
        } else {
            logFailExit("Failed to navigate to the 'Game Library' page");
        }

        //7
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        boolean resumedDownloading = Waits.pollingWait(() -> gameTile.isDownloading() && !gameTile.isPaused(),
                30000, 1000, 0);
        if (resumedDownloading) {
            logPass(entitlementName + " automatically resumed downloading");
        } else {
            logFailExit(entitlementName + " did not automatically resume downloading");
        }

        softAssertAll();
    }
}
