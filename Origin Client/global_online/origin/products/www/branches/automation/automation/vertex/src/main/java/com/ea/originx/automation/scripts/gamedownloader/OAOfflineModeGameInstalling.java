package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.SystemUtilities;
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
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;

import static com.ea.vx.originclient.templates.OATestBase.sleep;

import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test for going offline while game is installing
 *
 * @author palui
 */
public class OAOfflineModeGameInstalling extends EAXVxTestTemplate {

    @TestRail(caseId = 39455)
    @Test(groups = {"gamedownloader", "client_only", "full_regression", "int_only"})
    public void testOfflineModeGameInstalling(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.PI);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String username = userAccount.getUsername();

        logFlowPoint(String.format("Launch Origin and login as user: %s", username)); //1
        logFlowPoint(String.format("Navigate to game library and locate '%s' game tile", entitlementName)); //2
        logFlowPoint(String.format("Start downloading '%s' game", entitlementName)); //3
        logFlowPoint(String.format("Wait for '%s' to start installing", entitlementName)); //4
        logFlowPoint("Click 'Go Offline' at the Origin menu and verify client is offline"); //5
        logFlowPoint(String.format("Verify '%s' continues to install", entitlementName)); //6
        logFlowPoint(String.format("Wait for '%s' to complete install and ready to play", entitlementName)); //7
        logFlowPoint(String.format("Launch '%s'", entitlementName)); //8

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass(String.format("Verified login successful as user: %s", username));
        } else {
            logFailExit(String.format("Failed: Cannot login as user: %s", username));
        }

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        if (gameTile.waitForDownloadable()) {
            logPass(String.format("Verified successful navigation to Game Library with '%s' game tile", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library or locate '%s' game tile", entitlementName));
        }

        //3
        if (MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferId)) {
            logPass(String.format("Verified '%s' starts downloading", entitlementName));
        } else {
            logFailExit(String.format("Failed: '%s' cannot start downloading", entitlementName));
        }

        //4
        // Wait up to 60 secs for install to start
        if (gameTile.waitForInstalling(60)) {
            logPass(String.format("Verified '%s' starts installing", entitlementName));
        } else {
            logFailExit(String.format("Failed: '%s' cannot start installing", entitlementName));
        }

        //5
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectGoOffline();
        sleep(1000); // wait for Origin menu to update
        if (mainMenu.verifyItemEnabledInOrigin("Go Online")) {
            logPass("Verified client is offline");
        } else {
            logFailExit("Failed: Client cannot 'Go Offline'");
        }

        //6
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60); // Go back to SPA page - required after going offline
        new OfflineFlyout(driver).closeOfflineFlyout(); // close 'Offline flyout' which may interfere with game tiles operations
        sleep(1000); // pause for changes to happen if any
        if (Waits.pollingWait(() -> gameTile.isInstalling())) {
            logPass(String.format("Verified '%s' continues installing after going offline", entitlementName));
        } else {
            logFailExit(String.format("Failed: '%s' installing has been interrupted by going offline", entitlementName));
        }

        //7
        // Wait up to 60 secs for game to be ready-to-play
        if (gameTile.waitForReadyToPlay(60)) {
            logPass(String.format("Verified '%s' successfully completes install and is ready to play", entitlementName));
        } else {
            logFailExit(String.format("Failed: '%s' cannot complete install and is not ready to play", entitlementName));
        }

        //8
        gameTile.play();
        if (Waits.pollingWaitEx(() -> entitlement.isLaunched(client))) {
            logPass(String.format("Verified '%s' launches successfully", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot launch '%s'", entitlementName));
        }

        softAssertAll();
    }
}
