package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.*;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.OriginClientData;
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
 * Test to locate game to the same drive/different drive folder location
 * with or without DLC
 * - OALocateGameWithDLCUsingGameTileContextMenu
 * - OALocateGameWithoutDLCUsingGameTileContextMenu
 * - OALocateGameWithDLCUsingOGDGameSettingMenu
 * - OALocateGameWithoutDLCUsingOGDGameSettingMenu
 * @author sunlee
 */
public abstract class OALocateGame extends EAXVxTestTemplate {

    private static final Logger _log = LogManager.getLogger(OAMoveGame.class);

    private String LOCATE_FOLDER_PATH = null;
    private OriginClient CLIENT = null;
    public enum METHOD_TO_LOCATE_GAME {USE_GAME_TILE_CONTEXT_MENU, USE_OGD_GAME_SETTINGS};

    public void testLocateGame(ITestContext context, boolean isDLCNeeded, METHOD_TO_LOCATE_GAME howToMoveGame, String testName) throws Exception {
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

        LOCATE_FOLDER_PATH = SystemUtilities.getProgramFilesPath(CLIENT) + "\\Temp Origin Games";

        logFlowPoint("Create a folder to to locate game"); //1
        logFlowPoint(String.format("Launch and login to Origin client with an account owns a DiP entitlement %s",
                isDLCNeeded? "with dlc : " + baseGameName + " and " + expansionName : ": " + baseGameName)); //2
        logFlowPoint("Navigate to game library"); //3
        logFlowPoint(String.format("Download and install the entitlement %s", isDLCNeeded ? "with DLC" : "")); //4
        logFlowPoint("Log out of Origin"); //5
        logFlowPoint("Move game directory to a different location:" + LOCATE_FOLDER_PATH); //6
        logFlowPoint("Log into Origin client with the same account"); //7
        logFlowPoint("Navigate to game library and verify the entitlement is downloadable"); //8
        if(howToMoveGame == METHOD_TO_LOCATE_GAME.USE_GAME_TILE_CONTEXT_MENU) {
            logFlowPoint("Right click on the entitlement and verify 'Locate Game' option is available"); //9 - 1
        } else if(howToMoveGame == METHOD_TO_LOCATE_GAME.USE_OGD_GAME_SETTINGS) {
            logFlowPoint("Open OGD page of the entitlement"); //9 - 2
            logFlowPoint("Click game setting and verify 'Locate Game' option is available"); //10
        }
        logFlowPoint("Click 'Locate Game' and navigate to moved game folder and select folder, and verify the game start verifying/repairing."); //11
        if(isDLCNeeded) {
            logFlowPoint("Verify DLC queued in download manager"); //12
        }
        logFlowPoint("Wait for repairing process completed and verify the game tile showed completed"); //13
        if(isDLCNeeded) {
            logFlowPoint("Verify DLC verifying/repairing process started"); //14
            logFlowPoint("Wait until verifying/repairing process of DLC is complete and verify download manager showed base game dlc completed"); //15
        }
        logFlowPoint("Verify entitlement can launch through Origin without any issue"); //16
        logFlowPoint("Verify desktop shortcut works"); //17
        logFlowPoint("Verify start menu shortcut works"); //18

        // 1
        if(FileUtil.isDirectoryExist(CLIENT, LOCATE_FOLDER_PATH)) {
            FileUtil.deleteFolder(CLIENT, LOCATE_FOLDER_PATH);
        }
        logPassFail(FileUtil.createFolder(CLIENT, LOCATE_FOLDER_PATH), true);

        // 2
        WebDriver driver = startClientObject(context, CLIENT);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 3
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        // 4
        if(isDLCNeeded) {
            logPassFail(MacroGameLibrary.downloadFullEntitlementWithSelectedDLC(driver, baseGameOfferID,  new DownloadOptions(), expansionOfferIDList), true);
        } else {
            logPassFail(MacroGameLibrary.downloadFullEntitlement(driver, baseGameOfferID), true);
        }
        baseGame.addActivationFile(CLIENT);

        // 5
        LoginPage loginPage = new MiniProfile(driver).selectSignOut();
        logPassFail(Waits.pollingWait(() -> loginPage.isPageThatMatchesOpen(OriginClientData.LOGIN_PAGE_URL_REGEX)), true);

        // 6
        boolean verifyMoveFolder = true;
        String originalGameFolderSize = FileUtil.getFolderSize(CLIENT, baseGame.getDirectoryFullPath(CLIENT));
        String movedGameFolderName = LOCATE_FOLDER_PATH + "\\" + baseGame.getDirectoryName();
        _log.debug(originalGameFolderSize);
        if (originalGameFolderSize!= null && FileUtil.copyFolder(CLIENT, baseGame.getDirectoryFullPath(CLIENT), LOCATE_FOLDER_PATH)
                && FileUtil.isDirectoryExist(CLIENT, movedGameFolderName)
                && FileUtil.getFolderSize(CLIENT, movedGameFolderName).equals(originalGameFolderSize)
                && FileUtil.deleteFolder(CLIENT, baseGame.getDirectoryFullPath(CLIENT))) {
            verifyMoveFolder = !FileUtil.isDirectoryExist(CLIENT, baseGame.getDirectoryFullPath(CLIENT));
        } else {
            verifyMoveFolder = false;
        }
        logPassFail(verifyMoveFolder, true);

        // 7
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 8
        boolean verifyGameLibraryReached = navBar.gotoGameLibrary().verifyGameLibraryPageReached();
        GameTile gameTile =  new GameTile(driver, baseGameOfferID);
        GameSlideout entitlementSlideout = new GameSlideout(driver);
        logPassFail(verifyGameLibraryReached && gameTile.isDownloadable(), true);

        if(howToMoveGame == METHOD_TO_LOCATE_GAME.USE_GAME_TILE_CONTEXT_MENU) {
            // 9 - 1
            logPassFail(gameTile.verifyLocateGameVisible(), true);

            //11
            gameTile.locateGame();
        } else {
            // 9 - 2
            entitlementSlideout = gameTile.openGameSlideout();
            logPassFail(entitlementSlideout.waitForSlideout(), true);

            // 10
            GameSlideoutCogwheel gameSlideoutCogwheel = new GameSlideout(driver).getCogwheel();
            logPassFail(gameSlideoutCogwheel.verifyLocateGameVisible(), true);

            // 11
            gameSlideoutCogwheel.locateGame();
        }

        // 11
        boolean errorDuringRobotKeyBoard = TestScriptHelper.enterPathOnOpenDirectoryDialogUsingRobotKeyboard(CLIENT, LOCATE_FOLDER_PATH);
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
        if(howToMoveGame == METHOD_TO_LOCATE_GAME.USE_GAME_TILE_CONTEXT_MENU) {
            logPassFail(MacroGameLibrary.launchAndExitGame(driver, baseGame, CLIENT), false);
        } else {
            entitlementSlideout.startPlay(15);
            logPassFail(baseGame.isLaunched(CLIENT), false);
            baseGame.killLaunchedProcess(CLIENT);
        }

        // 17
        //disable cloudsave for reduece wait time for cloud sync lock to clear
        TestScriptHelper.setCloudSaveEnabled(driver, false);
        String desktopShortcutPath = OriginClientConstants.DESKTOP_PATH + baseGame.getParentName() + ".lnk";
        DesktopHelper.open(CLIENT, desktopShortcutPath);
        logPassFail(Waits.pollingWaitEx(()->baseGame.isLaunched(CLIENT), 120000, 1000, 10000), false);
        baseGame.killLaunchedProcess(CLIENT);

        // 18
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
        if (null != LOCATE_FOLDER_PATH && null!= CLIENT) {
            FileUtil.deleteFolder(CLIENT, LOCATE_FOLDER_PATH);
            _log.debug("Deleted locate folder");
        }

        _log.debug("Performing test cleanup for " + this.getClass().getSimpleName());
        super.testCleanUp(result, context);
    }
}
