package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadProblemDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.*;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoConstants;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test miscellaneous states of entitlements in the game library
 *
 * @author rmcneil
 * @author rocky
 */
public class OADownloadStates extends EAXVxTestTemplate {

    @TestRail(caseId = 9843)
    @Test(groups = {"gamedownoader", "client_only", "full_regression"})
    public void testDownloadStates(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final String LARGE_DIP = EntitlementInfoConstants.DIP_LARGE_NAME;
        final String LARGE_NON_DIP = EntitlementInfoConstants.NON_DIP_LARGE_NAME;
        final String SMALL_DIP = EntitlementInfoConstants.DIP_SMALL_NAME;
        final String SMALL_NON_DIP = EntitlementInfoConstants.NON_DIP_SMALL_NAME;

        final String LARGE_DIP_ID = EntitlementInfoConstants.DIP_LARGE_OFFER_ID;
        final String LARGE_NON_DIP_ID = EntitlementInfoConstants.NON_DIP_LARGE_OFFER_ID;
        final String SMALL_DIP_ID = EntitlementInfoConstants.DIP_SMALL_OFFER_ID;

        AccountManager accountManager = AccountManager.getInstance();
        Criteria criteria = new Criteria.CriteriaBuilder().build();
        criteria.addEntitlement(SMALL_DIP);
        criteria.addEntitlement(SMALL_NON_DIP);
        criteria.addEntitlement(LARGE_DIP);
        criteria.addEntitlement(LARGE_NON_DIP);
        UserAccount user = accountManager.requestWithCriteria(criteria, 360);
        logFlowPoint("Log into Origin with " + user.getUsername()); // 1
        logFlowPoint("Navigate to Game Library"); // 2
        logFlowPoint("Verify " + LARGE_DIP + " is downloadable"); // 3
        logFlowPoint("Start download of " + LARGE_DIP); // 4
        logFlowPoint("Start download of " + SMALL_DIP); // 5
        logFlowPoint("verify artwork for currently downloading entitlement " + LARGE_DIP); // 6
        logFlowPoint("Verify artwork for " + SMALL_DIP + " which is queued "); // 7
        logFlowPoint("Add " + LARGE_NON_DIP + " to the queue"); // 8
        logFlowPoint("push " + LARGE_NON_DIP + " to the front of the queue."); // 9
        logFlowPoint("Download and install " + SMALL_DIP + " and verify opening slideout cleared download overlay"); // 10
        logFlowPoint("Verify " + LARGE_DIP + " is partially downloaded and queued"); // 11
        logFlowPoint("Uninstall " + SMALL_DIP + " and verify it appears as un-installed in the Game Library"); // 12
        logFlowPoint("Exit Client and update EACore.ini file with wrong download path for " + SMALL_DIP); // 13
        logFlowPoint("Re-login to client"); // 14
        logFlowPoint("Re-download " + SMALL_DIP + " and verify download error dialog shows up"); // 15

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        // 2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigated to the 'Game Library' page");
        } else {
            logFailExit("Failed to navigate to the 'Game Library' page");
        }

        // 3
        // Change download rate to 25MB.
        sendJavascriptCommand(driver, "Origin.client.settings.writeSetting(\"MaxDownloadRateBpsOutOfGame\", \"25000000\");");
        GameTile largeDipTile = new GameTile(driver, LARGE_DIP_ID);
        if (largeDipTile.waitForDownloadable()) {
            logPass("Successfully verified " + LARGE_DIP + " is downloadable");
        } else {
            logFailExit("Could not verify " + LARGE_DIP + " is downloadable");
        }

        // 4
        if (MacroGameLibrary.startDownloadingEntitlement(driver, LARGE_DIP_ID)) {
            logPass("Successfully started downloading " + LARGE_DIP);
        } else {
            logFailExit("Could not start downloading " + LARGE_DIP);
        }

        // 5
        GameTile smallDipTile = new GameTile(driver, SMALL_DIP_ID);
        boolean downloadInitiated = MacroGameLibrary.startDownloadingEntitlement(driver, SMALL_DIP_ID);
        boolean gameQueued = smallDipTile.isQueued();
        if (Waits.pollingWait(() -> downloadInitiated && gameQueued)) {
            logPass(SMALL_DIP + " successfully added to the queue.");
        } else {
            logFailExit(SMALL_DIP + " was not added to the queue.");
        }

        // 6
        DownloadQueueTile dip_large_tile = new DownloadQueueTile(driver, LARGE_DIP_ID);
        if (dip_large_tile.verifyPackart()) {
            logPass("Successfully verified artwork for currently downloading entitlement " + LARGE_DIP);
        } else {
            logFailExit("could not find artwork for currently downloading entitlement " + LARGE_DIP);
        }

