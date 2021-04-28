package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CancelDownload;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.Battlefield4;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test to pause, resume, and cancel the downloading of a zip entitlement. There
 * are a set of parameterized test cases attached to this:<br>
 * - OADownloadCancelZip<br>
 * - OADownloadCancelDiP<br>
 * - OAPauseCancelZip<br>
 * - OAPauseCancelDiP<br>
 * - OAPauseResumeCancelZip<br>
 * - OAPauseResumeCancelDiP<br>
 *
 * @author glivingstone
 */
public abstract class OAPauseResumeCancel extends EAXVxTestTemplate {
    
    public enum ENTITLEMENT_TYPE {
        DIP_WITH_DLC,
        DIP_WITHOUT_DLC,
        NON_DIP
    }

    /**
     * The main test function which all parameterized test cases call.
     *
     * @param context  The context we are using
     * @param pause    Whether to pause the download in the test
     * @param resume   Whether to resume the download after pausing in the test
     * @param entitlementType  The entitlement type that is used for testing
     * @param testName The name of the test. We need to pass this up so
     *                 initLogger properly attaches to the CRS test case.
     * @throws Exception
     */
    public void testPauseResumeCancel(ITestContext context, boolean pause, boolean resume, ENTITLEMENT_TYPE entitlementType, String testName) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        
        EntitlementInfo entitlement;
        UserAccount userAccount;
        String DLCOfferID, DLCName;
        DownloadQueueTile DLCQueueTile;
        DownloadOptions downloadOptions = new DownloadOptions();
        if (entitlementType == ENTITLEMENT_TYPE.DIP_WITH_DLC) {
            entitlement = new Battlefield4();
            DLCOfferID = Battlefield4.BF4_LEGACY_OPERATIONS_OFFER_ID;
            DLCName = Battlefield4.BF4_LEGACY_OPERATIONS_NAME;
            userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.DLC_TEST_ACCOUNT);
            downloadOptions.setUncheckExtraContent(false);
        } else {
            entitlement = EntitlementInfoHelper.getEntitlementInfoForId(entitlementType == ENTITLEMENT_TYPE.DIP_WITHOUT_DLC ? 
                    EntitlementId.DIP_LARGE : EntitlementId.NON_DIP_LARGE);
            DLCOfferID = null;
            DLCName = null;
            userAccount = AccountManager.getEntitledUserAccount(entitlement);
        }

        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        final String username = userAccount.getUsername();
        final String downloadingEntitlementsName = (entitlementType == ENTITLEMENT_TYPE.DIP_WITH_DLC ? entitlementName + " and " + DLCName : entitlementName);

        logFlowPoint("Log into Origin with " + username); // 1
        logFlowPoint("Navigate to the Game Library Page "); // 2
        logFlowPoint("Begin Downloading " + downloadingEntitlementsName); // 3
        if (pause) {
            logFlowPoint("Pause the Download of " + downloadingEntitlementsName); // 4
        }
        if (resume) {
            logFlowPoint("Resume the Download of " + downloadingEntitlementsName); // 5
        }
        logFlowPoint("Cancel the Download of " + entitlementName); // 6
        if (entitlementType == ENTITLEMENT_TYPE.DIP_WITH_DLC ) {
            logFlowPoint("Cancel the Download of " + DLCName); // 7
        }

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        // 2
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.waitForPageToLoad();
        Thread.sleep(2000);
        NavigationSidebar navBar = new NavigationSidebar(driver);
        navBar.gotoGameLibrary();
        Thread.sleep(5000);
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        if (gameTile.waitForDownloadable()) {
            logPass("Navigated to Game Library.");
        } else {
            logFailExit("Could not navigate to Game Library.");
        }

        // 3
        if (MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferId, downloadOptions) && 
                ((entitlementType == ENTITLEMENT_TYPE.DIP_WITH_DLC) ? Waits.pollingWait(() -> new DownloadQueueTile(driver, DLCOfferID).isGameQueued()) : true)) {
            logPass("Successfully started downloading the entitlement.");
        } else {
            logFailExit("Could not start downloading the entitlement.");
        }

        // 4
        if (pause) {
            gameTile.pauseDownload();
            if (Waits.pollingWait(() -> gameTile.waitForPaused()) && 
                    ((entitlementType == ENTITLEMENT_TYPE.DIP_WITH_DLC) ? new DownloadQueueTile(driver, DLCOfferID).isGameQueued() : true)) {
                logPass("Successfully paused the download of the entitlement.");
            } else {
                logFailExit("Could not pause the download of the entitlement.");
            }
        }

        // 5
        if (resume) {
            gameTile.resumeDownload();
            if (Waits.pollingWait(() -> gameTile.waitForDownloading()) && 
                    ((entitlementType == ENTITLEMENT_TYPE.DIP_WITH_DLC) ? new DownloadQueueTile(driver, DLCOfferID).isGameQueued() : true)) {
                logPass("Successfully resumed downloading the entitlement.");
            } else {
                logFailExit("Could not resume downloading the entitlement.");
            }
        }

        // 6
        gameTile.cancelDownload();
        CancelDownload cancelDialog = new CancelDownload(driver);
        cancelDialog.isOpen();
        cancelDialog.clickYes();
        if (Waits.pollingWait(() -> gameTile.waitForDownloadable())) {
            logPass("Successfully cancelled the entitlement download.");
        } else {
            logFailExit("Could not cancel the download of the entitlement.");
        }
        
        // 7
        if (entitlementType == ENTITLEMENT_TYPE.DIP_WITH_DLC ) {
            if(!new DownloadManager(driver).isDownloadManagerVisible()) {
                logPass("Successfully cancelled the DLC download.");
            } else {
                logFailExit("Could not cancel the download of the DLC.");
            }
        }

        softAssertAll();
    }
}
