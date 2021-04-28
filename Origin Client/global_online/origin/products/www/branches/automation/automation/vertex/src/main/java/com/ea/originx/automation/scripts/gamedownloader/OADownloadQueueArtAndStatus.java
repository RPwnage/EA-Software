package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CancelDownload;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.Battlefield4;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * A test which checks if boxart and artwork exist for entitlements in download
 * queue
 *
 * @author rchoi
 */
public class OADownloadQueueArtAndStatus extends EAXVxTestTemplate {

    @TestRail(caseId = 9640)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testQueueArtAndStatus(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        // DipLarge is used to check for artwork
        EntitlementInfo dipLarge = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);
        final String dipLarge_OfferID = dipLarge.getOfferId();
        final String dipLarge_Name = dipLarge.getName();

        // DipSmall is used to check for boxart when repairing and updating
        EntitlementInfo dipSmall = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        final String dipSmall_OfferID = dipSmall.getOfferId();
        final String dipSmall_Name = dipSmall.getName();

        // basegame is used to check for artwork, boxart and some information when downloading
        EntitlementInfo baseGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String basegameOfferId = baseGame.getOfferId();
        final String baseGameName = baseGame.getName();

        // dlc is used to check for boxart
        final String dlcName = Battlefield4.BF4_LEGACY_OPERATIONS_NAME;
        final String dlcOfferId = Battlefield4.BF4_LEGACY_OPERATIONS_OFFER_ID;

