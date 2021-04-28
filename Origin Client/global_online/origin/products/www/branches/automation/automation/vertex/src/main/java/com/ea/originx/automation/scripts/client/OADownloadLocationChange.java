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
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.pageobjects.settings.InstallSaveSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.ITestResult;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.Test;

/**
 * Tests downloading a game to a location other than the default Game Library
 * download location
 *
 * @author lscholte
 */
public class OADownloadLocationChange extends EAXVxTestTemplate {

    private static final Logger _log = LogManager.getLogger(OADownloadLocationChange.class);

    private String TEMP_GAMES_FOLDER_PATH = null;
    private OriginClient CLIENT = null;

    @TestRail(caseId = 10062)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testDownloadLocationChange(ITestContext context) throws Exception {

        CLIENT = OriginClientFactory.create(context);

        OADipSmallGame entitlement = new OADipSmallGame();
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount user = AccountManager.getEntitledUserAccount(entitlement);
        final String username = user.getUsername();

        logFlowPoint("Log into Origin as " + username); //1
        logFlowPoint("Create a temporary Origin Games folder"); //2
        logFlowPoint("Navigate to the 'Application Settings' page"); //3
        logFlowPoint("Navigate to the 'Installs & Saves' section"); //4
        logFlowPoint("Change the download folder to the temporary Origin Games folder"); //5
        logFlowPoint("Navigate to the 'Game Library' page"); //6
        logFlowPoint("Download and install " + entitlementName); //7
        logFlowPoint("Verify that " + entitlementName + " was successfully downloaded to the temporary Origin Games folder"); //8

        //1
        WebDriver driver = startClientObject(context, CLIENT);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin as user: " + username);
        } else {
            logFailExit("Could not log into Origin as user: " + username);
        }

        //2
        TEMP_GAMES_FOLDER_PATH = SystemUtilities.getProgramFilesPath(CLIENT) + "\\Temp Origin Games";
        final String expectedEntitlementPath = TEMP_GAMES_FOLDER_PATH + "\\" + entitlementName;
        final String notExpectedEntitlementPath = entitlement.getDirectoryFullPath(CLIENT);
        FileUtil.deleteFolder(CLIENT, TEMP_GAMES_FOLDER_PATH);
        if (FileUtil.createFolder(CLIENT, TEMP_GAMES_FOLDER_PATH)) {
            logPass("Successfully created a temporary Origin Games folder");
        } else {
            logFailExit("Failed to create a temporary Origin Games folder");
        }

        //3
        AppSettings appSettings = new MainMenu(driver).selectApplicationSettings();
        if (appSettings.verifyAppSettingsReached()) {
            logPass("Successfully navigated to the 'Application Settings' page");
        } else {
            logFailExit("Failed to navigate to the 'Application Settings' page");
        }

        //4
        new SettingsNavBar(driver).clickInstallSaveLink();
        InstallSaveSettings installSaveSettings = new InstallSaveSettings(driver);
        if (Waits.pollingWait(() -> installSaveSettings.verifyInstallSaveSettingsReached())) {
            logPass("Successfully navigated to 'Install & Saves' section");
        } else {
            logFailExit("Failed to navigate to 'Install & Saves' section");
        }

        //5
        installSaveSettings.clickChangeGameFolderLocation();

        //Type in the location of the folder with denied permissions
        RobotKeyboard robotHandler = new RobotKeyboard();
        robotHandler.type(CLIENT, TEMP_GAMES_FOLDER_PATH, 200);

        sleep(1000);
        robotHandler.type(CLIENT, "\n\n", 2000);

        InstallationSettingsChangedDialog installationSettingsChangedDialog = new InstallationSettingsChangedDialog(driver);
        installationSettingsChangedDialog.close();
        Waits.pollingWait(() -> !installationSettingsChangedDialog.waitIsVisible());
        if (installSaveSettings.verifyGameLibraryLocation(TEMP_GAMES_FOLDER_PATH)) {
            logPass("Successfully changed download location");
        } else {
            logFailExit("Failed to change download location");
        }

        //6
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigated to the 'Game Library' page");
        } else {
            logFailExit("Failed to navigate to the 'Game Library' page");
        }

        //7
        boolean downloaded = MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId);
        if (downloaded) {
            logPass("Successfully downloaded and installed " + entitlementName);
        } else {
            logFailExit("Failed to download and install " + entitlementName);
        }

        boolean downloadedToCorrectLocation = FileUtil.isDirectoryExist(CLIENT, expectedEntitlementPath)
                && !(FileUtil.isDirectoryExist(CLIENT, notExpectedEntitlementPath));
        if (downloadedToCorrectLocation) {
            logPass("Successfully downloaded and installed " + entitlementName + " to the new location");
        } else {
            logFail("Failed to download and install " + entitlementName + " to the new location");
        }

        softAssertAll();
    }

    @Override
    @AfterMethod(alwaysRun = true)
    protected void testCleanUp(ITestResult result, ITestContext context) {
        if (null != TEMP_GAMES_FOLDER_PATH && null != CLIENT) {
            FileUtil.deleteFolder(CLIENT, TEMP_GAMES_FOLDER_PATH);
        }

        _log.debug("Performing test cleanup for " + this.getClass().getSimpleName());
        super.testCleanUp(result, context);
    }

}
