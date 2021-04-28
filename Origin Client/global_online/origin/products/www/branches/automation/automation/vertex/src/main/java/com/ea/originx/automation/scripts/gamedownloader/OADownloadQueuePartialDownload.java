package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests partial download in the download queue
 *
 * @author palui
 */
public class OADownloadQueuePartialDownload extends EAXVxTestTemplate {

    @TestRail(caseId = 9642)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testDownloadQueuePartialDownload(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement1 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);
        EntitlementInfo entitlement2 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        EntitlementInfo entitlement3 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.NON_DIP_SMALL);

        final String entitlementName1 = entitlement1.getName();
        final String offerId1 = entitlement1.getOfferId();

        final String entitlementName2 = entitlement2.getName();
        final String offerId2 = entitlement2.getOfferId();

        final String entitlementName3 = entitlement3.getName();
        final String offerId3 = entitlement3.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement1, entitlement2, entitlement3);
        String username = userAccount.getUsername();

        logFlowPoint(String.format("Launch Origin and login as user %s entitled to '%s', '%s' and '%s'",
                username, entitlementName1, entitlementName2, entitlementName3));//1
        logFlowPoint(String.format("Navigate to the Game Library and verify games '%s', '%s' and '%s' appear as downloadable",
                entitlementName1, entitlementName2, entitlementName3)); //2
        logFlowPoint(String.format("Start downloading games '%s', '%s' and '%s' in order",
                entitlementName1, entitlementName2, entitlementName3)); //3
        logFlowPoint(String.format("Pause download of '%s'", entitlementName1)); //4
        logFlowPoint(String.format("Verify game 1 '%s' is the current download and game 2 '%s' and game 3 '%s' are queued for download",
                entitlementName1, entitlementName2, entitlementName3)); //5
        logFlowPoint(String.format("Verify game 1 '%s' is partially downloaded", entitlementName1)); //6

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass(String.format("Verified login successful as user %s", username));
        } else {
            logFailExit(String.format("Failed: Cannot login as user %s", username));
        }

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();
        GameTile gameTile1 = new GameTile(driver, offerId1);
        GameTile gameTile2 = new GameTile(driver, offerId2);
        GameTile gameTile3 = new GameTile(driver, offerId3);
        if (gameTile1.waitForDownloadable() && gameTile2.waitForDownloadable() && gameTile3.waitForDownloadable()) {
            logPass(String.format("Verified successful navigation to Game Library with downloadable games '%s', '%s' and '%s'",
                    entitlementName1, entitlementName2, entitlementName3));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library or cannot locate downloable games '%s', '%s' or '%s'",
                    entitlementName1, entitlementName3, entitlementName3));
        }

        //3
        boolean startDownloading1 = MacroGameLibrary.startDownloadingEntitlement(driver, offerId1);
        boolean startDownloading2 = MacroGameLibrary.startDownloadingEntitlement(driver, offerId2);
        boolean startDownloading3 = MacroGameLibrary.startDownloadingEntitlement(driver, offerId3);
        if (startDownloading1 && startDownloading2 && startDownloading3) {
            logPass(String.format("Verified start downloading successful for '%s', '%s' and '%s'",
                    entitlementName1, entitlementName2, entitlementName3));
        } else {
            logFailExit(String.format("Failed: Start downloading failed for one of '%s', '%s' or '%s'",
                    entitlementName1, entitlementName2, entitlementName3));
        }

        //4
        gameTile1.pauseDownload();
        if (Waits.pollingWait(gameTile1::isPaused)) {
            logPass(String.format("Verified downloading pauses for '%s'", entitlementName1));
        } else {
            logFailExit(String.format("Failed: Cannot pause downloading for '%s'", entitlementName1));
        }

        //5
        new DownloadManager(driver).clickGameTitleToOpenFlyout(); // Click title of game in progress
        DownloadQueueTile queueTile1 = new DownloadQueueTile(driver, offerId1);
        DownloadQueueTile queueTile2 = new DownloadQueueTile(driver, offerId2);
        DownloadQueueTile queueTile3 = new DownloadQueueTile(driver, offerId3);
        boolean entitlement1Downloading = Waits.pollingWait(queueTile1::isGameDownloading);
        boolean entitlement2Queued = Waits.pollingWait(queueTile2::isGameQueued);
        boolean entitlement3Queued = Waits.pollingWait(queueTile3::isGameQueued);

        if (entitlement1Downloading && entitlement2Queued && entitlement3Queued) {
            logPass(String.format("Verified game 1 '%s' is the current download, and game 2 '%s' and game 3 '%s' are queued for download",
                    entitlementName1, entitlementName2, entitlementName3));
        } else {
            logFailExit(String.format("Failed: game 1 '%s' is not the current download, or game 2 '%s' and game 3 '%s' are not both queued for download",
                    entitlementName1, entitlementName2, entitlementName3));
        }

        //6
        int downloadPercentage = queueTile1.getDownloadPercentageValue();
        int progressBarValue = queueTile1.getProgressBarPercentageValue();
        boolean partiallyDownloaded = progressBarValue > 0 && progressBarValue < 100 && downloadPercentage == progressBarValue;
        if (partiallyDownloaded) {
            logPass(String.format("Verified '%s' progress bar indicates it is partially downloaded ", entitlementName1));
        } else {
            logFailExit(String.format("Failed: '%s' progress bar does not indicate it is partially downloaded ", entitlementName1));
        }

        softAssertAll();
    }
}
