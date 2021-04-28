package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CancelDownload;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the Global Progress bar
 *
 * @author mkalaivanan
 */
public class OAGlobalProgressSideBar extends EAXVxTestTemplate {

    @TestRail(caseId = 9819)
    @Test(groups = {"gamedownloader", "client_only", "full_regression", "int_only"})
    public void testGlobalProgressSideBar(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo dipLarge = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);
        final String dipLargeName = dipLarge.getName();
        final String dipLargeOfferId = dipLarge.getOfferId();

        OADipSmallGame dipSmall = new OADipSmallGame();
        final String dipSmallName = dipSmall.getName();
        final String dipSmallOfferId = dipSmall.getOfferId();

        final EntitlementInfo piEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.PI);
        final String piEntitlementName = piEntitlement.getName();
        final String piOfferId = piEntitlement.getOfferId();

        AccountManager accountManager = AccountManager.getInstance();
        Criteria criteria = new Criteria.CriteriaBuilder().build();
        criteria.addEntitlement(dipLargeName);
        criteria.addEntitlement(piEntitlementName);
        criteria.addEntitlement(dipSmall.getName());
        UserAccount userAccount = accountManager.requestWithCriteria(criteria, 360);

        logFlowPoint("Set EACore Override for PI  and Log into Origin as " + userAccount.getUsername()); //1
        logFlowPoint("Navigate to the 'Game Library' page"); //2
        logFlowPoint("Start downloading " + piEntitlementName); //3
        logFlowPoint("Verify there is a visual indication in download queue (ongoing download) when the game will become playable"); //4
        logFlowPoint("Verify that there is visual confirmation to user when the game becomes playable"); //5
        logFlowPoint("Verify user cannot pause the download of the game while user is playing the game"); //6
        logFlowPoint("Verify user is not able to cancel the download while the game is playing and downloading"); //7
        logFlowPoint("Very global progress percentage mathces the current download percentage on the game tile"); //8
        logFlowPoint("Quit playing PI and cancel the download");//9
        logFlowPoint("Start downloading " + dipSmallName + " and Verify the download progress in Global Progress bar disappears after successful download and installation"); //10
        logFlowPoint("Start downloading " + dipLargeName); //11
        logFlowPoint("Verify " + dipLargeName + "appears to be paused on the Global progress bar when download is paused from the Game details page"); //12

        // 1
        EACoreHelper.changeToBaseEntitlementPIOverride(client.getEACore());
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("EACore Override added for PI and Successfully logged into Origin with the user");
        } else {
            logFailExit("Could not log into Origin with the user");
        }

        //2
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Navigate to game library");
        } else {
            logFailExit("Could not navigate to game library");
        }

        //3
        boolean downloaded = MacroGameLibrary.startDownloadingEntitlement(driver, piOfferId);
        DownloadQueueTile queueItem = new DownloadQueueTile(driver, piOfferId);
        DownloadManager downloadManager = new DownloadManager(driver);
        downloadManager.clickRightArrowToOpenFlyout();    
        boolean visualIndicationExistsWhenGameIsPlayable = Waits.pollingWait(() -> (queueItem.verifyHasPlayableText("Playable at")));
        if (downloaded) {
            logPass("Successfully started downloading " + piEntitlementName);
        } else {
            logFailExit("Failed to start downloading " + piEntitlementName);
        }

        //4
        if (visualIndicationExistsWhenGameIsPlayable) {
            logPass("Visual indication in download queue (ongoing download) when the game will become playable is visible ");
        } else {
            logFail("Visual indication in download queue for ongoing download as as to when game will become playable does not exists");
        }

        GameTile gameTile = new GameTile(driver, piOfferId);
        Waits.pollingWait(() -> gameTile.isDownloading() && gameTile.isReadyToPlay(),
                180000, 2000, 0);

        //5
        boolean visualIndicationOnGlobalProgressBarWhenPlayableState = downloadManager.verifyPlayableTipText()
                && downloadManager.verifyCurrentDownloadIsPlayableNow();

        if (visualIndicationOnGlobalProgressBarWhenPlayableState) {
            logPass("Visual indication in download queue exists when game is in playable state");
        } else {
            logFailExit("Visual Indication when game in playable state does not exist on the download queue or on the global progress side bar");
        }

        //6
        gameTile.play();
        Waits.pollingWaitEx(() -> piEntitlement.isLaunched(client));
        downloadManager.clickGameTitleToOpenFlyout();
        boolean isPauseButtonDisabled = Waits.pollingWait(() -> queueItem.isPauseButtonDisabled());
        if (isPauseButtonDisabled) {
            logPass("User cannot stop or pause the download of the game while playing the game from the Queue");
        } else {
            logFailExit("User is able to stop or pause the download while the game is being played from the download queue");
        }

        //7
        boolean isCancelButtonDisabled = Waits.pollingWait(() -> queueItem.isDownloadCancelButtonDisabled());
        if (isCancelButtonDisabled) {
            logPass("User cannot cancel the dowload while the game is being played from the download queue");
        } else {
            logFailExit("User is able to cancel the download while the game is being played from the download queue");
        }

        //8
        int gameTiledownloadPercentage = gameTile.getDownloadPercentage();
        int globalProgressBarPercentage = downloadManager.getProgressBarValue();
        boolean isDownloadProgressVisible = Math.abs(globalProgressBarPercentage - gameTiledownloadPercentage) <= 4;
        if (isDownloadProgressVisible) {
            logPass("Global progress bar matches the current dowload percentage on the game tile");
        } else {
            logFailExit("Global progress indicator does not match the download percentage on the game tile ");
        }
        //9
        piEntitlement.killLaunchedProcess(client);
        gameTile.cancelDownload();
        //Wait for the cancel download dialog to appear
        sleep(2000);
        CancelDownload cancelDialog = new CancelDownload(driver);
        cancelDialog.isOpen();
        cancelDialog.clickYes();
        if (Waits.pollingWait(() -> !downloadManager.isDownloadManagerVisible())) {
            logPass("Download cancelled successfully and global progress bar is not visible anymore");
        } else {
            logFailExit("Could not cancel Download");
        }

        //10
        GameTile dipSmallgameTile = new GameTile(driver, dipSmallOfferId);
        boolean downloadDipSmallStarted = MacroGameLibrary.downloadFullEntitlement(driver, dipSmallOfferId);
        if (downloadDipSmallStarted) {
            logPass("Started to download " + dipSmall.getName() + " successfully");
        } else {
            logFailExit(dipSmall.getName() + " entitltement failed to download");
        }

        //11
        Waits.pollingWait(() -> dipSmallgameTile.isReadyToPlay());
        if (downloadManager.verifyDownloadCompleteCountEquals("1")) {
            logPass("Current Download on the global progress bar has disappeared after successfully download and installation");
        } else {
            logFailExit("Current download is still visible even after the download and installation was successful");
        }

        //12
        boolean startDipLargeDownload = MacroGameLibrary.startDownloadingEntitlement(driver, dipLargeOfferId);
        GameTile dipLargeGameTile = new GameTile(driver, dipLargeOfferId);
        GameSlideout dipLargeSlideout = dipLargeGameTile.openGameSlideout();
        dipLargeSlideout.pauseGameDownload(OriginClientData.PAUSE_DOWNLOAD_TIMEOUT_SEC);
        Waits.pollingWait(() -> dipLargeGameTile.isPaused());
        if (startDipLargeDownload & downloadManager.verifyGameIsPaused()) {
            logPass("Started downloading " + dipLargeName + " and Verified ongoing download is in paused state in Global progress sidebar when paused from the Game details page");
        } else {
            logFailExit("Downloading " + dipLargeName + " failed");
        }
        softAssertAll();
    }

}
