package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.annotations.TestRail;
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
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadProblemDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.InstallationSettingsChangedDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.pageobjects.settings.InstallSaveSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.apache.logging.log4j.LogManager;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.ITestResult;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.Test;

/**
 * Tests that the 'Download Problem' dialog appears when Origin is unable to
 * download a game
 *
 * @author lscholte
 */
public class OADownloadProblemDialog extends EAXVxTestTemplate {

    private static final org.apache.logging.log4j.Logger _log = LogManager.getLogger(EAXVxTestTemplate.class);

    private String TEMP_GAMES_FOLDER_PATH = null;
    private OriginClient CLIENT = null;

    @TestRail(caseId = 9848)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testDownloadProblemDialog(ITestContext context) throws Exception {

        CLIENT = OriginClientFactory.create(context);

        OADipSmallGame entitlement = new OADipSmallGame();
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount user = AccountManager.getEntitledUserAccount(entitlement);
        final String username = user.getUsername();

        logFlowPoint("Log into Origin as " + username); //1
        logFlowPoint("Create a temporary Origin Games folder and with all permissions denied"); //2
        logFlowPoint("Navigate to the 'Application Settings' page"); //3
        logFlowPoint("Navigate to the 'Installs & Saves' section"); //4
        logFlowPoint("Change the download folder to a folder with all permissions denied"); //5
        logFlowPoint("Navigate to the 'Game Library' page"); //6
        logFlowPoint("Verify that the 'Download Problem' dialog appears when attempting to download " + entitlementName); //7

        //1
        WebDriver driver = startClientObject(context, CLIENT);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        //2
        TEMP_GAMES_FOLDER_PATH = SystemUtilities.getProgramFilesPath(CLIENT) + "\\Origin Games No Access";
        FileUtil.setPermissionsOnFolder(CLIENT, TEMP_GAMES_FOLDER_PATH, true);
        FileUtil.deleteFolder(CLIENT, TEMP_GAMES_FOLDER_PATH);
        FileUtil.createFolder(CLIENT, TEMP_GAMES_FOLDER_PATH);
        boolean folderPermissionsDenied = FileUtil.setPermissionsOnFolder(CLIENT, TEMP_GAMES_FOLDER_PATH, false);
        if (folderPermissionsDenied) {
            logPass("Successfully created an Origin Games folder with all permissions denied");
        } else {
            logFailExit("Failed to create an Origin Games folder with all permissions denied");
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

        //Press ENTER 4 times to handle Windows permission denied dialogs
        sleep(1000);
        robotHandler.type(CLIENT, "\n\n\n\n", 2000);

        InstallationSettingsChangedDialog installationSettingsChangedDialog = new InstallationSettingsChangedDialog(driver);
        installationSettingsChangedDialog.waitForVisible();
        installationSettingsChangedDialog.close();
        if (Waits.pollingWait(() -> installSaveSettings.verifyGameLibraryLocation(TEMP_GAMES_FOLDER_PATH))) {
            logPass("Successfully changed download location to folder with no permissions");
        } else {
            logFailExit("Failed to change download location to folder with no permissions");
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
        MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferId);
        DownloadProblemDialog downloadProblem = new DownloadProblemDialog(driver);
        if (downloadProblem.isOpen()) {
            logPass("The 'Download Problem' dialog appeared when attempting to start downloading " + entitlementName);
        } else {
            logFailExit("The 'Download Problem' dialog did not appear when attempting to start downloading " + entitlementName);
        }

        softAssertAll();
    }

    @Override
    @AfterMethod(alwaysRun = true)
    protected void testCleanUp(ITestResult result, ITestContext context) {
        if (null != TEMP_GAMES_FOLDER_PATH && null != CLIENT) {
            FileUtil.setPermissionsOnFolder(CLIENT, TEMP_GAMES_FOLDER_PATH, true);
            FileUtil.deleteFolder(CLIENT, TEMP_GAMES_FOLDER_PATH);
        }

        _log.debug("Performing test cleanup for OADownloadProblemDialog");
        super.testCleanUp(result, context);
    }

}
