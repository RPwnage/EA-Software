package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideoutCogwheel;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.Sims4;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.DesktopHelper;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.FileUtil;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.vx.originclient.utils.Waits;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.ITestResult;
import org.testng.annotations.AfterMethod;

import java.util.Arrays;
import java.util.List;

/**
 * Test to move game to the same drive/different drive folder location
 * with or without DLC
 * - OAMoveGameWithoutDLCToSameDriveUsingGameTileContextMenu
 * - OAMoveGameWithDLCToSameDriveUsingGameTileContextMenu
 * - OAMoveGameWithoutDLCToDifferentDriveUsingGameTileContextMenu
 * - OAMoveGameWithDLCToDifferentDriveUsingGameTileContextMenu
 * - OAMoveGameWithoutDLCToSameDriveUsingOGDGameSettingMenu
 * - OAMoveGameWithDLCToSameDriveUsingOGDGameSettingMenu
 * - OAMoveGameWithoutDLCToDifferentDriveUsingOGDGameSettingMenu
 * - OAMoveGameWithDLCToDifferentDriveUsingOGDGameSettingMenu
 * @author sunlee
 */
public abstract class OAMoveGame extends EAXVxTestTemplate {

    private static final Logger _log = LogManager.getLogger(OAMoveGame.class);

    private String MOVE_FOLDER_PATH = null;
    private String VIRTUAL_DISK_FOLDER = null;
    private OriginClient CLIENT = null;
    public enum METHOD_TO_MOVE_GAME {USE_GAME_TILE_CONTEXT_MENU, USE_OGD_GAME_SETTINGS};

