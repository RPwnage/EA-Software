package com.ea.originx.automation.scripts.feature.gamedownloader;

import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test of launching a game in offline mode
 *
 * @author cvanichsarn
 */
public class OALaunchWhileOffline extends EAXVxTestTemplate {

    @TestRail(caseId = 3068747)
    @Test(groups = {"gamedownloader", "gamedownloader_smoke", "client_only"})
    public void testLaunchOffline(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        // DiP small
        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String username = userAccount.getUsername();

        logFlowPoint("Launch Origin and login to user: " + username); //1
        logFlowPoint(String.format("From user account, navigate to game library and locate '%s' game tile", entitlementName)); //2
        logFlowPoint(String.format("From user account, download '%s' game from user", entitlementName)); //3
        logFlowPoint("Select 'Go Offline' from the Origin menu"); //4
        logFlowPoint(String.format("From user account, launch '%s' from user", entitlementName)); //5

        //1
        final WebDriver driverUser = startClientObject(context, client);
        if (MacroLogin.startLogin(driverUser, userAccount)) {
            logPass("Verified login successful to user: " + userAccount.getUsername());
        } else {
            logFailExit("Failed: Cannot login to user: " + userAccount.getUsername());
        }

        //2
        new NavigationSidebar(driverUser).gotoGameLibrary();
        GameTile gameTile = new GameTile(driverUser, entitlementOfferId);
        if (gameTile.waitForDownloadable()) {
            logPass(String.format("Verified user successfully navigated to 'Game Library' with '%s' game tile", entitlementName));
        } else {
            logFailExit(String.format("Failed: User cannot navigate to 'Game Library' or locate '%s' game tile", entitlementName));
        }

        //3
        boolean downloaded = MacroGameLibrary.downloadFullEntitlement(driverUser, entitlementOfferId);
        if (downloaded) {
            logPass(String.format("Verified user successful downloaded '%s'", entitlementName));
        } else {
            logFailExit(String.format("Failed: User cannot download '%s'", entitlementName));
        }

        //4
        MainMenu mainMenu = new MainMenu(driverUser);
        mainMenu.selectGoOffline();
        sleep(1000); // wait for Origin menu to update
        if (mainMenu.verifyItemEnabledInOrigin("Go Online")) {
            logPass("Verified client is offline");
        } else {
            logFailExit("Failed: Client cannot 'Go Offline'");
        }

        //5
        gameTile.play();
        if (Waits.pollingWaitEx(() -> entitlement.isLaunched(client))) {
            logPass(String.format("Verified user successful launches '%s'", entitlementName));
        } else {
            logFailExit(String.format("Failed: User cannot launch '%s'", entitlementName));
        }

        softAssertAll();
    }
}
