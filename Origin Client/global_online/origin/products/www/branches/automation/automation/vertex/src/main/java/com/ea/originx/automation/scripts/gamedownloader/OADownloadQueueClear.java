package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.*;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
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
 * Tests the queued game is cleared when the clear button is clicked
 *
 * @author nvarthakavi
 */
public class OADownloadQueueClear extends EAXVxTestTemplate {

    @TestRail(caseId = 9943)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testDownloadQueueClear(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement1 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        EntitlementInfo entitlement2 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BEJEWELED3);
        EntitlementInfo entitlement3 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);

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
        logFlowPoint(String.format("Start downloading game 1 '%s' and Verify the download is complete ", entitlementName1)); //3
        logFlowPoint(String.format("Start downloading game 2 '%s' and game 3 '%s' and Verify the game 2 '%s' is downloading and game 3 '%s' is queued for download",
                entitlementName2, entitlementName3, entitlementName2, entitlementName3)); //4
        logFlowPoint(String.format("Verify game 1 '%s' is removed from download queue", entitlementName1)); //5
        logFlowPoint(String.format("Verified game 2 '%s' is downloaded completely and installed", entitlementName2)); //6
        logFlowPoint("Logout of the Client"); //7
        logFlowPoint(String.format("Login to Client as user %s entitled", username)); //8
        logFlowPoint(String.format("Verify game 2 '%s' is removed from download queue", entitlementName2)); //9

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();
        GameTile gameTile1 = new GameTile(driver, offerId1);
        GameTile gameTile2 = new GameTile(driver, offerId2);
        GameTile gameTile3 = new GameTile(driver, offerId3);
        logPassFail(gameTile1.waitForDownloadable() && gameTile2.waitForDownloadable() && gameTile3.waitForDownloadable(), true);

        //3
        MacroGameLibrary.downloadFullEntitlement(driver, offerId1);
        DownloadQueueTile queueTile1 = new DownloadQueueTile(driver, offerId1);
        DownloadManager downloadManager = new DownloadManager(driver);
        downloadManager.clickDownloadCompleteCountToOpenFlyout();        
        boolean entitlement1DownloadComplete = Waits.pollingWait(queueTile1::isGameDownloadComplete);
        logPassFail(entitlement1DownloadComplete, true);

        //4
        // Close Download Queue Flyout where appropriate, as it may interfere with Game Library operations
        downloadManager.closeDownloadQueueFlyout();
        MacroGameLibrary.startDownloadingEntitlement(driver, offerId2);
        MacroGameLibrary.startDownloadingEntitlement(driver, offerId3);
        downloadManager.clickRightArrowToOpenFlyout();        
        DownloadQueueTile queueTile2 = new DownloadQueueTile(driver, offerId2);
        DownloadQueueTile queueTile3 = new DownloadQueueTile(driver, offerId3);
        boolean entitlement2Downloading = Waits.pollingWait(queueTile2::isGameDownloading);
        boolean entitlement3Queued = Waits.pollingWait(queueTile3::isGameQueued);
        logPassFail(entitlement2Downloading && entitlement3Queued, true);

        //5
        new DownloadQueue(driver).clearCompletedDownloads();
        logPassFail(!queueTile1.verifyIsInQueue(), true);

        //6
        gameTile2.waitForReadyToPlay(300);
        boolean entitlement2DownloadComplete = Waits.pollingWait(() -> queueTile2.isGameDownloadComplete(), 40000, 500, 0);
        logPassFail(entitlement2DownloadComplete, true);

        //7
        LoginPage loginPage = new MainMenu(driver).selectLogOut();
        logPassFail(loginPage.isOnLoginPage(), true);

        //8
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //9
        new DownloadManager(driver).clickRightArrowToOpenFlyout();
        sleep(2000);
        boolean entitlement3Downloading = Waits.pollingWait(() -> queueTile3.isGameDownloading(), 40000, 500, 0);
        logPassFail(!queueTile2.verifyIsInQueue() && entitlement3Downloading, true);

        softAssertAll();
    }
}