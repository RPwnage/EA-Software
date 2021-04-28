package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.OfflineFlyout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test going offline while game downloading
 *
 * @author palui
 */
public class OAOfflineModeGameDownloading extends EAXVxTestTemplate {

    @TestRail(caseId = 39453)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testOfflineModeGameDownloading(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();
        final UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String username = userAccount.getUsername();

        logFlowPoint(String.format("Launch Origin and login as user: %s", username)); //1
        logFlowPoint(String.format("Navigate to game library and locate '%s' game tile", entitlementName)); //2
        logFlowPoint(String.format("Start downloading '%s' game. Verify game tile overlay is showing 'DOWNLOADING'", entitlementName)); //3
        logFlowPoint("Click 'Go Offline' at the Origin menu and verify client is offline"); //4
        logFlowPoint(String.format("Verify '%s' downloading pauses with game tile overlay showing 'PAUSED'", entitlementName)); //5
        logFlowPoint("Click 'Go Online' at the Origin menu and verify client is online"); //6
        logFlowPoint(String.format("Verify '%s' downloading resumes automatically with game tile overlay showing 'DOWNLOADING'", entitlementName)); //7
        logFlowPoint(String.format("Wait 30 seconds, then verify '%s' game tile overlay is still showing 'DOWNLOADING'", entitlementName)); //8

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        logPassFail(gameTile.waitForDownloadable(), true);

        //3
        boolean startDownloadingOK = MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferId);
        boolean overlayStatusDownloading = gameTile.verifyOverlayStatusDownloading();
        TestScriptHelper.setMaxDownloadRateBpsOutOfGame(driver, "1000000");
        logPassFail(startDownloadingOK && overlayStatusDownloading, true);

        //4
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectGoOffline();
        sleep(1000); // wait for Origin menu to update
        logPassFail(mainMenu.verifyItemEnabledInOrigin("Go Online"), true);

        //5
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60); // Go back to SPA page - required after going offline
        new OfflineFlyout(driver).closeOfflineFlyout(); // close 'Offline flyout' which may interfere with game tiles operations
        boolean pausedDownloading = Waits.pollingWait(() -> gameTile.isPaused());
        boolean overlayStatusPaused = gameTile.verifyOverlayStatusPaused();
        logPassFail(pausedDownloading && overlayStatusPaused, true);

        //6
        mainMenu.selectGoOnline();
        boolean mainMenuOnline = mainMenu.verifyItemEnabledInOrigin("Go Offline");
        logPassFail(mainMenuOnline, true);

        //7
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60); // Go back to SPA page - required after going back online
        // Seems it is taking longer now to auto-update. Increase poll to 30 secs
        boolean resumedDownloading = Waits.pollingWait(() -> gameTile.isDownloading() && !gameTile.isPaused(), 30000, 1000, 0);
        overlayStatusDownloading = gameTile.verifyOverlayStatusDownloading();
        logPassFail(resumedDownloading && overlayStatusDownloading, true);

        //8
        // Wait 30 seconds to try detect the ORPLAT-6841 issue: overlay vanishing a few seconds after going back online
        sleep(30000);
        boolean stillDownloading = gameTile.isDownloading() && !gameTile.isPaused();
        overlayStatusDownloading = gameTile.verifyOverlayStatusDownloading();
        logPassFail(stillDownloading && overlayStatusDownloading, true);

        softAssertAll();
    }
}