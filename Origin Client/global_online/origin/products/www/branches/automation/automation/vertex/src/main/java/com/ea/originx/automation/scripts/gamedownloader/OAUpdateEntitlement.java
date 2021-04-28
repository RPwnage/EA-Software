package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadEULASummaryDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.GameUpToDateDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests updating a game as well as verifying already up-to-date games cannot be
 * updated again
 *
 * @author lscholte
 */
public class OAUpdateEntitlement extends EAXVxTestTemplate {

    @TestRail(caseId = 1633228)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testUpdateEntitlement(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount user = AccountManager.getEntitledUserAccount(entitlement);

        logFlowPoint("Log into Origin with " + user.getUsername()); //1
        logFlowPoint("Navigate to the 'Game Library' page"); //2
        logFlowPoint("Download and install " + entitlementName); //3
        logFlowPoint("Update " + entitlementName + " and verify that the 'Up to Date' dialog appears"); //4
        logFlowPoint("Close the dialog"); //5
        logFlowPoint("Navigate to the 'Application Settings' page"); //6
        logFlowPoint("Disable the 'Automatic game updates' setting"); //7
        logFlowPoint("Exit Origin and add an EACore override for a medium-sized update to " + entitlementName); //8
        logFlowPoint("Log back into Origin with the user"); //9
        logFlowPoint("Navigate to the 'Game Library' page"); //10
        logFlowPoint("Update " + entitlementName + " and verify the update has started"); //11
        logFlowPoint("Wait until the update has finished"); //12
        logFlowPoint("Update " + entitlementName + " and verify that a dialog appears stating it is already up to date"); //13

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //3
        logPassFail(MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId), true);

        //4
        new GameTile(driver, entitlementOfferId).updateGame();
        GameUpToDateDialog upToDateDialog1 = new GameUpToDateDialog(driver);
        boolean upToDateDialogOpened = upToDateDialog1.waitIsVisible() && upToDateDialog1.isOpen();
        logPassFail(upToDateDialogOpened, true);

        //5
        upToDateDialog1.close();
        logPassFail(Waits.pollingWait(() -> !upToDateDialog1.waitIsVisible()), true);

        //6
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectApplicationSettings();
        SettingsNavBar settingsNavBar = new SettingsNavBar(driver);
        settingsNavBar.clickApplicationLink();
        AppSettings appSettings = new AppSettings(driver);
        logPassFail(appSettings.verifyAppSettingsReached(), true);

        //7
        appSettings.setKeepGamesUpToDate(false);
        logPassFail(Waits.pollingWait(() -> appSettings.verifyKeepGamesUpToDate(false)), true);

        //8
        mainMenu.selectExit();
        boolean originExited = Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, OriginClientData.ORIGIN_PROCESS_NAME));
        boolean eaCoreUpdated = OADipSmallGame.changeToMediumEntitlementDipSmallOverride(client);
        logPassFail(originExited && eaCoreUpdated, true);

        //9
        client.stop();
        driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //10
        navBar = new NavigationSidebar(driver);
        gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //11
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        gameTile.updateGame();
        DownloadEULASummaryDialog downloadEULASummaryDialog = new DownloadEULASummaryDialog(driver);
        downloadEULASummaryDialog.setLicenseAcceptCheckbox(true);
        downloadEULASummaryDialog.clickNextButton();
        boolean isUpdating = Waits.pollingWait(() -> gameTile.isUpdating() || gameTile.isInstalling());
        logPassFail(isUpdating, true);

        //12
        gameTile.waitForUpdating(180);
        boolean isFinishedUpdating = Waits.pollingWait(() -> !gameTile.isUpdating() && gameTile.isReadyToPlay(),
                300000, 2000, 0); //wait up to 5 minutes for update to finish
        logPassFail(isFinishedUpdating, true);

        //13
        gameTile.updateGame();
        GameUpToDateDialog upToDateDialog2 = new GameUpToDateDialog(driver);
        logPassFail(Waits.pollingWait(() -> upToDateDialog2.isOpen()), true);

        softAssertAll();
    }
}
