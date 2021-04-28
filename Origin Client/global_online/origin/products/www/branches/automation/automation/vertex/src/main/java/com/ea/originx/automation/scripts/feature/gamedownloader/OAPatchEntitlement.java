package com.ea.originx.automation.scripts.feature.gamedownloader;

import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadEULASummaryDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests patching an entitlement - DIP Small
 *
 * @author nvarthakavi
 */
public class OAPatchEntitlement extends EAXVxTestTemplate {

    @TestRail(caseId = 3069085)
    @Test(groups = {"gamelibrary", "gamedownloader_smoke", "gamedownloader", "client_only", "allfeaturesmoke"})
    public void testPatchEntitlement(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        String username = userAccount.getUsername();

        logFlowPoint(String.format("Launch Origin and login as user %s entitled to '%s'", username, entitlementName));//1
        logFlowPoint(String.format("Navigate to the Game Library and verify game '%s' appear as downloadable", entitlementName)); //2
        logFlowPoint(String.format("Completely download game '%s' and Verify the game is downloaded completed", entitlementName)); //3
        logFlowPoint(String.format("Exit from Origin and Override the EACore to set an update for game '%s'", entitlementName)); //4
        logFlowPoint(String.format("Log back into Origin with the user '%s'", username)); //5
        logFlowPoint(String.format("Navigate to the game library and check the game '%s' is updating automatically", entitlementName));//6
        logFlowPoint(String.format("Wait for the update to complete and verify the download is complete", entitlementName));//7
        logFlowPoint(String.format("Launch the entitlement '%s' and verify it is launched", entitlementName));//8

        //1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        logPassFail(gameTile.waitForDownloadable(), true);

        //3
        boolean entitlementDownloaded = MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId);
        // Change download rate to 16KB in order to be able to check the rest steps before completing the download.
        TestScriptHelper.setMaxDownloadRateBpsOutOfGame(driver, "16000");
        logPassFail(entitlementDownloaded, true);

        //4
        new MainMenu(driver).selectExit();
        boolean originExited = Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, OriginClientData.ORIGIN_PROCESS_NAME));
        boolean eaCoreUpdated = OADipSmallGame.changeToMediumEntitlementDipSmallOverride(client);
        logPassFail(originExited && eaCoreUpdated, true);

        //5
        client.stop();
        final WebDriver driverNew = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driverNew, userAccount), true);

        //6
        new NavigationSidebar(driverNew).gotoGameLibrary();
        new GameLibrary(driverNew).waitForPage();
        GameTile gameTileNew = new GameTile(driverNew, entitlementOfferId);
        boolean isUpdating = Waits.pollingWait(() -> gameTileNew.isUpdating() || gameTileNew.isInstalling());
        logPassFail(isUpdating, true);

        //7
        // Change download rate back to "No limit".
        TestScriptHelper.setMaxDownloadRateBpsOutOfGame(driverNew, "0");
        DownloadEULASummaryDialog downloadEULASummaryDialog = new DownloadEULASummaryDialog(driverNew);
        if (Waits.pollingWait(() -> downloadEULASummaryDialog.isOpen(), 300000, 2000, 0)) {
            downloadEULASummaryDialog.setLicenseAcceptCheckbox(true);
            downloadEULASummaryDialog.clickNextButton(); // click 'Install Update' button
        }
        boolean isFinishedUpdating = Waits.pollingWait(() -> !gameTileNew.isUpdating() && gameTileNew.isReadyToPlay(),
                300000, 2000, 0); //wait up to 5 minutes for update to finish
        logPassFail(isFinishedUpdating, true);

        //8
        gameTileNew.play();
        logPassFail(Waits.pollingWaitEx(() -> entitlement.isLaunched(client)), true);

        softAssertAll();

    }
}
