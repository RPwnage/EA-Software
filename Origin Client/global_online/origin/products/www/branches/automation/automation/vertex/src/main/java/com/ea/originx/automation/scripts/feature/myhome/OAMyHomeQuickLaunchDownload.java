package com.ea.originx.automation.scripts.feature.myhome;

import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadEULASummaryDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadInfoDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadLanguageSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.discover.QuickLaunchTile;
import com.ea.originx.automation.lib.pageobjects.discover.RecentlyPlayedSection;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.TestCleaner;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests downloading a game via Quick Launch
 *
 * @author sbentley
 * @author mdobre
 */
public class OAMyHomeQuickLaunchDownload extends EAXVxTestTemplate {

    @TestRail(caseId = 963050)
    @Test(groups = {"myhome", "client_only", "full_regression"})
    public void testMyHomeQuickLaunchDownload(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context); 
        final OADipSmallGame entitlement = new OADipSmallGame();
        final String entitlementOfferID = entitlement.getOfferId();
        final UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);

        //clear the local app data and uninstall
        new TestCleaner(client).clearLocalSettings(true);

        logFlowPoint("Log in to Origin client with an account that has games."); //1
        logFlowPoint("Navigate to Game Library, download and play a game then uninstall it."); //2
        logFlowPoint("Navigate to My Home and verify Recently Played section appears."); //3
        logFlowPoint("Verify last closed game appears in Recently Played section."); //4
        logFlowPoint("Verify Quick Launch tile has game title."); //5
        logFlowPoint("Verify Quick Launch tile has DOWNLOAD above game title."); //6
        logFlowPoint("Verify achievements appear on Quick Launch tile."); //7
        logFlowPoint("Hover over Quick Launch tile and verify tile expand animation.");  //8        
        logFlowPoint("Click on Quick Launch tile and start download of the game."); //9
        logFlowPoint("Verify Quick Launch tile updates from DOWNLOAD to DOWNLOADING."); //10
        logFlowPoint("Wait for Install to finish and verify Quick Launch tile updates from DOWNLOAD to PLAY."); //11
        
        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with account: " + userAccount.getUsername());
        } else {
            logFailExit("Could not log into Origin with account: " + userAccount.getUsername());
        }

        //2
        NavigationSidebar navSideBar = new NavigationSidebar(driver);
        navSideBar.gotoGameLibrary();       
        GameTile gameTile = new GameTile(driver, entitlementOfferID);
        MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferID);
        gameTile.play();
        MacroGameLibrary.handlePlayDialogs(driver);
        entitlement.isLaunched(client);
        entitlement.killLaunchedProcess(client);
        
        entitlement.enableSilentUninstall(client);
        gameTile.uninstall();
        if (Waits.pollingWait(() -> !gameTile.isReadyToPlay())) {
            logPass("Successfully downloaded, played and uninstall game.");
        } else {
            logFailExit("Could not download, play or uninstall game.");
        }

        //3
        //Throttle the speed so all states can be checked before download finishes
        AppSettings appSettings = new MainMenu(driver).selectApplicationSettings();
        appSettings.verifyAppSettingsReached();
        appSettings.setDownloadThrottleOutOfGame("1000000");
        navSideBar.gotoDiscoverPage();
        RecentlyPlayedSection recentlyPlayed = new RecentlyPlayedSection(driver);
        if (recentlyPlayed.verifyRecentlyPlayedAreaVisisble()) {
            logPass("Successfully located Quick Launch area.");
        } else {
            logFailExit("Could not locate Quick Launch area.");
        }

        //4
        QuickLaunchTile quickLaunchTile = new QuickLaunchTile(driver, entitlementOfferID);
        if (quickLaunchTile.verifyTileVisible()) {
            logPass("Successfully verified recently played game appears in Quick Launch area.");
        } else {
            logFailExit("Failed to verify recently played game appears in Quick Launch area.");
        }

        //5
        if (quickLaunchTile.verifyQuickLaunchTitle(entitlement)) {
            logPass("Successfully verified Quick Launch tile's title.");
        } else {
            logFail("Failed to verify Quick Launch tile's title.");
        }

        //6
        if (quickLaunchTile.verifyEyebrowStateDownloadVisible()) {
            logPass("Successfully verified Quick Launch tile in Download state.");
        } else {
            logFail("Failed to verify Quick Launch tile in Download state.");
        }

        //7
        //We want achievements list to show for games that do, but don't for games that don't have achievements
        boolean verifyAchievements = quickLaunchTile.verifyAchievementsVisible();
        if (verifyAchievements) {
            logPass("Successfully verified Achievements are showing appropriately on the tile.");
        } else {
            logFail("Failed to verify achievements are showing appropriately on the tile.");
        }

        //8
        if (quickLaunchTile.verifyExpandHoverAnimation()) {
            logPass("Successfully verified animation on hover.");
        } else {
            logFail("Failed to verify animation on hover.");
        }

        //9
        quickLaunchTile.clickTile();
        new DownloadLanguageSelectionDialog(driver).clickAccept();
        new DownloadInfoDialog(driver).clickNext();
        DownloadEULASummaryDialog eulaDialog = new DownloadEULASummaryDialog(driver);
        eulaDialog.setLicenseAcceptCheckbox(true);
        eulaDialog.clickNextButton();
        
        DownloadManager downloadManager = new DownloadManager(driver);
        if (downloadManager.verifyInProgressGame(entitlement)) {
            logPass("Successfully verified download is in progress.");
        } else {
            logFailExit("Could not verify download is in progress.");
        }
        
        //10
        if (quickLaunchTile.verifyEyebrowStatePauseDownloadVisible()) {
            logPass("Successfully verified tile is in 'Downloading' state.");
        } else {
            logFailExit("Could not verify tile is in 'Downloading' state.");
        }
        
        //11
        //Wait for game to install
        if (quickLaunchTile.verifyEyebrowStatePlayVisible(180000, 100, 0)) {
            logPass("Successfully verified tile in Play state.");
        } else {
            logFail("Failed to verify tile in Play state.");
        }
        softAssertAll();
    }
}