package com.ea.originx.automation.scripts.feature.ogd;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CancelDownload;
import com.ea.originx.automation.lib.pageobjects.dialog.GamePropertiesDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.*;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the OGD cog functionality (Download, Pause, Resume, Cancel and Game
 * Properties)
 *
 * @author nvarthakavi
 */
public class OAOGDCogwheel extends EAXVxTestTemplate {

    @TestRail(caseId = 3096151)
    @Test(groups = {"gamelibrary", "ogd_smoke", "ogd", "client_only", "allfeaturesmoke"})
    public void testOGDCogwheel(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        String username = userAccount.getUsername();

        logFlowPoint(String.format("Launch Origin and login as user %s entitled to '%s'", username, entitlementName));//1
        logFlowPoint(String.format("Navigate to the Game Library and verify game '%s' is downloadable ", entitlementName));//2
        logFlowPoint(String.format("Click on Download button through Cog menu and verify game '%s' is downloading", entitlementName));//3
        logFlowPoint(String.format("Click on Pause button through Cog menu and verify game '%s' has paused", entitlementName));//4
        logFlowPoint(String.format("Click on Resume button through Cog menu and verify game '%s' has resumed download", entitlementName));//5
        logFlowPoint(String.format("Click on Cancel button through Cog menu and verify game '%s' has cancelled download", entitlementName));//6
        logFlowPoint(String.format("Click on Game Properties button through Cog menu and verify game '%s''s Game Properties window opens", entitlementName));//7

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass(String.format("Verified login successful as user %s", username));
        } else {
            logFailExit(String.format("Failed: Cannot login as user %s", username));
        }

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        if (gameTile.isGameTileVisible()) {
            logPass(String.format("Verified successful navigation to Game Library with downloadable game '%s' ",
                    entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library or cannot locate downloadable game '%s'",
                    entitlementName));
        }

        //3
        GameSlideout entitlementSlideout = gameTile.openGameSlideout();
        entitlementSlideout.waitForSlideout();
        DownloadOptions options = new DownloadOptions().setDownloadTrigger(DownloadOptions.DownloadTrigger.DOWNLOAD_FROM_GAME_SLIDEOUT_COG_WHEEL);
        MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferId, options);
        DownloadQueueTile queueTile = new DownloadQueueTile(driver, entitlementOfferId);
        boolean isDownloading = Waits.pollingWait(queueTile::isGameDownloading);
        if (isDownloading) {
            logPass(String.format("Verified game '%s' is the downloading", entitlementName));
        } else {
            logFailExit(String.format("Failed: game '%s' is not downloading", entitlementName));
        }

        //4
        DownloadManager downloadManager = new DownloadManager(driver);
        downloadManager.closeDownloadQueueFlyout();
        GameSlideoutCogwheel gameSlideoutCogwheel = new GameSlideout(driver).getCogwheel();
        gameSlideoutCogwheel.pause();
        boolean isPaused = Waits.pollingWait(gameTile::isPaused);
        if (isPaused) {
            logPass(String.format("Verified game '%s' has paused", entitlementName));
        } else {
            logFailExit(String.format("Failed: game '%s' has not paused", entitlementName));
        }

        //5
        downloadManager.closeDownloadQueueFlyout();
        gameSlideoutCogwheel.resume();
        downloadManager.openDownloadQueueFlyout();
        boolean isResume = Waits.pollingWait(queueTile::isGameDownloading);
        if (isResume) {
            logPass(String.format("Verified game '%s' has resumed downloading", entitlementName));
        } else {
            logFailExit(String.format("Failed: game '%s' has not resumed downloading", entitlementName));
        }

        //6
        downloadManager.closeDownloadQueueFlyout();
        gameSlideoutCogwheel.cancel();
        CancelDownload cancelDownload = new CancelDownload(driver);
        cancelDownload.clickYes();
        sleep(1000); // Wait for the cancel download dialog close
        entitlementSlideout.clickSlideoutCloseButton();
        boolean isCancel = !Waits.pollingWait(gameTile::isDownloading);
        if (isCancel) {
            logPass(String.format("Verified game '%s' download has been cancelled ", entitlementName));
        } else {
            logFailExit(String.format("Failed: game '%s' download has not been cancelled", entitlementName));
        }

        //7
        gameTile.openGameSlideout();
        entitlementSlideout.waitForSlideout();
        gameSlideoutCogwheel.showGameProperties();
        GamePropertiesDialog gamePropertiesDialog = new GamePropertiesDialog(driver);
        boolean isGamePropertiesOpen = Waits.pollingWait(gamePropertiesDialog::isOpen);
        if (isGamePropertiesOpen) {
            logPass(String.format("Verified game properties dialog for the game '%s' has opened", entitlementName));
        } else {
            logFailExit(String.format("Failed: game properties dialog for the game '%s' has not opened", entitlementName));
        }

        softAssertAll();

    }

}
