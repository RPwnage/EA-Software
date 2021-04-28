package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.vx.originclient.resources.EACore;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueue;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
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
 * Test download queue flyout with downloading, queued and completed items
 *
 * @author palui
 */
public class OADownloadQueue extends EAXVxTestTemplate {

    @TestRail(caseId = 9619)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testDownloadQueue(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        OADipSmallGame entitlement1 = new OADipSmallGame();
        EntitlementInfo entitlement2 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);
        EntitlementInfo entitlement3 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.PI);

        final String entitlementName1 = entitlement1.getName();
        final String offerId1 = entitlement1.getOfferId();
        final String entitlementName2 = entitlement2.getName();
        final String offerId2 = entitlement2.getOfferId();
        final String entitlementName3 = entitlement3.getName();
        final String offerId3 = entitlement3.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement1, entitlement2, entitlement3);
        String username = userAccount.getUsername();

        EACoreHelper.changeToBaseEntitlementPIOverride(client.getEACore());

        logFlowPoint(String.format("Launch Origin and login as user %s entitled to '%s', '%s' and '%s'",
                username, entitlementName1, entitlementName2, entitlementName3));//1
        logFlowPoint(String.format("Navigate to the Game Library and verify games '%s', '%s' and '%s' appear as downloadable",
                entitlementName1, entitlementName2, entitlementName3)); //2
        logFlowPoint(String.format("Completely download game 1 '%s'", entitlementName1)); //3
        logFlowPoint(String.format("Start downloading of game 2 '%s' and game 3 '%s'", entitlementName2, entitlementName3)); //4
        logFlowPoint(String.format("Verify game 2 '%s' is the current download, game 3 '%s' is queued for download, and game 1 '%s' has completed download",
                entitlementName2, entitlementName3, entitlementName1)); //5
        logFlowPoint(String.format("Verify clicking 'Repair' on game tile of game 1 '%s' makes it the current download, and game 2 '%s' and game 3 '%s' are queued for download",
                entitlementName1, entitlementName2, entitlementName3)); //6
        logFlowPoint(String.format("Verify that after game 1 '%s' has completed repair, game 2 '%s' is the current download, and game 3 '%s' is queued for download",
                entitlementName1, entitlementName2, entitlementName3)); //7
        logFlowPoint("Navigate to 'Application Settings' page and and disable 'Automatic game updates'"); //8
        logFlowPoint(String.format("Exit Origin, add update path of game 1 '%s' to EACore, and log back in", entitlementName1)); //9
        logFlowPoint(String.format("Verify clicking 'Update Game' on game tile of game 1 '%s' makes it the current download, and game 2 '%s' and game 3 '%s' are queued for download",
                entitlementName1, entitlementName2, entitlementName3)); //10
        logFlowPoint(String.format("Verify that after game 1 '%s' has completed update, game 2 '%s' is the current download, and game 3 '%s' is queued for download",
                entitlementName1, entitlementName2, entitlementName3)); //11

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
        if (MacroGameLibrary.downloadFullEntitlement(driver, offerId1)) {
            logPass(String.format("Verified successful full download of game 1 '%s'", entitlementName1));
        } else {
            logFailExit(String.format("Failed: Cannot complete download of game 1 '%s'", entitlementName1));
        }

        //4
        boolean startDownloading2 = MacroGameLibrary.startDownloadingEntitlement(driver, offerId2);
        Waits.pollingWait(new DownloadQueue(driver)::isOpen); // Wait for Download Queue Flyout to open before closing it
        boolean startDownloading3 = MacroGameLibrary.startDownloadingEntitlement(driver, offerId3);
        if (startDownloading2 && startDownloading3) {
            logPass(String.format("Verified start downloading successful for game 2 '%s' and game 3 '%s'",
                    entitlementName2, entitlementName3));
        } else {
            logFailExit(String.format("Failed: Start downloading failed for one of game 2 '%s' or game 3 '%s'",
                    entitlementName2, entitlementName3));
        }

        //5
        DownloadQueueTile queueTile1 = new DownloadQueueTile(driver, offerId1);
        DownloadQueueTile queueTile2 = new DownloadQueueTile(driver, offerId2);
        DownloadQueueTile queueTile3 = new DownloadQueueTile(driver, offerId3);
        DownloadManager miniDownloadManager = new DownloadManager(driver);
        miniDownloadManager.clickRightArrowToOpenFlyout();        
        boolean entitlement1DownloadComplete = Waits.pollingWait(queueTile1::isGameDownloadComplete);
        boolean entitlement2Downloading = Waits.pollingWait(queueTile2::isGameDownloading);
        boolean entitlement3Queued = Waits.pollingWait(queueTile3::isGameQueued);
        if (entitlement1DownloadComplete && entitlement2Downloading && entitlement3Queued) {
            logPass(String.format("Verified game 2 '%s' is the current download, game 3 '%s' is queued for download, and game 1 '%s' has completed download",
                    entitlementName2, entitlementName3, entitlementName1));
        } else {
            logFailExit(String.format("Failed: Game 2 '%s' is not the current download, game 3 '%s' is not queued for download, or game 1 '%s' download has not completed download",
                    entitlementName2, entitlementName3, entitlementName1));
        }

        //6
        miniDownloadManager.closeDownloadQueueFlyout();
        gameTile1.repair();
        miniDownloadManager.clickRightArrowToOpenFlyout();        
        boolean entitlement1Downloading = Waits.pollingWait(queueTile1::isGameDownloading);
        boolean entitlement2Queued = Waits.pollingWait(queueTile2::isGameQueued);
        entitlement3Queued = Waits.pollingWait(queueTile3::isGameQueued);
        if (entitlement1Downloading && entitlement2Queued && entitlement3Queued) {
            logPass(String.format("Verified repairing game 1 '%s' makes it the current download, and game 2 '%s' and game 3 '%s' are queued for download",
                    entitlementName1, entitlementName2, entitlementName3));
        } else {
            logFailExit(String.format("Failed: Repairing game 1 '%s' does not make it current download, or and game 2 '%s' and/or game 3 '%s' are not queued for download",
                    entitlementName1, entitlementName2, entitlementName3));
        }

        //7
        boolean gameTile1DownloadComplete = gameTile1.waitForReadyToPlay();
        entitlement1DownloadComplete = Waits.pollingWait(queueTile1::isGameDownloadComplete);
        entitlement2Downloading = Waits.pollingWait(queueTile2::isGameDownloading);
        entitlement3Queued = Waits.pollingWait(queueTile3::isGameQueued);
        if (gameTile1DownloadComplete && entitlement1DownloadComplete && entitlement2Downloading && entitlement3Queued) {
            logPass(String.format("Verified that after game 1 '%s' has completed repair, game 2 '%s' is the current download, and game 3 '%s' is queued for download",
                    entitlementName1, entitlementName2, entitlementName3));
        } else {
            logFailExit(String.format("Failed: Game 1 '%s' has not completed repair, or game 2 '%s' is not the current download, or game 3 '%s' is not queued for download",
                    entitlementName1, entitlementName2, entitlementName3));
        }

        //8
        miniDownloadManager.closeDownloadQueueFlyout();
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectApplicationSettings();
        new SettingsNavBar(driver).clickApplicationLink();
        AppSettings appSettings = new AppSettings(driver);
        appSettings.setKeepGamesUpToDate(false);
        if (Waits.pollingWait(() -> appSettings.verifyKeepGamesUpToDate(false))) {
            logPass("Verified 'Automatic game updates' setting disabled");
        } else {
            logFailExit("Failed: Cannot disable 'Automatic game updates' setting");
        }

        //9
        LoginPage loginPage = mainMenu.selectLogOut();
        loginPage.close();
        boolean eaCoreUpdated = client.getEACore().setEACoreValue(EACore.EACORE_CONNECTION_SECTION,
                OADipSmallGame.DIP_SMALL_AUTO_DOWNLOAD_PATH,
                OADipSmallGame.DIP_SMALL_DOWNLOAD_VALUE_UPDATE_TINY);
        client.stop();
        driver = startClientObject(context, client);
        boolean reloginSuccessful = MacroLogin.startLogin(driver, userAccount);
        if (eaCoreUpdated && reloginSuccessful) {
            logPass("Verified Origin exits, the EACore is modified, and user can log back in");
        } else {
            logFailExit("Failed: Cannot modify the EACore or log back in");
        }

        //10
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();

        gameTile1 = new GameTile(driver, offerId1);
        queueTile1 = new DownloadQueueTile(driver, offerId1);
        queueTile2 = new DownloadQueueTile(driver, offerId2);
        queueTile3 = new DownloadQueueTile(driver, offerId3);

        gameTile1.updateGame();

        miniDownloadManager = new DownloadManager(driver);
        miniDownloadManager.clickRightArrowToOpenFlyout();
        entitlement1Downloading = Waits.pollingWait(queueTile1::isGameDownloading);
        entitlement2Queued = Waits.pollingWait(queueTile2::isGameQueued);
        entitlement3Queued = Waits.pollingWait(queueTile3::isGameQueued);
        if (entitlement1Downloading && entitlement2Queued && entitlement3Queued) {
            logPass(String.format("Verified updating game 1 '%s' makes it the current download, and game 2 '%s' and game 3 '%s' are queued for download",
                    entitlementName1, entitlementName2, entitlementName3));
        } else {
            logFailExit(String.format("Failed: Updating game 1 '%s' does not make it current download, or and game 2 '%s' and/or game 3 '%s' are not queued for download",
                    entitlementName1, entitlementName2, entitlementName3));
        }

        //11
        boolean gameTile1UpdateComplete = gameTile1.waitForReadyToPlay();
        entitlement1DownloadComplete = Waits.pollingWait(queueTile1::isGameDownloadComplete);
        entitlement2Downloading = Waits.pollingWait(queueTile2::isGameDownloading);
        entitlement3Queued = Waits.pollingWait(queueTile3::isGameQueued);

        if (gameTile1UpdateComplete && entitlement1DownloadComplete && entitlement2Downloading && entitlement3Queued) {
            logPass(String.format("Verified that after game 1 '%s' has completed update, game 2 '%s' is the current download, and game 3 '%s' is queued for download",
                    entitlementName1, entitlementName2, entitlementName3));
        } else {
            logFailExit(String.format("Failed: Game 1 '%s' has not completed update, or game 2 '%s' is not the current download, or game 3 '%s' is not queued for download",
                    entitlementName1, entitlementName2, entitlementName3));
        }

        softAssertAll();
    }
}
