package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.OfflineFlyout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the download queue for PI when switching between offline and online
 * mode as well as when launching PI
 *
 * @author lscholte
 */
public class OAOfflineModeQueuePI extends EAXVxTestTemplate {

    @TestRail(caseId = 9611)
    @Test(groups = {"gamedownloader", "client_only", "full_regression", "int_only"})
    public void testDownloadPI(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo pi = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.PI);
        final String piName = pi.getName();
        final String piOfferId = pi.getOfferId();
        final String piProcess = pi.getProcessName();

        final EntitlementInfo dipLarge = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);
        final String dipLargeName = dipLarge.getName();
        final String dipLargeOfferId = dipLarge.getOfferId();

        final AccountManager accountManager = AccountManager.getInstance();
        Criteria criteria = new Criteria.CriteriaBuilder().build();
        criteria.addEntitlement(piName);
        criteria.addEntitlement(dipLargeName);
        UserAccount user = accountManager.requestWithCriteria(criteria, 360);

        logFlowPoint("Log into Origin as " + user.getUsername()); //1
        logFlowPoint("Navigate to the 'Game Library' page"); //2
        logFlowPoint("Begin downloading " + piName); //3
        logFlowPoint("Wait for " + piName + " to finish installing and become playable"); //4
        logFlowPoint("Pause the download for " + piName); //5
        logFlowPoint("Begin downloading " + dipLargeName); //6
        logFlowPoint("Move " + dipLargeName + " to the front of the download queue"); //7
        logFlowPoint("Go into offline mode"); //8
        logFlowPoint("Launch " + piName); //9
        logFlowPoint("Verify " + piName + " moves to the front of the download queue"); //10
        logFlowPoint("Go back to online mode"); //11
        logFlowPoint("Verify " + piName + " begins downloading"); //12
        logFlowPoint("Go back to offline mode"); //13
        logFlowPoint("Verify the " + piName + " download is paused"); //14
        logFlowPoint("Close " + piName); //15
        logFlowPoint("Verify " + piName + " is playable"); //16

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin with the user");
        } else {
            logFailExit("Could not log into Origin with the user");
        }

        //2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigated to the 'Game Library' page");
        } else {
            logFailExit("Failed to navigate to the 'Game Library' page");
        }

        //3
        // Change download rate to 5MB
        TestScriptHelper.setMaxDownloadRateBpsOutOfGame(driver, "5000000");
        boolean startedDownloading = MacroGameLibrary.startDownloadingEntitlement(driver, piOfferId);
        if (startedDownloading) {
            logPass("Successfully started downloading " + piName);
        } else {
            logFailExit("Failed to start downloading " + piName);
        }

        //4
        GameTile piGameTile = new GameTile(driver, piOfferId);
        boolean isFinishedInstalling = Waits.pollingWait(() -> piGameTile.isDownloading() && piGameTile.isReadyToPlay(),
                180000, 2000, 0);
        if (isFinishedInstalling) {
            logPass("Successfully waited for " + piName + " to finish installing and become playable");
        } else {
            logFailExit("Failed to wait for " + piName + " to finish installing and become playable");
        }

        //5
        piGameTile.pauseDownload();
        if (piGameTile.waitForPaused()) {
            logPass("Successfully paused the download of " + piName);
        } else {
            logFailExit("Failed to pause the download of " + piName);
        }

        //6
        startedDownloading = MacroGameLibrary.startDownloadingEntitlement(driver, dipLargeOfferId);
        if (startedDownloading) {
            logPass("Successfully started downloading " + dipLargeName);
        } else {
            logFailExit("Failed to start downloading " + dipLargeName);
        }

        //7
        GameTile dipLargeGameTile = new GameTile(driver, dipLargeOfferId);
        dipLargeGameTile.downloadNow();
        if (Waits.pollingWait(() -> !dipLargeGameTile.verifyOverlayStatusQueued() && piGameTile.verifyOverlayStatusQueued())) {
            logPass("Successfully moved " + dipLargeName + " to the front of the download queue");
        } else {
            logFailExit("Failed to move " + dipLargeName + " to the front of the download queue");
        }

        //8
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectGoOffline();
        if (mainMenu.verifyOfflineModeButtonVisible()) {
            logPass("Successfully entered offline mode");
        } else {
            logFailExit("Failed to enter offline mode");
        }
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60);

        //9
        //The offline flyout might be blocking the game tile, so close it
        new OfflineFlyout(driver).closeOfflineFlyout(); // close 'Offline flyout' which may interfere with game tiles operations
        sleep(1000); //Brief sleep to let offline flyout close

        piGameTile.play();
        boolean piLaunched = Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, piProcess));
        if (piLaunched) {
            logPass("Successfully launched " + piName);
        } else {
            logFailExit("Failed to launch " + piName);
        }

        //10
        if (Waits.pollingWait(() -> !piGameTile.verifyOverlayStatusQueued() && dipLargeGameTile.verifyOverlayStatusQueued())) {
            logPass(piName + " was moved to the front of the download queue");
        } else {
            logFailExit(piName + " was not moved to the front of the download queue");
        }

        //11
        mainMenu.selectGoOnline();
        if (!mainMenu.verifyOfflineModeButtonVisible()) {
            logPass("Successfully entered online mode");
        } else {
            logFailExit("Failed to enter online mode");
        }
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60);

        //12
        if (Waits.pollingWait(() -> piGameTile.isDownloading() && !piGameTile.isPaused())) {
            logPass(piName + " automatically started downloading");
        } else {
            logFail(piName + " did not automatically start downloading");
        }

        //13
        mainMenu.selectGoOffline();
        if (mainMenu.verifyOfflineModeButtonVisible()) {
            logPass("Successfully entered offline mode");
        } else {
            logFailExit("Failed to enter offline mode");
        }
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60);

        //14
        if (piGameTile.waitForPaused()) {
            logPass("The download of " + piName + " was paused");
        } else {
            logFail("The download of " + piName + " was not paused");
        }

        //15
        // Change download rate back to "No limit".
        TestScriptHelper.setMaxDownloadRateBpsOutOfGame(driver, "0");
        ProcessUtil.killProcess(client, piProcess);
        boolean piClosed = !ProcessUtil.isProcessRunning(client, piProcess);
        if (piClosed) {
            logPass("Successfully closed " + piName);
        } else {
            logFailExit("Failed to close " + piName);
        }

        //16
        //wait for the client to be responsive after the entitlement is closed.
        Waits.sleep(1000);
        if (piGameTile.isReadyToPlay()) {
            logPass(piName + " is ready to play");
        } else {
            logFail(piName + " is not ready to play");
        }

        softAssertAll();
    }
}
