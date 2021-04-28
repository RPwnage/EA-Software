package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.pageobjects.dialog.CancelUpdateDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.UpdateCompleteDialog;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.UpdateAvailableDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.resources.EACore;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests patching a game with both the 'Automatic game updates' setting enabled
 * and disabled. Also tests automatic update downloading and update dialog.
 *
 * @author lscholte
 * @author rocky
 */
public class OAPatchGame extends EAXVxTestTemplate {

    @TestRail(caseId = 9840)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testPatchGame(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        OADipSmallGame entitlement = new OADipSmallGame();
        String entitlementOfferId = entitlement.getOfferId();
        String entitlementName = entitlement.getName();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String username = userAccount.getUsername();

        logFlowPoint("Log into Origin."); // 1
        logFlowPoint("Verify navigation to the 'Game Library' page."); // 2
        logFlowPoint("Verify downloading/installing game."); // 3
        logFlowPoint("Verify navigation to the 'Application Settings' page."); // 4
        logFlowPoint("Verify enabling the 'Automatic Game Updates' setting."); // 5
        logFlowPoint("Log out of Origin and update the EACore."); // 6
        logFlowPoint("Log back into Origin with the user."); // 7
        logFlowPoint("Verify game automatically starts updating."); // 8
        logFlowPoint("Cancel updating and verify disabling the 'Automatic Game Updates' setting."); // 9
        logFlowPoint("Log out and verify updating the EACore override setting."); // 10
        logFlowPoint("Successfully log into Origin."); // 11
        logFlowPoint("Verify game does NOT automatically start updating."); // 12
        logFlowPoint("Navigate to the 'Game Library' and verify that the 'Updating Game' dialog appears when attempting to play game."); // 13
        logFlowPoint("Click the 'Update' button and verify game starts updating."); // 14
        logFlowPoint("Verify update completion."); // 15


        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user: " + username);
        } else {
            logFailExit("Could not log into Origin with the user: " + username);
        }

        // 2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigated to the 'Game Library' page.");
        } else {
            logFailExit("Failed to navigate to the 'Game Library' page.");
        }

        // 3
        boolean isDownloaded = MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId);
        if (isDownloaded) {
            logPass("Successfully downloaded game: " + entitlementName);
        } else {
            logFailExit("Failed to download game: " + entitlementName);
        }

        // 4
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectApplicationSettings();
        SettingsNavBar settingsNavBar = new SettingsNavBar(driver);
        settingsNavBar.clickApplicationLink();
        AppSettings appSettings = new AppSettings(driver);
        if (appSettings.verifyAppSettingsReached()) {
            logPass("Successfully navigated to the 'Application Settings' page.");
        } else {
            logFailExit("Failed to navigate to the 'Application Settings' page.");
        }

        // 5
        appSettings.scrollToBottom();
        appSettings.setKeepGamesUpToDate(true);
        if (Waits.pollingWaitEx(() -> appSettings.verifyKeepGamesUpToDate(true))) {
            logPass("Successfully enabled the 'Automatic game updates' setting.");
        } else {
            logFailExit("Failed to enable the 'Automatic game updates' setting.");
        }

        // 6
        mainMenu.selectLogOut();
        boolean isEACoreUpdated = OADipSmallGame.changeToMediumEntitlementDipSmallOverride(client);
        if (isEACoreUpdated) {
            logPass("Logged out and Successfully updated the EACore.");
        } else {
            logFailExit("Failed to update the EACore after logging out.");
        }

        // 7
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        // 8
        Waits.pollingWaitEx(() -> navBar.isSidebarOpen());
        navBar.gotoGameLibrary();
        final GameTile gameTile = new GameTile(driver, entitlementOfferId);
        boolean isGameStartUpdating = gameTile.waitForUpdating(20);
        if (isGameStartUpdating) {
            logPass("Verified game automatically starts updating.");
        } else {
            logFailExit("Failed: Game does not automatically start updating.");
        }

        // 9
        gameTile.cancelUpdate();
        CancelUpdateDialog cancelUpdateDialog = new CancelUpdateDialog(driver);
        Waits.pollingWaitEx(() -> cancelUpdateDialog.waitIsVisible());
        cancelUpdateDialog.clickYesButton();
        mainMenu.selectApplicationSettings();
        settingsNavBar.clickApplicationLink();
        Waits.pollingWaitEx(() -> appSettings.verifyAppSettingsReached());
        appSettings.scrollToBottom();
        appSettings.setKeepGamesUpToDate(false);
        if (Waits.pollingWaitEx(() -> appSettings.verifyKeepGamesUpToDate(false))) {
            logPass("Cancelled updating and verified 'Automatic Game Updates' setting was disabled.");
        } else {
            logFailExit("Failed to disable the 'Automatic Game Updates' setting after cancelling the update.");
        }

        // 10
        mainMenu.selectLogOut();
        boolean isEACoreUpdated2 = client.getEACore().setEACoreValue(EACore.EACORE_CONNECTION_SECTION,
                OADipSmallGame.DIP_SMALL_AUTO_DOWNLOAD_PATH,
                OADipSmallGame.DIP_SMALL_DOWNLOAD_VALUE_UPDATE_TINY);
        if (isEACoreUpdated2) {
            logPass("Logged out and verified updating the EACore override setting.");
        } else {
            logFailExit("Failed to update the EACore override setting after logging out.");
        }

        // 11
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        // 12
        navBar.gotoGameLibrary();
        isGameStartUpdating = gameTile.waitForUpdating(20);
        if (!isGameStartUpdating) {
            logPass("Verified game does NOT automatically start updating.");
        } else {
            logFailExit("Failed: Game does automatically start updating.");
        }

        // 13
        gameTile.play();
        UpdateAvailableDialog updateDialog = new UpdateAvailableDialog(driver);
        if (updateDialog.waitIsVisible()) {
            logPass("Navigated to the 'Game Library' and verified the 'Update Available' dialog appears when attempting to play the game.");
        } else {
            logFailExit("The 'Update Available' dialog did not appear when attempting to play the game after navigating to the 'Game Library'.");
        }

        // 14
        updateDialog.clickUpdateButton();
        isGameStartUpdating = gameTile.waitForUpdating(20);
        if (isGameStartUpdating) {
            logPass("Verified game starts updating after clicking update button.");
        } else {
            logFailExit("Failed: Game does not start updating after clicking update button.");
        }

        // 15
        Waits.pollingWait(() -> !gameTile.isUpdating());
        boolean isGameUpdated = Waits.pollingWait(() -> new UpdateCompleteDialog(driver).isOpen(), 500000, 500, 0);
        if (isGameUpdated) {
            logPass("Verified updating is complete.");
        } else {
            logFailExit("Failed to update game.");
        }

        softAssertAll();
    }
}