    /**
     * The main test function which all parameterized test cases call.
     * @param context                   The context we are using
     * @param isDLCNeeded               If it's true dlc is needed. False otherwise
     * @param isGameMovedToSameDrive    If it's true the game is moved to same drive. False move to different drive
     * @param howToMoveGame             How to move game
     * @param testName                  The name of the test. We need to pass this up so
     * @throws Exception
     */
    public void testMoveGame(ITestContext context, boolean isDLCNeeded, boolean isGameMovedToSameDrive, METHOD_TO_MOVE_GAME howToMoveGame, String testName) throws Exception {
        CLIENT = OriginClientFactory.create(context);

        String baseGameOfferID, baseGameName, expansionOfferID = "", expansionName = "";
        final EntitlementInfo baseGame;
        final List<String> expansionOfferIDList;

        if (isDLCNeeded) {
            // Use The Sims 4 standard edition and DLC - 'City living'
            baseGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);
            baseGameOfferID = baseGame.getOfferId();
            baseGameName = baseGame.getName();
            expansionOfferID = Sims4.SIMS4_CITY_LIVING_NAME_OFFER_ID;
            expansionName = Sims4.SIMS4_CITY_LIVING_NAME;
        } else {
            // Use Bejeweled 3
            baseGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BEJEWELED3);
            baseGameName = baseGame.getName();
            baseGameOfferID = baseGame.getOfferId();
        }
        expansionOfferIDList = Arrays.asList(expansionOfferID);

        UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.ENTITLEMENT_1103); // Use 1103 entitlement as this test requires launching game 3 times.

        MOVE_FOLDER_PATH = isGameMovedToSameDrive ? SystemUtilities.getProgramFilesPath(CLIENT) :
                OriginClientConstants.NEW_VIRTUAL_DRIVE_LETTER + ":";
        MOVE_FOLDER_PATH += "\\Temp Origin Games";
        final int diskSize;
        if(isDLCNeeded) {
            diskSize = 16500;
        } else {
            diskSize = 1000;
        }

        if(!isGameMovedToSameDrive) {
            logFlowPoint(String.format("Create a partition on the disk of size %d GB and assign to W: Drive", diskSize/1000)); //1
        }
        logFlowPoint("Create a folder to move game"); //2
        logFlowPoint(String.format("Launch and login to Origin client with an account owns a DiP entitlement %s",
                isDLCNeeded? "with dlc : " + baseGameName + " and " + expansionName : ": " + baseGameName)); //3
        logFlowPoint("Navigate to game library"); //4
        logFlowPoint(String.format("Download and install the entitlement %s", isDLCNeeded ? "with DLC" : "")); //5
        logFlowPoint("Verify the entitlement is playable"); //6
        logFlowPoint("Log out and log into Origin"); //7
        logFlowPoint("Navigate to game library"); //8
        if(howToMoveGame == METHOD_TO_MOVE_GAME.USE_GAME_TILE_CONTEXT_MENU) {
            logFlowPoint("Right click on the entitlement and click 'Move Game'"); //9 - 1
        } else if(howToMoveGame == METHOD_TO_MOVE_GAME.USE_OGD_GAME_SETTINGS) {
            logFlowPoint("Open OGD page of the entitlement"); //9 - 2
            logFlowPoint("Click game setting and select 'Move Game' option"); //10
        }
        logFlowPoint(String.format("Navigate to folder %s and select folder and verify file verification/repairing of the base game started", MOVE_FOLDER_PATH)); //11
        if(isDLCNeeded) {
            logFlowPoint("Verify DLC queued in download manager"); //12
        }
        logFlowPoint("Wait until verifying/repairing process of the base game is complete and verify it show completed in download manager and in game tile"); //13
        if(isDLCNeeded) {
            logFlowPoint("Verify DLC verifying/repairing process started"); //14
            logFlowPoint("Wait until verifying/repairing process of DLC is complete and verify download manager showed base game dlc completed"); //15
        }
        logFlowPoint("Verify entitlement can launch through Origin without any issue"); //16
        logFlowPoint("Check all game files and folders are moved from source to destination folder");//17
        logFlowPoint("Verify desktop shortcut works"); //18
        logFlowPoint("Verify start menu shortcut works"); //19

        // 1
        if (!isGameMovedToSameDrive) {
            VIRTUAL_DISK_FOLDER = OriginClientConstants.VIRTUAL_DISK_LOCATION;
            FileUtil.createFolder(CLIENT, VIRTUAL_DISK_FOLDER);
            FileUtil.createVirtualHardDrive(CLIENT, VIRTUAL_DISK_FOLDER, OriginClientConstants.NEW_VIRTUAL_DRIVE_LETTER, diskSize);
            logPassFail(Waits.pollingWait(()->FileUtil.isDirectoryExist(CLIENT, OriginClientConstants.NEW_VIRTUAL_DRIVE_LETTER + ":\\"), 300000, 5000, 0), true);
        }

        // 2
        logPassFail(FileUtil.createFolder(CLIENT, MOVE_FOLDER_PATH), true);

        // 3
        WebDriver driver = startClientObject(context, CLIENT);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 4
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        // 5
        if(isDLCNeeded) {
            logPassFail(MacroGameLibrary.downloadFullEntitlementWithSelectedDLC(driver, baseGameOfferID,  new DownloadOptions(), expansionOfferIDList), true);
        } else {
            logPassFail(MacroGameLibrary.downloadFullEntitlement(driver, baseGameOfferID), true);
        }
        baseGame.addActivationFile(CLIENT);

        // 6
        logPassFail(new GameTile(driver, baseGameOfferID).isReadyToPlay(), true);
        long originalGameFolderSize = Long.parseLong(FileUtil.getFolderSize(CLIENT, baseGame.getDirectoryFullPath(CLIENT)));

        // 7
        new MiniProfile(driver).selectSignOut();
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 8
        gameLibrary = navBar.gotoGameLibrary();

        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);
        GameTile gameTile = new GameTile(driver, baseGameOfferID);
        GameSlideout entitlementSlideout = new GameSlideout(driver);

        if(howToMoveGame == METHOD_TO_MOVE_GAME.USE_GAME_TILE_CONTEXT_MENU) {
            // 9 - 1
            logPassFail(gameTile.verifyMoveGameVisible(), true);
            gameTile.moveGame();
        } else {
            // 9 - 2
            entitlementSlideout = gameTile.openGameSlideout();
            logPassFail(entitlementSlideout.waitForSlideout(), true);

            // 10
            GameSlideoutCogwheel gameSlideoutCogwheel = new GameSlideout(driver).getCogwheel();
            logPassFail(gameSlideoutCogwheel.verifyMoveGameVisible(), true);
            gameSlideoutCogwheel.moveGame();
        }

        // 11
        boolean errorDuringRobotKeyBoard = TestScriptHelper.enterPathOnOpenDirectoryDialogUsingRobotKeyboard(CLIENT, MOVE_FOLDER_PATH);
        DownloadManager miniDownloadManager = new DownloadManager(driver);
        miniDownloadManager.openDownloadQueueFlyout();
        DownloadQueueTile baseDownloadQueueTile = new DownloadQueueTile(driver, baseGameOfferID);
        logPassFail(!errorDuringRobotKeyBoard && baseDownloadQueueTile.isGameDownloading(), true);

        // 12
        if(isDLCNeeded) {
            logPassFail(new DownloadQueueTile(driver, expansionOfferID).isGameQueued(), true);
        }

        // 13
        MacroGameLibrary.finishDownloadingEntitlement(driver, baseGameOfferID);
        boolean isDownloadCompletedOnDownloadManager = baseDownloadQueueTile.isGameDownloadComplete();
        boolean isDownloadCompletedOnGameTile = gameTile.verifyOverlayStatusComplete();
        logPassFail(isDownloadCompletedOnDownloadManager && isDownloadCompletedOnGameTile, true);

        if(isDLCNeeded) {
            // 14
            DownloadQueueTile expansionDownloadQueueTile = new DownloadQueueTile(driver, expansionOfferID);
            logPassFail(Waits.pollingWait(() -> expansionDownloadQueueTile.isGameDownloading()) ||
                    expansionDownloadQueueTile.isGameDownloadComplete(), true);

            // 15
            MacroGameLibrary.finishDownloadingSelectedDLC(driver, expansionOfferIDList);
            logPassFail(expansionDownloadQueueTile.waitForGameDownloadToComplete() &&
                    miniDownloadManager.verifyDownloadCompleteCountEquals("2"), true);
        }

        // 16
        if(howToMoveGame == METHOD_TO_MOVE_GAME.USE_GAME_TILE_CONTEXT_MENU) {
            logPassFail(MacroGameLibrary.launchAndExitGame(driver, baseGame, CLIENT), false);
        } else {
            entitlementSlideout.startPlay(15);
            logPassFail(baseGame.isLaunched(CLIENT), false);
            baseGame.killLaunchedProcess(CLIENT);
        }

        // 17
        // Because of installer log file, original folder and moved game folder size won't be the same
        long movedGameFolderSize = Long.parseLong(FileUtil.getFolderSize(CLIENT, MOVE_FOLDER_PATH + "\\" + baseGame.getDirectoryName()));
        long diffPercent = (originalGameFolderSize - movedGameFolderSize) * 100 / originalGameFolderSize;
        logPassFail(diffPercent <= 1
                && !FileUtil.isDirectoryExist(CLIENT, baseGame.getDirectoryFullPath(CLIENT)), false);

        // 18
        //disable cloudsave for reduece wait time for cloud sync lock to clear
        TestScriptHelper.setCloudSaveEnabled(driver, false);
        String desktopShortcutPath = OriginClientConstants.DESKTOP_PATH + baseGame.getParentName() + ".lnk";
        DesktopHelper.open(CLIENT, desktopShortcutPath);
        logPassFail(Waits.pollingWaitEx(()->baseGame.isLaunched(CLIENT), 120000, 1000, 10000), false);
        baseGame.killLaunchedProcess(CLIENT);

        // 19
        new GameTile(driver, baseGameOfferID).isReadyToPlay();
        String startMenuShortcutPath = OriginClientConstants.START_MENU_PATH + baseGame.getDirectoryName() + "\\" + baseGame.getParentName() + ".lnk";
        DesktopHelper.open(CLIENT, startMenuShortcutPath);
        logPassFail(Waits.pollingWaitEx(()->baseGame.isLaunched(CLIENT), 120000, 1000, 10000), false);
        baseGame.killLaunchedProcess(CLIENT);

        softAssertAll();
    }

    @Override
    @AfterMethod(alwaysRun = true)
    protected void testCleanUp(ITestResult result, ITestContext context) {
        if (null != VIRTUAL_DISK_FOLDER && null!= CLIENT) {
            FileUtil.deleteVirtualHardDrive(CLIENT);
            Waits.pollingWait(()->FileUtil.isDirectoryExist(CLIENT, OriginClientConstants.NEW_VIRTUAL_DRIVE_LETTER + ":\\"));
            FileUtil.deleteFolder(CLIENT, VIRTUAL_DISK_FOLDER);
            _log.debug("Deleted virtual drive and folder");
        } else if (null != MOVE_FOLDER_PATH && null != CLIENT) {
            FileUtil.deleteFolder(CLIENT, MOVE_FOLDER_PATH);
            _log.debug("Deleted move folder");
        }

        _log.debug("Performing test cleanup for " + this.getClass().getSimpleName());
        super.testCleanUp(result, context);
    }
}
