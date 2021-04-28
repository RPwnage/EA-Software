package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.pageobjects.dialog.DownloadLanguageSelectionDialog;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadEULASummaryDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadInfoDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.PunkbusterWarningDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.ProcessUtil;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the punk buster notifications and that it gets installed with
 * installing a game that requires it
 *
 * @author jmittertreiner
 */
public class OAInstallPunkbuster extends EAXVxTestTemplate {

    @TestRail(caseId = 9852)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testInstallPunkbuster(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlementInfo = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF_BAD_COMPANY_2); //Battlefield Bad Company 2 requires Punkbuster
        final UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlementInfo);
        final String userName = userAccount.getUsername();
        final String entitlementName = entitlementInfo.getName();
        final String punkBusterExecutableName = "PnkBstrA.exe";

        logFlowPoint("Launch Origin and login as a user that owns a game that requires 'Punkbuster' to play online"); // 1
        logFlowPoint("Navigate to the 'Game Library' and start downloading the game that requires 'Punkbuster'"); // 2
        logFlowPoint("Verify that 'PunkBuster' is unchecked by default (in Canada)"); // 3
        logFlowPoint("Verify that a warning appears stating 'PunkBuster' is needed to play online"); // 4
        logFlowPoint("Select 'Yes' on the 'PunkBuster Warning' dialog and verify you are brought to EULA"); // 5
        logFlowPoint("Cancel and restart the download of the game that requires 'PunkBuster' to play online"); // 6
        logFlowPoint("Select 'No' on the 'PunkBuster Warning' dialog and verify the user is brought back to the 'Download Information' dialog"); // 7
        logFlowPoint("Complete the installation of the game and verify 'PunkBuster' is installed"); // 8

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully launched Origin and logged in with account " + userName);
        } else {
            logFailExit("Failed: Cannot login to user account: " + userName);
        }

        // 2
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameTile(driver, entitlementInfo.getOfferId()).startDownload();
        DownloadLanguageSelectionDialog downloadLanguageSelectionDialog = new DownloadLanguageSelectionDialog(driver);
        downloadLanguageSelectionDialog.clickAccept();
        DownloadInfoDialog downloadInfoDialog = new DownloadInfoDialog(driver);
        if (downloadInfoDialog.waitIsVisible()) {
            logPass("Began downloading " + entitlementName);
        } else {
            logFailExit("Failed to begin downloading " + entitlementName);
        }

        // 3
        if (!downloadInfoDialog.isPunkBusterChecked()) {
            logPass("Verified that 'PunkBuster' is not checked by default in Canada");
        } else {
            logFail("Failed to verify that 'PunkBuster' is checked by default in Canada");
        }

        // 4
        downloadInfoDialog.clickNext();
        PunkbusterWarningDialog pbWarningDialog = new PunkbusterWarningDialog(driver);
        if (pbWarningDialog.waitIsVisible()) {
            logPass("The 'PunkBuster Warning' dialog appeared");
        } else {
            logFailExit("The 'PunkBuster Warning' dialog did not appear when it should have");
        }

        // 5
        pbWarningDialog.clickYes();
        DownloadEULASummaryDialog eulaSummaryDialog = new DownloadEULASummaryDialog(driver);
        if (eulaSummaryDialog.waitIsVisible()) {
            logPass("Clicking through the 'PunkBuster' warning brings you to the EULA");
        } else {
            logFailExit("Click through the 'PunkBuster' warning did go to the EULA");
        }

        // 6
        eulaSummaryDialog.closeAndWait();
        new GameTile(driver, entitlementInfo.getOfferId()).startDownload();
        downloadLanguageSelectionDialog.clickAccept();
        if (downloadInfoDialog.waitIsVisible()) {
            logPass("Successfully restarted the download process");
        } else {
            logFailExit("Failed to restart the download process");
        }

        // 7
        downloadInfoDialog.clickNext();
        pbWarningDialog.waitForVisible();
        pbWarningDialog.close();
        if (downloadInfoDialog.waitIsVisible()) {
            logPass("Clicking 'No' on the 'PunkBuster Warning' takes you back to the 'Download Information' dialog");
        } else {
            logFailExit("Clicking 'No' on the 'PunkBuster Warning' does not take you back to the 'Download Information' dialog");
        }

        // 8
        downloadInfoDialog.closeAndWait();
        boolean isGameDownloaded = MacroGameLibrary.downloadFullEntitlement(driver, entitlementInfo.getOfferId());
        boolean isPunkBusterRunning = ProcessUtil.isProcessRunning(client,  punkBusterExecutableName);
        if (isGameDownloaded && isPunkBusterRunning) {
            logPass("PunkBuster was installed along with " + entitlementName);
        } else {
            logFail("PunkBuster was not installed along with " + entitlementName);
        }

        softAssertAll();
    }
}
