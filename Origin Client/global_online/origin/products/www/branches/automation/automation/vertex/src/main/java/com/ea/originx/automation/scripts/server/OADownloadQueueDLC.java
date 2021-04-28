package com.ea.originx.automation.scripts.server;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import java.util.Arrays;
import java.util.List;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests DLC in download queue
 *
 * @author palui
 * @author mdobre
 */
public class OADownloadQueueDLC extends EAXVxTestTemplate {

    @TestRail(caseId = 1016726)
    @Test(groups = {"server", "client_only", "services_smoke"})
    public void testDownloadQueueDLC(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo baseGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_DIGITAL_DELUXE);
        final String baseOfferId = baseGame.getOfferId();
        
        EntitlementInfo dlcGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_DLC_DIGITAL_SOUNDTRACK);
        final String dlcOfferId = dlcGame.getOfferId();
        List<String> checkExtraContentList = Arrays.asList(dlcOfferId);
        
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.ENTITLEMENTS_DOWNLOAD_PLAY_TEST_ACCOUNT);

        logFlowPoint(String.format("Launch Origin and login as user entitled to base game."));// 1
        logFlowPoint(String.format("Navigate to game library and locate base game game tile.")); // 2
        logFlowPoint(String.format("Start downloading base game. Verify DLC  is queued for download.")); // 3
        logFlowPoint(String.format("Finish downloading base game. Verify it is playable.")); // 4
        logFlowPoint(String.format("Verify DLC is downloading or downloaded.")); //5

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();
        GameTile baseGameTile = new GameTile(driver, baseOfferId);
        logPassFail(baseGameTile.waitForDownloadable(), true);

        // 3 
        boolean startDownloading = MacroGameLibrary.startDownloadingEntitlement(driver, baseOfferId,
                new DownloadOptions().setUncheckExtraContent(true), checkExtraContentList);

        DownloadManager downloadManager = new DownloadManager(driver);
        downloadManager.openDownloadQueueFlyout();
        DownloadQueueTile baseQueueTile = new DownloadQueueTile(driver, baseOfferId);
        DownloadQueueTile dlcQueueTile = new DownloadQueueTile(driver, dlcOfferId);
        new DownloadManager(driver).clickRightArrowToOpenFlyout();
        boolean baseDownloading = startDownloading && Waits.pollingWait(baseQueueTile::isGameDownloading);
        boolean dlcQueued =  Waits.pollingWait(() -> dlcQueueTile.isGameQueued());
        logPassFail(baseDownloading && dlcQueued, true);

        // 4
        boolean finishDownloading = MacroGameLibrary.finishDownloadingEntitlement(driver, baseOfferId);
        boolean baseDownloadComplete = finishDownloading && baseQueueTile.isGameDownloadComplete();
        boolean readyToPlay = new GameTile(driver, baseOfferId).isReadyToPlay();
        logPassFail(baseDownloadComplete && readyToPlay, true);

        // 5
        boolean dlcDownloading = dlcQueueTile.isGameDownloading();
        // adding this condition due to timing, downloading status might not be available
        // since download of DLC is done very fast
        boolean dlcDownloaded = Waits.pollingWait(() -> dlcQueueTile.isGameDownloadComplete());
        logPassFail(dlcDownloading || dlcDownloaded, true);

        softAssertAll();
    }
}