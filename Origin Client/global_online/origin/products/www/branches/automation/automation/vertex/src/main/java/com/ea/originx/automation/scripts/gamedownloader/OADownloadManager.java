package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
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
 * Test Download Manager game in progress status matches that of the current
 * 'Download Queue' tile
 *
 * @author palui
 */
public class OADownloadManager extends EAXVxTestTemplate {

    @TestRail(caseId = 9908)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testDownloadManager(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement1 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);
        EntitlementInfo entitlement2 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.NON_DIP_LARGE);

        final String entitlementName1 = entitlement1.getName();
        final String offerId1 = entitlement1.getOfferId();
        final String entitlementName2 = entitlement2.getName();
        final String offerId2 = entitlement2.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement1, entitlement2);
        String username = userAccount.getUsername();

        logFlowPoint(String.format("Launch Origin and login as user %s entitled to '%s' and '%s'",
                username, entitlementName1, entitlementName2));//1
        logFlowPoint(String.format("Navigate to the Game Library and verify games '%s' and '%s' appear as downloadable",
                entitlementName1, entitlementName2)); //2
        logFlowPoint(String.format("Start downloading '%s' and '%s' and verify their 'Download Queue' tiles", entitlementName1, entitlementName2)); //3
        logFlowPoint(String.format("Verify 'Download Manager' matches 'Download Queue' to show '%s' as the current download", entitlementName1)); //4
        logFlowPoint(String.format("Verify 'Download Manager' is showing '%s' with similar percentage progress as the 'Download Queue' tile", entitlementName1)); //5
        logFlowPoint(String.format("Verify 'Download Manager' is showing '%s' with similar downloading time remianing as the 'Download Queue' tile", entitlementName1)); //6
        logFlowPoint(String.format("Click 'Download Now' button at '%s'. Verify 'Download Queue' tiles", entitlementName2)); //7
        logFlowPoint(String.format("Verify 'Download Manager' matches 'Download Queue' to show '%s' as the current download", entitlementName2)); //8
        logFlowPoint(String.format("Verify 'Download Manager' is showing '%s' with similar percentage progress as the 'Download Queue' tile", entitlementName2)); //9
        logFlowPoint(String.format("Verify 'Download Manager' is showing '%s' with similar downloading time remianing as the 'Download Queue' tile", entitlementName2)); //10

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();
        GameTile gameTile1 = new GameTile(driver, offerId1);
        GameTile gameTile2 = new GameTile(driver, offerId2);
        logPassFail(gameTile1.waitForDownloadable() && gameTile2.waitForDownloadable(), true);

        //3
        DownloadManager downloadManager = new DownloadManager(driver);
        // Change download rate to 50MB.
        sendJavascriptCommand(driver, "Origin.client.settings.writeSetting(\"MaxDownloadRateBpsOutOfGame\", \"50000000\");");
        boolean startDownloading1 = MacroGameLibrary.startDownloadingEntitlement(driver, offerId1);
        boolean startDownloading2 = MacroGameLibrary.startDownloadingEntitlement(driver, offerId2);
        new DownloadManager(driver).clickMiniDownloadManagerToOpenFlyout();

        DownloadQueueTile queueTile1 = new DownloadQueueTile(driver, offerId1);
        DownloadQueueTile queueTile2 = new DownloadQueueTile(driver, offerId2);
        boolean entitlement1Downloading = Waits.pollingWait(queueTile1::isGameDownloading);
        boolean entitlement2Queued = Waits.pollingWait(queueTile2::isGameQueued);

        boolean downloading1AndQueued2 = startDownloading1 && startDownloading2 && entitlement1Downloading && entitlement2Queued;
        logPassFail(downloading1AndQueued2, true);

        //4
        logPassFail(downloadManager.verifyInProgressGame(entitlement1), true);

        //5
        logPassFail(MacroGameLibrary.verifyProgressBarSimilar(downloadManager, queueTile1), false);

        //6
        logPassFail(MacroGameLibrary.verifyDownloadTileAndQueueTileTimeSimilar(downloadManager, queueTile1), false);

        //7
        queueTile2.clickDownloadButton();
        boolean entitlement2Downloading = Waits.pollingWait(queueTile2::isGameDownloading);
        boolean entitlement1Queued = Waits.pollingWait(queueTile1::isGameQueued);
        logPassFail(entitlement2Downloading && entitlement1Queued, true);

        //8
        logPassFail(downloadManager.verifyInProgressGame(entitlement2), true);

        //9
        logPassFail(MacroGameLibrary.verifyProgressBarSimilar(downloadManager, queueTile2), false);

        //10
        logPassFail(MacroGameLibrary.verifyDownloadTileAndQueueTileTimeSimilar(downloadManager, queueTile2), false);

        // Change download rate back to "No limit".
        sendJavascriptCommand(driver, "Origin.client.settings.writeSetting(\"MaxDownloadRateBpsOutOfGame\", \"0\");");

        softAssertAll();
    }
}
