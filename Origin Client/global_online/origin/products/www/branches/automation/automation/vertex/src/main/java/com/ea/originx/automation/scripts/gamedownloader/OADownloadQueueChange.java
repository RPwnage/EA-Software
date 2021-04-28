package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueue;
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
 * Test changing the 'in progress' game using the 'Download Now' button at a
 * queued game's 'Download Queue' tile
 *
 * @author palui
 */
public class OADownloadQueueChange extends EAXVxTestTemplate {

    @TestRail(caseId = 9939)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testDownloadQueueChange(ITestContext context) throws Exception {

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
        logFlowPoint(String.format("Navigate to Game Library and verify games '%s' and '%s' appear as downloadable",
                entitlementName1, entitlementName2)); //2
        logFlowPoint(String.format("Start downloading '%s' and '%s'", entitlementName1, entitlementName2)); //3
        logFlowPoint(String.format("Verify 'Download Queue' shows '%s' is downloading and '%s' has been queued",
                entitlementName1, entitlementName2)); //4
        logFlowPoint(String.format("Click 'Download Now' button at 'Download queue Tile' of '%s' . Verify is now downloading and '%s' queued",
                entitlementName2, entitlementName1)); //5

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
        // Change download rate to 1MB in order to be able to check all the steps before completing the download.
        TestScriptHelper.setMaxDownloadRateBpsOutOfGame(driver, "1000000");
        boolean startDownloading1 = MacroGameLibrary.startDownloadingEntitlement(driver, offerId1);
        boolean startDownloading2 = MacroGameLibrary.startDownloadingEntitlement(driver, offerId2);
        if (startDownloading1 && startDownloading2) {
            logPass(String.format("Verified start downloading successful for both '%s' and '%s'",
                    entitlementName1, entitlementName2));
        } else {
            logFailExit(String.format("Failed: Start downloading failed for one of '%s' or '%s'",
                    entitlementName1, entitlementName2));
        }

        //4
        DownloadManager miniDownloadManager = new DownloadManager(driver);
        miniDownloadManager.clickRightArrowToOpenFlyout();        
        DownloadQueueTile queueTile1 = new DownloadQueueTile(driver, offerId1);
        DownloadQueueTile queueTile2 = new DownloadQueueTile(driver, offerId2);
        boolean entitlement1Downloading = Waits.pollingWait(queueTile1::isGameDownloading);
        boolean entitlement2Queued = Waits.pollingWait(queueTile2::isGameQueued);
        if (entitlement1Downloading && entitlement2Queued) {
            logPass(String.format("Verified 'Download Queue' shows '%s' is the current download and '%s' is queued",
                    entitlementName1, entitlementName2));
        } else {
            logFailExit(String.format("Failed: Game 1 '%s' is not the current download or '%s' is not queued",
                    entitlementName1, entitlementName2));
        }

        //5
        queueTile2.clickDownloadButton();
        miniDownloadManager.clickRightArrowToOpenFlyout();        
        boolean entitlement2Downloading = Waits.pollingWait(queueTile2::isGameDownloading);
        boolean entitlement1Queued = Waits.pollingWait(queueTile1::isGameQueued);
        if (entitlement2Downloading && entitlement1Queued) {
            logPass(String.format("Verified 'Download Queue' now shows '%s' is the current download and '%s' is queued",
                    entitlementName2, entitlementName1));
        } else {
            logFailExit(String.format("Failed: '%s' is not the current download or '%s' is not queued",
                    entitlementName2, entitlementName1));
        }

        softAssertAll();
    }
}