        // test with account which has DLC
        UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.DLC_TEST_ACCOUNT);

        logFlowPoint("Login to Origin with account which has DLC."); // 1
        logFlowPoint("Navigate to the 'Game Library'."); // 2
        logFlowPoint("Download " + dipLarge_Name); // 3
        logFlowPoint("Verify " + dipLarge_Name + " does not have artwork then cancel download of " + dipLarge_Name); // 4
        logFlowPoint("Download base game" + baseGameName + " and verify current downloading " + baseGameName + " does have a artwork."); // 5
        logFlowPoint("Verify current downloading " + baseGameName + " does have a boxart."); // 6
        logFlowPoint("Verify DLC " + dlcName + " which is in queue does have a boxart."); // 7
        logFlowPoint("Verify download progress, speed, time remaining, and total size shown are in the 'Mini Queue' for " + baseGameName + " then cancel downloading it."); // 8
        logFlowPoint("Finish downloading and start repairing for " + dipSmall_Name + " then verify box art while repairing it and wait for repairing to be completed."); // 9
        logFlowPoint("Navigate to the 'Application Settings' page."); // 10
        logFlowPoint("Disable the 'Automatic Game Updates' setting."); // 11
        logFlowPoint("Exit Origin and update the EACore."); // 12
        logFlowPoint("Re-login to client."); // 13
        logFlowPoint("Update " + dipSmall_Name); // 14
        logFlowPoint("Verify boxart for current updating " + dipSmall_Name); // 15

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        // 2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Navigated to the 'Game Library'.");
        } else {
            logFailExit("Could not navigate to the 'Game Library'.");
        }

        // 3
        if (MacroGameLibrary.startDownloadingEntitlement(driver, dipLarge_OfferID)) {
            logPass("Successfully started downloading " + dipLarge_Name);
        } else {
            logFailExit("Could not start downloading " + dipLarge_Name);
        }

        // 4
        // Assume dipLarge does not have background artwork
        DownloadQueueTile dipLargeDownloadQueueTile = new DownloadQueueTile(driver, dipLarge_OfferID);
        DownloadManager miniDownloadManager = new DownloadManager(driver);
        miniDownloadManager.clickRightArrowToOpenFlyout();        
        dipLargeDownloadQueueTile.clickPauseButton();
        if (!dipLargeDownloadQueueTile.hasBackgroundArt()) {
            logPass("Verified " + dipLarge_Name + " does not have artwork.");
        } else {
            logFailExit("Found artwork for " + dipLarge_Name);
        }
        // close download queue
        miniDownloadManager.clickRightArrowToOpenFlyout();        
        dipLargeDownloadQueueTile.clickCancelClearButton();
        new CancelDownload(driver).clickYes();

        // 5
        // Assume mass effect 3 has a artwork
        MacroGameLibrary.startDownloadingEntitlement(driver, basegameOfferId, new DownloadOptions().setUncheckExtraContent(false));
        DownloadQueueTile basegameDownloadQueueTile = new DownloadQueueTile(driver, basegameOfferId);
        DownloadQueueTile basegameDlcDownloadQueueTile = new DownloadQueueTile(driver, dlcOfferId);
        miniDownloadManager.clickRightArrowToOpenFlyout();
        boolean isDownloadInfoVisible = basegameDownloadQueueTile.verifyInformationinMiniQueue(); // for later check because sometimes game finishes downloading by then
        boolean isBackgroundArtVisible = basegameDownloadQueueTile.hasBackgroundArt();
        boolean isPackArtVisible = basegameDownloadQueueTile.verifyPackart();
        boolean isDLCPackArtVisible = basegameDlcDownloadQueueTile.verifyPackart();
        if (isBackgroundArtVisible) {
            logPass("Successfully verified artwork for " + baseGameName + " base game.");
        } else {
            logFailExit("Could not find artwork for " + baseGameName + " base game.");
        }

        // 6
        if (isPackArtVisible) {
            logPass("Successfully verified boxart for " + baseGameName);
        } else {
            logFailExit("Could not download/install" + baseGameName);
        }

        // 7
        if (isDLCPackArtVisible) {
            logPass("Successfully verified boxart for DLC " + dlcName + " in download queue.");
        } else {
            logFailExit("Could not find boxart for" + dlcName);
        }

        // 8
        if (isDownloadInfoVisible) {
            logPass("Successfully verified information for download progress, speed, time remaining, and total size shown in the 'Mini Queue' for" + baseGameName + " base game.");
        } else {
            logFailExit("Could not get information from the 'Mini Queue' for" + baseGameName + " base game.");
        }
        // close download queue
        miniDownloadManager.clickRightArrowToOpenFlyout();
        basegameDownloadQueueTile.clickCancelClearButton();
        new CancelDownload(driver).clickYes();


        // 9
        GameTile dipSmallTile = new GameTile(driver, dipSmall_OfferID);
        MacroGameLibrary.downloadFullEntitlement(driver, dipSmall_OfferID);
        DownloadQueueTile dipSmallQueueTile = new DownloadQueueTile(driver, dipSmall_OfferID);
        miniDownloadManager.clickDownloadCompleteCountToOpenFlyout();
        dipSmallQueueTile.clickCancelClearButton();
        dipSmallTile.repair();
        Waits.pollingWait(() -> dipSmallQueueTile.isGameDownloadComplete());
        miniDownloadManager.clickDownloadCompleteCountToOpenFlyout();
        if (dipSmallQueueTile.verifyPackart()) {
            logPass("Successfully verified boxart for " + dipSmall_Name + " which is being repaired.");
        } else {
            logFailExit("Could not download/install" + dipSmall_Name);
        }

        // 10
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectApplicationSettings();
        SettingsNavBar settingsNavBar = new SettingsNavBar(driver);
        settingsNavBar.clickApplicationLink();
        AppSettings appSettings = new AppSettings(driver);
        if (appSettings.verifyAppSettingsReached()) {
            logPass("Successfully navigated to the 'Application Settings' page.");
        } else {
            logFailExit("Failed to navigate to the 'Application Settings' page.");
        }

        // 11
        appSettings.setKeepGamesUpToDate(false);
        if (Waits.pollingWait(() -> appSettings.verifyKeepGamesUpToDate(false))) {
            logPass("Successfully disabled the 'Automatic Game Updates' setting.");
        } else {
            logFailExit("Failed to disable the 'Automatic Game Updates' setting.");
        }

        // 12
        mainMenu.selectExit();
        boolean isOriginProcessStopped = Waits.pollingWaitEx(() -> !ProcessUtil.isProcessRunning(client, OriginClientData.ORIGIN_PROCESS_NAME));
        boolean isEACoreUpdated = OADipSmallGame.changeToMediumEntitlementDipSmallOverride(client);
        if (isOriginProcessStopped && isEACoreUpdated) {
            logPass("Successfully exited Origin and updated the EACore.");
        } else {
            logFailExit("Failed to exit Origin and/or update the EACore.");
        }

        // 13
        client.stop();
        driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully re-logged into the client.");
        } else {
            logFailExit("Failed to re-logged into the client.");
        }

        // 14
        new NavigationSidebar(driver).gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, dipSmall_OfferID);
        DownloadQueueTile downloadQueueTile = new DownloadQueueTile(driver, dipSmall_OfferID);
        gameTile.updateGame();
        boolean isUpdating = Waits.pollingWait(() -> gameTile.isUpdating() || gameTile.isInstalling());
        if (isUpdating) {
            logPass("Successfully started updating " + dipSmall_Name);
        } else {
            logFailExit("Failed to start updating " + dipSmall_Name);
        }

        // 15
        sleep(1000); // wait for updating entitlement
        if (downloadQueueTile.verifyPackart()) {
            logPass("Successfully verified boxart for " + dipSmall_Name + " which is being updated.");
        } else {
            logFailExit("Could not find boxart for" + dipSmall_Name + " which is being updated.");
        }

        softAssertAll();
    }
}