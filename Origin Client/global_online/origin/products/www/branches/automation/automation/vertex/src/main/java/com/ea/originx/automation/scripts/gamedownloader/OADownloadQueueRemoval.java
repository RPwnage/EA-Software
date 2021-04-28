package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
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
 * Test download queue flyout with removal
 *
 * @author palui
 */
public class OADownloadQueueRemoval extends EAXVxTestTemplate {

    @TestRail(caseId = 9941)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testDownloadQueueRemoval(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        OADipSmallGame entitlement1 = new OADipSmallGame();
        EntitlementInfo entitlement2 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.NON_DIP_SMALL);

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
        logFlowPoint(String.format("Completely download game 1 '%s' and game 2 '%s'", entitlementName1, entitlementName2)); //3
        logFlowPoint(String.format("Verify 'Download Queue' shows game 1 '%s' and game 2 '%s' have completed download",
                entitlementName1, entitlementName2)); //4
        logFlowPoint("Logout and log back in"); //5
        logFlowPoint("Verify 'Download Queue' is no longer available"); //6
        logFlowPoint(String.format("Uninstall and re-install game 1 '%s'", entitlementName1)); //7
        logFlowPoint(String.format("Completely download game 1 '%s'", entitlementName1)); //8
        logFlowPoint("Remove the installed notification from the download manager"); //9

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
        if (gameTile1.waitForDownloadable() && gameTile2.waitForDownloadable()) {
            logPass(String.format("Verified successful navigation to Game Library with downloadable games '%s' and '%s'",
                    entitlementName1, entitlementName2));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library, or cannot locate downloable games '%s' or '%s'",
                    entitlementName1, entitlementName2));
        }

        //3
        boolean fullDownload1 = MacroGameLibrary.downloadFullEntitlement(driver, offerId1);
        boolean fullDownload2 = MacroGameLibrary.downloadFullEntitlement(driver, offerId2);
        if (fullDownload1 && fullDownload2) {
            logPass(String.format("Verified successful full download of game 1 '%s' & game 2 '%s'", entitlementName1, entitlementName2));
        } else {
            String status1 = fullDownload1 ? "complete" : "not complete";
            String status2 = fullDownload2 ? "complete" : "not complete";
            logFailExit(String.format("Failed: Cannot complete both downloads - '%s' : %s, '%s' : %s",
                    entitlementName1, status1, entitlementName2, status2));
        }

        //4
        DownloadQueueTile queueTile1 = new DownloadQueueTile(driver, offerId1);
        DownloadQueueTile queueTile2 = new DownloadQueueTile(driver, offerId2);
        DownloadManager downloadManager = new DownloadManager(driver);
        downloadManager.clickDownloadCompleteCountToOpenFlyout();
        boolean entitlement1DownloadComplete = Waits.pollingWait(queueTile1::isGameDownloadComplete);
        boolean entitlement2DownloadComplete = Waits.pollingWait(queueTile2::isGameDownloadComplete);
        if (entitlement1DownloadComplete && entitlement2DownloadComplete) {
            logPass(String.format("Verified 'Download Queue' shows games 1 '%s' and 2 '%s' have completed download",
                    entitlementName1, entitlementName2));
        } else {
            String status1 = fullDownload1 ? "complete" : "not complete";
            String status2 = fullDownload2 ? "complete" : "not complete";
            logFailExit(String.format("Failed: 'Download Queue' does not show both downloads complete - '%s' : %s, '%s' : %s",
                    entitlementName1, status1, entitlementName2, status2));
        }

        //5
        new MainMenu(driver).selectLogOut();
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified user can logout and log back in");
        } else {
            logFailExit("Failed: User cannot logout and log back in");
        }

        //6
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();
        if (!downloadManager.isDownloadManagerVisible()) {
            logPass("Verified 'Download Queue' is no longer available");
        } else {
            logFailExit("Failed: 'Download Queue' is still available");
        }

        //7
        entitlement1.enableSilentUninstall(client);
        gameTile1 = new GameTile(driver, offerId1);
        gameTile1.uninstall();
        new MainMenu(driver).selectReloadMyGames();
        boolean uninstallGame1 = gameTile1.waitForDownloadable();
        boolean reinstallGame1 = MacroGameLibrary.downloadFullEntitlement(driver, offerId1);
        if (uninstallGame1 && reinstallGame1) {
            logPass(String.format("Verified successful uninstall and re-install of game 1 '%s' ", entitlementName1));
        } else {
            logFailExit(String.format("Failed: Cannot uninstall or re-install game 1 '%s'", entitlementName1));
        }

        //8
        queueTile1 = new DownloadQueueTile(driver, offerId1);
        downloadManager.clickDownloadCompleteCountToOpenFlyout();
        if (Waits.pollingWait(queueTile1::isGameDownloadComplete)) {
            logPass(String.format("Verified Download Queue shows games 1 '%s' has completed download", entitlementName1));
        } else {
            logFailExit(String.format("Failed: Download Queue does not show games 1 '%s' have completed download", entitlementName1));
        }

        //9
        queueTile1.clickCancelClearButton();
        if (!downloadManager.isDownloadManagerVisible()) {
            logPass("Verified 'Download Queue' is not available after clicking 'Cancel' on 'Download Queue Tile' of game 1");
        } else {
            logFailExit("Failed: 'Download Queue' is still available after clicking 'Cancel' on 'Download Queue Tile' of game 1");
        }

        softAssertAll();
    }
}
