package com.ea.originx.automation.scripts.ito;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.InstallationSettingsChangedDialog;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests opening Origin and installation via Install Through Origin
 *
 * @author sbentley
 */
public class OAInstallThroughOrigin extends EAXVxTestTemplate {

    @TestRail(caseId = 1016756)
    @Test(groups = {"release_smoke", "ito", "ito_smoke", "client_only", "allfeaturesmoke"})
    public void OAInstallThroughOrigin(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlementInfo = new OADipSmallGame();
        final String entitlementName = entitlementInfo.getName();
        final UserAccount user = AccountManager.getEntitledUserAccount(entitlementInfo);

        logFlowPoint("Start ITO for an entitlement and verify the Origin client launches"); // 1
        logFlowPoint("Log into an account with an ITO entitlement and verify the 'Installation Settings' dialog appears"); // 2

        // 1
        //Path and Commands to start origin.exe with ITO
        String ITO_PATH = "C:\\automation\\SmallITO";
        String[] commands = {
            SystemUtilities.getOriginPath(client),
            "/noUpdate",
            String.format("/contentLocation:%s\\automation_dip_small.zip", ITO_PATH),
            "/runEADM",
            String.format("/contentIds:\"%s\\AutoRun\\installerdata.xml\"", ITO_PATH),
            "/language:en_US"};
        final String command = String.join(" ", commands);
        ProcessUtil.launchProcess(client, command);
        if (Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, "origin.exe"))) {
            logPass("Successfully launched Origin using ITO for " + entitlementName);
        } else {
            logFailExit("Failed to launch Origin using ITO for " + entitlementName);
        }

        // 2
        //Need to close Origin and reopen to continue with test normally.
        //Added a long minimum wait time so that Origin isn't killed too soon that it won't open with ITO
        Waits.pollingWait(() -> ProcessUtil.killProcess(client, "origin.exe"), 10000, 1000, 10000);
        WebDriver driver = startClientObject(context, client);
        boolean isLoginSuccessful = MacroLogin.startLogin(driver, user);
        InstallationSettingsChangedDialog installDialog = new InstallationSettingsChangedDialog(driver);
        // After logging in, the 'Installation Settings' dialog can occasionally take time to open
        boolean isInstallationSettingsDialogOpen = Waits.pollingWait(() -> installDialog.isOpen(), 120000, 1000, 0);
        if (isLoginSuccessful && isInstallationSettingsDialogOpen) {
            logPass("Logged into client and verified that installation started through Origin");
        } else {
            logFailExit("Failed to log into client or verify that installation started through Origin");
        }

        softAssertAll();
    }
}