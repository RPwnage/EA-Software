package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.settings.InstallSaveSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test downloading and launching for one of test entitlements
 *
 * @author rchoi
 */
public class OADownloadandPlay extends EAXVxTestTemplate {

    @TestRail(caseId = 1016754)
    @Test(groups = {"gamedownloader", "client_only", "full_regression", "release_smoke"})
    public void testDownloadandPlay(ITestContext context) throws Exception {
        // Download "The Sims 4", which is the most downloaded game on Origin
        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_DIGITAL_DELUXE);
        String entitlementName = entitlement.getName();
        String entitlementOfferID = entitlement.getOfferId();

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount account = AccountManagerHelper.getTaggedUserAccount(AccountTags.ENTITLEMENTS_DOWNLOAD_PLAY_TEST_ACCOUNT);
        final String accountName = account.getUsername();

        logFlowPoint("Launch Origin and login to client"); //1
        logFlowPoint("Navigate to game library and verify the test entitlement " + entitlementName + " is downloadable "); //2
        logFlowPoint("Start Downloading the Entitlement"); //3
        logFlowPoint("Verify the test entitlement is downloaded and installed along with extra content if exists"); //4
        logFlowPoint("Launch the test entitlement and verify it is successfully launched"); //5

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, account)) {
            logPass(String.format("Verified login successful to user account: %s", accountName));
        } else {
            logFailExit(String.format("Failed: Cannot login to user account: %s", accountName));
        }

        //2
        // disable cloud save to prevent showing cloud save dialog when launching test entitlement
        new MainMenu(driver).selectApplicationSettings();
        new SettingsNavBar(driver).clickInstallSaveLink();
        InstallSaveSettings installSettings = new InstallSaveSettings(driver);
        Waits.pollingWait(() -> installSettings.verifyInstallSaveSettingsReached());
        installSettings.setCloudSave(false);
        new NavigationSidebar(driver).gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, entitlementOfferID);
        if (gameTile.waitForDownloadable()) {
            logPass("Verified " + entitlementName + " is downloadable.");
        } else {
            logFailExit("Failed: unable to verify " + entitlementName + " is downloadable");
        }

        //3
        if (MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferID, new DownloadOptions().setUncheckExtraContent(false))) {
            logPass("Verified " + entitlementName + " starts downloading");
        } else {
            logFailExit("Failed: unable to verify downloading for " + entitlementName);
        }

        //4
        // wait up to 50 minutes for the download to complete
        if (Waits.pollingWait(() -> gameTile.isReadyToPlay() && !new DownloadManager(driver).isGameTitleVisible(), 3000000, 30000, 0)) {
            logPass("Verified test entitlement is downloaded and installed along with extra content if exist");
        } else {
            logFailExit("Failed: unable to verify test entitlement is downloaded and installed");
        }

        //5
        boolean isLaunched;
        int i = 0;
        entitlement.addActivationFile(client);
        // If it fails, then it kills the attempt to load, and tries again.
        do {
            entitlement.killLaunchedProcess(client);
            gameTile.play();
            isLaunched = entitlement.isLaunched(client);
            i++;
        } while (!isLaunched && i <= 1);
        if (isLaunched) {
            logPass(String.format("Verified '%s' launches successfully", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot launch '%s'", entitlementName));
        }
        entitlement.killLaunchedProcess(client);

        softAssertAll();
    }
}