        // 7
        DownloadQueueTile dip_small_tile = new DownloadQueueTile(driver, SMALL_DIP_ID);
        if (dip_small_tile.verifyPackart()) {
            logPass("Successfully verified artwork for " + SMALL_DIP + " which is queued");
        } else {
            logFailExit("could not find artwork for " + SMALL_DIP + " which is queued");
        }

        // 8
        GameTile largeNonDipTile = new GameTile(driver, LARGE_NON_DIP_ID);
        final boolean downloadInitiated0 = MacroGameLibrary.startDownloadingEntitlement(driver, LARGE_NON_DIP_ID);
        final boolean gameQueued0 = largeNonDipTile.isQueued();
        if (downloadInitiated0 && gameQueued0) {
            logPass(LARGE_NON_DIP + " successfully added to the queue.");
        } else {
            logFailExit(LARGE_NON_DIP + " was not added to the queue.");
        }

        // 9
        largeNonDipTile.downloadNow();
        gameLibrary.clearDownloadQueue();
        boolean activeDownloadChanged = largeNonDipTile.waitForDownloading();
        boolean gamePaused = largeDipTile.waitForPaused();
        boolean isPartialDownload = largeDipTile.verifyPartialDownload();
        if (activeDownloadChanged && gamePaused && isPartialDownload) {
            logPass(LARGE_NON_DIP + " successfully pushed to the front of the queue.");
        } else {
            logFailExit(LARGE_NON_DIP + " was not pushed to the front of the queue.");
        }

        // 10
        smallDipTile.downloadNow();
        smallDipTile.waitForReadyToPlay(20);
        gameLibrary.clearDownloadQueue();
        // sometimes it clicks game tile before overlay shows up.
        sleep(1000);
        GameSlideout smallDipSlideout = smallDipTile.openGameSlideout();
        smallDipSlideout.waitForSlideout();
        smallDipSlideout.clickSlideoutCloseButton();
        boolean gametileOverlayVisible = smallDipTile.isTileOverlayVisible();
        if (Waits.pollingWait(() -> !gametileOverlayVisible)) {
            logPass(SMALL_DIP + " successfully downloaded and opening slideout cleared download overlay.");
        } else {
            logFailExit(SMALL_DIP + " was not downloaded.");
        }

        // 11
        new DownloadManager(driver).openDownloadQueueFlyout();
        boolean isPartialDownload0 = dip_large_tile.verifyPercentageProgressDownloadedWhenQueued();
        boolean isQueued = dip_large_tile.isGameQueued();
        new DownloadManager(driver).closeDownloadQueueFlyout();
        if (isPartialDownload0 && isQueued) {
            logPass(LARGE_DIP + " successfully verified it is queued and partially downloaded.");
        } else {
            logFailExit("unable to verify " + LARGE_DIP + " it is queued or/and partially downloaded.");
        }

        // 12
        OADipSmallGame dipSmallGame = new OADipSmallGame();
        dipSmallGame.enableSilentUninstall(client);
        smallDipTile.uninstall();
        boolean gameReadyToDownload = smallDipTile.waitForDownloadable();
        boolean gameUninstalled = !smallDipTile.isReadyToPlay();
        if (gameReadyToDownload && gameUninstalled) {
            logPass(String.format("Verified '%s' appears as un-installed in the Game Library", SMALL_DIP));
        } else {
            logFailExit(String.format("Failed: '%s' does not appear as un-installed in the Game Library", SMALL_DIP));
        }

        // 13
        // Change download rate back to "No limit".
        sendJavascriptCommand(driver, "Origin.client.settings.writeSetting(\"MaxDownloadRateBpsOutOfGame\", \"0\");");
        new MainMenu(driver).selectExit();
        boolean originExited = Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, OriginClientData.ORIGIN_PROCESS_NAME));
        boolean eaCoreUpdated = dipSmallGame.setInvalidPathOverride(client);
        if (originExited && eaCoreUpdated) {
            logPass("Successfully exited Origin and updated the EACore with invalid download URL for " + SMALL_DIP);
        } else {
            logFailExit("Failed to exit Origin and/or update the EACore with invalid download URL for " + SMALL_DIP);
        }

        // 14
        client.stop();
        WebDriver driverNew = startClientObject(context, client);
        if (MacroLogin.startLogin(driverNew, user)) {
            logPass("Successfully logged out and back into Origin");
        } else {
            logFailExit("Failed to log out and back into Origin");
        }

        // 15
        new NavigationSidebar(driverNew).gotoGameLibrary();
        new GameLibrary(driverNew).waitForPage();
        smallDipTile = new GameTile(driverNew, SMALL_DIP_ID);
        smallDipTile.startDownload();
        DownloadProblemDialog downloadProblemDialog = new DownloadProblemDialog(driverNew);
        downloadProblemDialog.waitForVisible();
        if (downloadProblemDialog.isOpen()) {
            logPass("Verify it shows download error dialog");
        } else {
            logFailExit("Failed to show download error dialog");
        }

        softAssertAll();
    }
}
