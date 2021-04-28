package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FileUtil;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadEULASummaryDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that game directories are not created if the EULA dialog has not been
 * accepted when downloading a game
 *
 * @author lscholte
 */
public class OAEULADialogCancel extends EAXVxTestTemplate {

    @TestRail(caseId = 9883)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testEULADialogCancel(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount user = AccountManager.getEntitledUserAccount(entitlement);
        final DownloadOptions optionsStopAtEULA = new DownloadOptions().setStopAtEULA(true);

        logFlowPoint("Login to Origin as " + user.getUsername()); //1
        logFlowPoint("Navigate to game library"); //2
        logFlowPoint("Start Downloading " + entitlementName + " and get to the EULA Dialog"); //3
        logFlowPoint("Click Cancel on the EULA dialog"); //4
        logFlowPoint("Verify no game directory is created for " + entitlementName); //5
        logFlowPoint("Start Downloading " + entitlementName + " and get to the EULA Dialog"); //6
        logFlowPoint("Exit Origin"); //7
        logFlowPoint("Verify no game directory is created for " + entitlementName); //8
        logFlowPoint("Login to Origin"); //9
        logFlowPoint("Navigate to game library"); //10
        logFlowPoint("Start Downloading " + entitlementName + " and get to the EULA Dialog"); //11
        logFlowPoint("Close Origin by pressing the X button in the upper-right corner"); //12
        logFlowPoint("Verify no game directory is created for " + entitlementName); //13

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin with the user");
        } else {
            logFailExit("Could not log into Origin with the user");
        }

        //2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Navigate to Game Library");
        } else {
            logFailExit("Could not navigate to Game Library");
        }

        //3
        if (MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId, optionsStopAtEULA)) {
            logPass("Sucessfully arrived at the EULA Dialog");
        } else {
            logFailExit("Did not arrive at the EULA Dialog");
        }

        //4
        DownloadEULASummaryDialog eulaDialog = new DownloadEULASummaryDialog(driver);
        if (eulaDialog.closeAndWait()) {
            logPass("Successfully cancelled the EULA dialog");
        } else {
            logFailExit("Failed to cancel the EULA dialog");
        }

        //5
        final String entitlementGameFolderPath = entitlement.getDirectoryFullPath(client);
        if (!FileUtil.isDirectoryExist(client, entitlementGameFolderPath)) {
            logPass("A game folder was not created for " + entitlementName);
        } else {
            logFailExit("A game folder was created for " + entitlementName);
        }

        //6
        if (MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId, optionsStopAtEULA)) {
            logPass("Sucessfully arrived at the EULA Dialog");
        } else {
            logFailExit("Did not arrive at the EULA Dialog");
        }

        //7
        new MainMenu(driver).selectExit();
        if (Waits.pollingWaitEx(() -> !ProcessUtil.isProcessRunning(client, OriginClientData.ORIGIN_PROCESS_NAME))) {
            logPass("Successfully exited Origin");
        } else {
            logFailExit("Failed to exit Origin");
        }

        //8
        if (!FileUtil.isDirectoryExist(client, entitlementGameFolderPath)) {
            logPass("A game folder was not created for " + entitlementName);
        } else {
            logFailExit("A game folder was created for " + entitlementName);
        }

        //9
        client.stop();
        driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin with the user");
        } else {
            logFailExit("Could not log into Origin with the user");
        }

        //10
        navBar = new NavigationSidebar(driver);
        gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Navigate to Game Library");
        } else {
            logFailExit("Could not navigate to Game Library");
        }

        //11
        if (MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId, optionsStopAtEULA)) {
            logPass("Sucessfully arrived at the EULA Dialog");
        } else {
            logFailExit("Did not arrive at the EULA Dialog");
        }

        //12
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.clickCloseButton();
        if (EAXVxTestTemplate.isOriginClientClosed(driver)) {
            logPass("Successfully closed Origin using the X button");
        } else {
            logFailExit("Failed to close Origin using the X button");
        }

        //13
        if (!FileUtil.isDirectoryExist(client, entitlementGameFolderPath)) {
            logPass("A game folder was not created for " + entitlementName);
        } else {
            logFailExit("A game folder was created for " + entitlementName);
        }

        softAssertAll();
    }

}
