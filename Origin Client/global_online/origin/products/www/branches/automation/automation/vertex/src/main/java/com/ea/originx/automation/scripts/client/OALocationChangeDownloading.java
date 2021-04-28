package com.ea.originx.automation.scripts.client;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
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
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.apache.logging.log4j.LogManager;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.ITestResult;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.Test;

/**
 * Tests changing the Game Library download location while a game is being
 * downloaded
 *
 * @author lscholte
 * @author mdobre
 */
public class OALocationChangeDownloading extends EAXVxTestTemplate {

    private static final org.apache.logging.log4j.Logger _log = LogManager.getLogger(OALocationChangeDownloading.class);

    private String TEMP_GAMES_FOLDER_PATH = null;
    private OriginClient CLIENT = null;

    @TestRail(caseId = 10063)
    @Test(groups = {"client", "client_only", "full_regression", "int_only"})
    public void testLocationChangeDownloading(ITestContext context) throws Exception {

        CLIENT = OriginClientFactory.create(context);

        final EntitlementInfo largeEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);
        final String largeEntitlementName = largeEntitlement.getName();
        final String largeEntitlementOfferId = largeEntitlement.getOfferId();
        final String largeEntitlementFolderName = largeEntitlement.getDirectoryName();

        final EntitlementInfo smallEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        final String smallEntitlementName = smallEntitlement.getName();
        final String smallEntitlementOfferId = smallEntitlement.getOfferId();
        final String smallEntitlementFolderName = smallEntitlement.getDirectoryName();

        final AccountManager accountManager = AccountManager.getInstance();
        Criteria criteria = new Criteria.CriteriaBuilder().build();
        criteria.addEntitlement(largeEntitlementName);
        criteria.addEntitlement(smallEntitlementName);
        UserAccount user = accountManager.requestWithCriteria(criteria, 360);

        logFlowPoint("Log into Origin as " + user.getUsername()); //1
        logFlowPoint("Create a temporary Origin Games folder"); //2
        logFlowPoint("Navigate to the 'Game Library' page"); //3
        logFlowPoint("Start downloading " + largeEntitlementName); //4
        logFlowPoint("Navigate to the 'Application Settings' page"); //5
        logFlowPoint("Navigate to the 'Installs & Saves' section"); //6
        logFlowPoint("Change the download folder to the temporary Origin Games folder"); //7
        logFlowPoint("Navigate to the 'Game Library' page"); //8
        logFlowPoint("Wait for " + largeEntitlementName + " to finish downloading and installing"); //9
        logFlowPoint("Verify " + largeEntitlementName + " was downloaded to the original Origin Games folder"); //10
        logFlowPoint("Download and install " + smallEntitlementName); //11
        logFlowPoint("Verify that " + smallEntitlementName + " was downloaded to the temporary Origin Games folder"); //12

        //1
        WebDriver driver = startClientObject(context, CLIENT);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        //2
        TEMP_GAMES_FOLDER_PATH = SystemUtilities.getProgramFilesPath(CLIENT) + "\\Temp Origin Games";
        final String expectedLargeEntitlementPath = SystemUtilities.getOriginGamesPath(CLIENT) + "\\" + largeEntitlementFolderName;
        final String notExpectedLargeEntitlementPath = TEMP_GAMES_FOLDER_PATH + "\\" + largeEntitlementFolderName;
        final String expectedSmallEntitlementPath = TEMP_GAMES_FOLDER_PATH + "\\" + smallEntitlementFolderName;
        final String notExpectedSmallEntitlementPath = SystemUtilities.getOriginGamesPath(CLIENT) + "\\" + smallEntitlementFolderName;

        FileUtil.deleteFolder(CLIENT, TEMP_GAMES_FOLDER_PATH);
        FileUtil.createFolder(CLIENT, TEMP_GAMES_FOLDER_PATH);
        if (FileUtil.isDirectoryExist(CLIENT, TEMP_GAMES_FOLDER_PATH)) {
            logPass("Successfully created a temporary Origin Games folder");
        } else {
            logFailExit("Failed to create a temporary Origin Games folder");
        }

        //3
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigated to the 'Game Library' page");
        } else {
            logFailExit("Failed to navigate to the 'Game Library' page");
        }

        //4
        boolean downloaded = MacroGameLibrary.startDownloadingEntitlement(driver, largeEntitlementOfferId);
        if (downloaded) {
            logPass("Successfully started downloading " + largeEntitlementName);
        } else {
            logFailExit("Failed to start downloading " + largeEntitlementName);
        }

        //5
        AppSettings appSettings = new MainMenu(driver).selectApplicationSettings();
        if (appSettings.verifyAppSettingsReached()) {
            logPass("Successfully navigated to the 'Application Settings' page");
        } else {
            logFailExit("Failed to navigate to the 'Application Settings' page");
        }

        //6
        new SettingsNavBar(driver).clickInstallSaveLink();
        InstallSaveSettings installSaveSettings = new InstallSaveSettings(driver);
        if (Waits.pollingWait(installSaveSettings::verifyInstallSaveSettingsReached)) {
            logPass("Successfully navigated to 'Install & Saves' section");
        } else {
            logFailExit("Failed to navigate to 'Install & Saves' section");
        }

        //7
        installSaveSettings.clickChangeGameFolderLocation();
        RobotKeyboard robotHandler = new RobotKeyboard();
        robotHandler.type(CLIENT, TEMP_GAMES_FOLDER_PATH, 200);
        sleep(1000);
        robotHandler.type(CLIENT, "\n\n", 2000);
        new InstallationSettingsChangedDialog(driver).closeAndWait();
        if (installSaveSettings.verifyGameLibraryLocation(TEMP_GAMES_FOLDER_PATH)) {
            logPass("Successfully changed download location");
        } else {
            logFailExit("Failed to change download location");
        }

        //8
        gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigated to the 'Game Library' page");
        } else {
            logFailExit("Failed to navigate to the 'Game Library' page");
        }

        //9
        GameTile gameTile = new GameTile(driver, largeEntitlementOfferId);
        boolean finishedDownloading = Waits.pollingWait(
                () -> gameTile.isReadyToPlay() && !gameTile.isDownloading(),
                300000, 2000, 0
        ); //wait up to 5 minutes for game to finish downloading
        if (finishedDownloading) {
            logPass("Successfully downloaded and installed " + largeEntitlementName);
        } else {
            logFailExit("Failed to download and install " + largeEntitlementName);
        }

        //10
        boolean downloadedToCorrectLocation = FileUtil.isDirectoryExist(CLIENT, expectedLargeEntitlementPath);
        if (downloadedToCorrectLocation) {
            logPass("Successfully downloaded and installed " + largeEntitlementName + " to the original location");
        } else {
            logFail("Failed to download and install " + largeEntitlementName + " to the original location");
        }

        //11
        downloaded = MacroGameLibrary.downloadFullEntitlement(driver, smallEntitlementOfferId);
        if (downloaded) {
            logPass("Successfully downloaded and installed " + smallEntitlementName);
        } else {
            logFailExit("Failed to download and install " + smallEntitlementName);
        }

        //12
        downloadedToCorrectLocation = FileUtil.isDirectoryExist(CLIENT, expectedSmallEntitlementPath)
                && !(FileUtil.isDirectoryExist(CLIENT, notExpectedSmallEntitlementPath));
        if (downloadedToCorrectLocation) {
            logPass("Successfully downloaded and installed " + smallEntitlementName + " to the new location");
        } else {
            logFail("Failed to download and install " + smallEntitlementName + " to the new location");
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
