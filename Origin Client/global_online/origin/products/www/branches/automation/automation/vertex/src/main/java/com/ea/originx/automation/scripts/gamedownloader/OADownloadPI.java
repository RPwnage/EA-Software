package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test behavior of downloading a PI entitlement
 *
 * @author palui
 */
public class OADownloadPI extends EAXVxTestTemplate {

    @TestRail(caseId = 9604)
    @Test(groups = {"gamedownloader", "client_only", "full_regression", "gamedownloader_smoke", "int_only"})
    public void testDownloadPI(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.PI);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String username = userAccount.getUsername();
        EACoreHelper.changeToBaseEntitlementPIOverride(client.getEACore());

        logFlowPoint(String.format("Launch Origin and login as user: %s", username)); //1
        logFlowPoint(String.format("Navigate to game library and locate '%s' game tile", entitlementName)); //2
        logFlowPoint(String.format("Start downloading '%s' game", entitlementName)); //3
        logFlowPoint(String.format("Wait for '%s' to start installing", entitlementName)); //4
        logFlowPoint(String.format("Verify '%s' game tile has playable status indicator of 'PLAYABLE AT ..%%' (less than 100%%)", entitlementName)); //5
        logFlowPoint(String.format("Wait for '%s' to complete install and ready to play", entitlementName)); //6
        logFlowPoint(String.format("Verify '%s' game tile has playable status indicator of 'NOW PLAYABLE'", entitlementName)); //7
        logFlowPoint(String.format("Launch '%s'", entitlementName)); //8

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass(String.format("Verified login successful to user account: %s", username));
        } else {
            logFailExit(String.format("Failed: Cannot login to user account: %s", username));
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
        if (gameTile.verifyOverlayPlayableStatusPercent()) {
            logPass(String.format("Verified '%s' game tile has playable status indicator of 'PLAYABLE AT ..%%' (less than 100%%)", entitlementName));
        } else {
            logFailExit(String.format("Failed: '%s' game tile does not have playable status indicator of 'PLAYABLE AT ..%%' (or not less than 100%%)", entitlementName));
        }

        //6
        // Wait up to 60 secs for game to be ready-to-play
        if (gameTile.waitForReadyToPlay(60)) {
            logPass(String.format("Verified '%s' successfully completes install and is ready to play", entitlementName));
        } else {
            logFailExit(String.format("Failed: '%s' cannot complete install or is not ready to play", entitlementName));
        }

        //7
        if (gameTile.verifyOverlayPlayableStatusNowPlayable()) {
            logPass(String.format("Verified '%s' game tile has playable status indicator of 'NOW PLAYABLE'", entitlementName));
        } else {
            logFailExit(String.format("Failed: '%s' game tile does not have playable status indicator of 'NOW PLAYABLE'", entitlementName));
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
