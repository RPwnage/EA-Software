package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CancelDownload;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;

import static com.ea.vx.originclient.templates.OATestBase.sleep;

import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test to pause, resume, and cancel the downloading of a DiP or Zip
 * entitlement. There are two parameterized test cases attached to this:<br>
 * - OABasicPauseResumeCancelZip<br>
 * - OABasicPauseResumeCancelDiP<br>
 *
 * @author glivingstone
 */
public abstract class OABasicPauseResumeCancel extends EAXVxTestTemplate {

    /**
     * The main test function which all parameterized test cases call.
     *
     * @param context     The context we are using
     * @param entitlement The entitlement to use for the test (DiP Large or
     *                    Non-Dip Large)
     * @param testName    The name of the test. We need to pass this up so
     *                    initLogger properly attaches to the CRS test case.
     * @throws Exception
     */
    public void testBasicPauseResumeCancel(ITestContext context, EntitlementInfo entitlement, String testName) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);

        logFlowPoint("Log into Origin with " + userAccount.getUsername()); // 1
        logFlowPoint("Navigate to the Game Library Page "); // 2
        logFlowPoint("Begin Downloading " + entitlementName); // 3
        logFlowPoint("Pause the Download of " + entitlementName); // 4
        logFlowPoint("Resume the Download of " + entitlementName); // 5
        logFlowPoint("Cancel the Download of " + entitlementName); // 6

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
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        if (gameTile.waitForDownloadable()) {
            logPass("Navigated to Game Library.");
        } else {
            logFailExit("Could not navigate to Game Library.");
        }

        // 3
        if (MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferId)) {
            logPass("Successfully started downloading the entitlement.");
        } else {
            logFailExit("Could not start downloading the entitlement.");
        }

        // 4
        gameTile.pauseDownloadCaseInsensitive();
        sleep(1000);
        if (gameTile.waitForPaused()) {
            logPass("Successfully paused the download of the entitlement.");
        } else {
            logFailExit("Could not pause the download of the entitlement.");
        }

        // 5
        gameTile.resumeDownloadCaseInsensitive();
        sleep(1000);
        if (gameTile.waitForDownloading()) {
            logPass("Successfully resumed downloading the entitlement.");
        } else {
            logFailExit("Could not resume downloading the entitlement.");
        }

        // 6
        gameTile.cancelDownloadCaseInsensitive();
        sleep(1000);
        CancelDownload cancelDialog = new CancelDownload(driver);
        cancelDialog.isOpen();
        cancelDialog.clickYes();
        if (gameTile.waitForDownloadable()) {
            logPass("Successfully cancelled the entitlement download.");
        } else {
            logFailExit("Could not cancel the download of the entitlement.");
        }

        softAssertAll();
    }
}
