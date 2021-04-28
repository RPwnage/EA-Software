package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FileUtil;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.resources.EACore;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CancelUpdateDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests canceling and pausing an update to an entitlement
 *
 * @author lscholte
 */
public class OAUpdateCancelPause extends EAXVxTestTemplate {

    @TestRail(caseId = 9825)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testUpdateCancelPause(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        OADipSmallGame entitlement = new OADipSmallGame();
        String entitlementName = entitlement.getName();
        String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String username = userAccount.getUsername();

        logFlowPoint("Log into Origin as " + username); //1
        logFlowPoint("Navigate to the 'Game Library' page"); //2
        logFlowPoint("Download and install " + entitlementName); //3
        logFlowPoint("Navigate to the 'Application Settings' page"); //4
        logFlowPoint("Enable the 'Automatic game updates' setting"); //5
        logFlowPoint("Exit the Origin client"); //6
        logFlowPoint("Add an EACore override to download an update for " + entitlementName); //7
        logFlowPoint("Log back into Origin as " + username); //8
        logFlowPoint("Navigate to the 'Game Library' page"); //9
        logFlowPoint("Verify the update to " + entitlementName + " started automatically"); //10
        logFlowPoint("Pause the update"); //11
        logFlowPoint("Resume the update"); //12
        logFlowPoint("Cancel the update and confirm that a confirmation dialog appears"); //13
        logFlowPoint("Confirm the update cancellation and verify that the update is cancelled"); //14
        logFlowPoint("Verify that cancelling the update does not uninstall the base game"); //15

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //3
        logPassFail(MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId), true);

        //4
        MainMenu mainMenu = new MainMenu(driver);
        AppSettings appSettings = mainMenu.selectApplicationSettings();
        logPassFail(appSettings.verifyAppSettingsReached(), true);

        //5
        appSettings.setKeepGamesUpToDate(true);
        boolean keepGamesUpToDate = Waits.pollingWait(() -> appSettings.verifyKeepGamesUpToDate(true));
        logPassFail(keepGamesUpToDate, true);

        //6
        mainMenu.selectExit();
        boolean clientExited = Waits.pollingWaitEx(() -> !ProcessUtil.isProcessRunning(client, "Origin.exe"));
        logPassFail(clientExited, true);

        //7
        boolean eaCoreUpdated = client.getEACore().setEACoreValue(EACore.EACORE_CONNECTION_SECTION,
                OADipSmallGame.DIP_SMALL_AUTO_DOWNLOAD_PATH,
                OADipSmallGame.DIP_SMALL_DOWNLOAD_VALUE_UPDATE_LARGE);

        logPassFail(eaCoreUpdated, true);

        //8
        client.stop();
        driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //9
        navBar = new NavigationSidebar(driver);
        gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //10
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        boolean isUpdating = Waits.pollingWait(
                () -> gameTile.isUpdating() && !gameTile.isInitializing() && !gameTile.isPreparing(),
                240000, 2000, 0); //Wait up to 4 minutes for update to begin automatically
        logPassFail(isUpdating, true);

        //11
        gameTile.pauseUpdate();
        logPassFail(gameTile.waitForPaused(), true);

        //12
        sleep(2000); //Leave paused for a bit to stabilize
        gameTile.resumeUpdate();
        boolean updateHasResumed = Waits.pollingWait(() -> !gameTile.isPaused());
        logPassFail(updateHasResumed, true);

        //13
        sleep(2000); //Leave resumed for a bit to stabilize
        gameTile.cancelUpdate();
        CancelUpdateDialog cancelUpdateDialog = new CancelUpdateDialog(driver);
        logPassFail(cancelUpdateDialog.waitIsVisible(), true);

        //14
        cancelUpdateDialog.clickYesButton();
        boolean isNotUpdating = Waits.pollingWait(() -> !gameTile.isUpdating() && gameTile.hasUpdateAvailable());
        logPassFail(isNotUpdating, true);

        //15
        boolean gameNotUninstalled = !gameTile.isDownloadable()
                && gameTile.isReadyToPlay()
                && FileUtil.isDirectoryExist(client, entitlement.getDirectoryFullPath(client));
        logPassFail(gameNotUninstalled, true);

        softAssertAll();
    }
}