package com.ea.originx.automation.scripts.client;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FileUtil;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.vx.originclient.helpers.RobotKeyboard;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.InstallationSettingsChangedDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.pageobjects.settings.InstallSaveSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.ITestResult;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.Test;

/**
 * Tests resetting the default location of a game
 *
 * @author jmittertreiner
 */
public class OAResetDownloadLocation extends EAXVxTestTemplate {

    private String TEMP_GAMES_FOLDER_PATH = null;
    private OriginClient CLIENT = null;

    @TestRail(caseId = 10066)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testDownloadLocationChange(ITestContext context) throws Exception {

        CLIENT = OriginClientFactory.create(context);

        final OADipSmallGame entitlement = new OADipSmallGame();
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount user = AccountManager.getEntitledUserAccount(entitlement);

        logFlowPoint("Log into Origin as " + user.getUsername()); //1
        logFlowPoint("Navigate to the 'Application Settings' page"); //2
        logFlowPoint("Change the download location for games downloads"); //3
        logFlowPoint("Navigate to the 'Game Library' page"); //4
        logFlowPoint("Download and install " + entitlementName); //5
        logFlowPoint("Verify that " + entitlementName + " was successfully downloaded to the temporary Origin Games folder"); //6
        logFlowPoint("Open the Application Settings -> Install & Saves"); //7
        logFlowPoint("Select 'Restore Default' for the download location"); //8
        logFlowPoint("Navigate to the GAme Library"); //9
        logFlowPoint("Uninstall Dip Small then download Dip Small"); //10
        logFlowPoint("Verify DiP Small is in the default directory"); //11

        //1
        final WebDriver driver = startClientObject(context, CLIENT);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        //2
        MainMenu mainMenu = new MainMenu(driver);
        AppSettings appSettings = mainMenu.selectApplicationSettings();
        if (appSettings.verifyAppSettingsReached()) {
            logPass("Successfully navigated to the 'Application Settings' page");
        } else {
            logFailExit("Failed to navigate to the 'Application Settings' page");
        }

        // Clear out and recreate the temporary folder
        TEMP_GAMES_FOLDER_PATH = SystemUtilities.getProgramFilesPath(CLIENT) + "\\Temp Origin Games";
        final String defaultEntitlementPath = entitlement.getDirectoryFullPath(CLIENT);
        final String changedEntitlementPath = TEMP_GAMES_FOLDER_PATH + "\\" + entitlement.getDirectoryName();
        FileUtil.deleteFolder(CLIENT, TEMP_GAMES_FOLDER_PATH);
        boolean madeDirs = FileUtil.createFolder(CLIENT, TEMP_GAMES_FOLDER_PATH);

        new SettingsNavBar(driver).clickInstallSaveLink();
        InstallSaveSettings installSaveSettings = new InstallSaveSettings(driver);
        installSaveSettings.clickChangeGameFolderLocation();
        RobotKeyboard robotHandler = new RobotKeyboard();
        robotHandler.type(CLIENT, TEMP_GAMES_FOLDER_PATH, 200);
        sleep(1000);
        robotHandler.type(CLIENT, "\n\n", 2000);

        //3
        InstallationSettingsChangedDialog installationSettingsChangedDialog = new InstallationSettingsChangedDialog(driver);
        installationSettingsChangedDialog.close();
        Waits.pollingWait(() -> !installationSettingsChangedDialog.waitIsVisible());
        if (madeDirs && installSaveSettings.verifyGameLibraryLocation(TEMP_GAMES_FOLDER_PATH)) {
            logPass("Successfully changed download location");
        } else {
            logFailExit("Failed to change download location");
        }

        //4
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigated to the 'Game Library' page");
        } else {
            logFailExit("Failed to navigate to the 'Game Library' page");
        }

        //5
        boolean downloaded = MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId);
        if (downloaded) {
            logPass("Successfully downloaded and installed " + entitlementName);
        } else {
            logFailExit("Failed to download and install " + entitlementName);
        }

        //6
        boolean downloadedToCorrectLocation = FileUtil.isDirectoryExist(CLIENT, changedEntitlementPath)
                && !FileUtil.isDirectoryExist(CLIENT, defaultEntitlementPath);
        if (downloadedToCorrectLocation) {
            logPass("Successfully downloaded and installed " + entitlementName + " to the new location");
        } else {
            logFail("Failed to download and install " + entitlementName + " to the new location");
        }

        //7
        mainMenu.selectApplicationSettings();
        if (appSettings.verifyAppSettingsReached()) {
            logPass("Successfully navigated to the 'Application Settings' page");
        } else {
            logFailExit("Failed to navigate to the 'Application Settings' page");
        }

        // 8
        new SettingsNavBar(driver).clickInstallSaveLink();
        installSaveSettings.clickGameLibraryRestoreDefaultLocation();
        installationSettingsChangedDialog.close();
        Waits.pollingWait(() -> !installationSettingsChangedDialog.waitIsVisible());
        if (installSaveSettings.verifyGameLibraryLocation(SystemUtilities.getOriginGamesPath(CLIENT) + "\\")) {
            logPass("Successfully changed download location back to default");
        } else {
            logFailExit("Failed to change download location back to default");
        }

        // 9
        navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigated to the 'Game Library' page");
        } else {
            logFailExit("Failed to navigate to the 'Game Library' page");
        }

        // 10
        entitlement.enableSilentUninstall(CLIENT);
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        gameTile.uninstall();
        gameTile.waitForDownloadable();
        downloaded = MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId);
        if (downloaded) {
            logPass("Successfully downloaded and installed " + entitlementName);
        } else {
            logFailExit("Failed to download and install " + entitlementName);
        }

        // 11
        downloadedToCorrectLocation = FileUtil.isDirectoryExist(CLIENT, defaultEntitlementPath)
                && !FileUtil.isDirectoryExist(CLIENT, changedEntitlementPath);
        if (downloadedToCorrectLocation) {
            logPass("Successfully downloaded and installed " + entitlementName + " to the default location");
        } else {
            logFail("Failed to download and install " + entitlementName + " to the default location");
        }

        softAssertAll();
    }

    @Override
    @AfterMethod(alwaysRun = true)
    protected void testCleanUp(ITestResult result, ITestContext context) {
        if (null != TEMP_GAMES_FOLDER_PATH && null != CLIENT) {
            FileUtil.deleteFolder(CLIENT, TEMP_GAMES_FOLDER_PATH);
        }
        super.testCleanUp(result, context);
    }

}
