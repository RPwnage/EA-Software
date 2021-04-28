package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests crashing while downloading PI to see if download will resume after
 * Origin re-launch
 *
 * @author nvarthakavi
 */
public class OACrashDuringPI extends EAXVxTestTemplate {

    /**
     * The updating of game tile for PI download seem to interfere with context
     * menu item 'Play' in this script, even though manually clicking 'Play'
     * works fine. Fix by retrying if game does not start playing
     *
     * @param client Client under test
     * @param driver Selenium web driver
     * @param game   entitlement info for game being downloaded
     * @return true if game launched by clicking the 'Play' context menu item,
     * false otherwise
     */
    private boolean playGame(OriginClient client, WebDriver driver, EntitlementInfo game) {
        new GameTile(driver, game.getOfferId()).play();
        // Note launch method in EntitlementInfo is more bulky than it should. Use simple process running check instaed
        boolean gameLaunched = Waits.pollingWaitEx(()
                -> ProcessUtil.isProcessRunning(client, game.getProcessName()), 60000, 500, 500);
        return gameLaunched;
    }

    @TestRail(caseId = 9612)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testCrashDuringPI(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        EntitlementInfo gamePI = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.PI);
        final String gamePIName = gamePI.getName();
        final String offerIdPI = gamePI.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(gamePI);
        String username = userAccount.getUsername();

        EACoreHelper.changeToBaseEntitlementPIOverride(client.getEACore());

        logFlowPoint(String.format("Launch Origin and login as user %s entitled to '%s'", username, gamePIName));//1
        logFlowPoint(String.format("Navigate to the Game Library and verify game '%s' appear as downloadable", gamePIName)); //2
        logFlowPoint(String.format("Start downloading game '%s' and Verify the game is downloading and Kill Origin process", gamePIName)); //3
        logFlowPoint(String.format("Relaunch Origin and login as user %s and navigate to Game Library", username));//4
        logFlowPoint(String.format("Verify the game '%s' downloads automatically", gamePIName)); //5
        logFlowPoint(String.format("Verify the playable indicator exists on the '%s' entitlement", gamePIName)); //6
        logFlowPoint(String.format("Launch '%s' entitlement and Verify the launch was successfully", gamePIName)); //7

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass(String.format("Verified login successful as user %s", username));
        } else {
            logFailExit(String.format("Failed: Cannot login as user %s", username));
        }

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();
        GameTile gameTilePI = new GameTile(driver, offerIdPI);
        if (gameTilePI.waitForDownloadable()) {
            logPass(String.format("Verified successful navigation to Game Library with downloadable game '%s'",
                    gamePIName));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library or cannot locate downloadable game '%s'",
                    gamePIName));
        }

        //3
        MacroGameLibrary.startDownloadingEntitlement(driver, offerIdPI);
        DownloadQueueTile queueTilePI = new DownloadQueueTile(driver, offerIdPI);
        boolean gamePIDownloading = Waits.pollingWait(queueTilePI::isGameDownloading);
        boolean originKill = Waits.pollingWait(() -> ProcessUtil.killOriginProcess(client));
        if (originKill && gamePIDownloading) {
            logPass(String.format("Verified game '%s' is the downloading and Kill the Origin", gamePIName));
        } else {
            logFailExit(String.format("Failed: game '%s' is not downloading or kill origin process was not successful", gamePIName));
        }

        //4
        client.stop();
        WebDriver driver_new = startClientObject(context, client);
        if (MacroLogin.startLogin(driver_new, userAccount)) {
            logPass(String.format("Verified login successful as user %s and Navigated to Game Library", username));
        } else {
            logFailExit(String.format("Failed: Cannot login as user %s", username));
        }

        //5
        new NavigationSidebar(driver_new).gotoGameLibrary();
        new GameLibrary(driver_new).waitForPage();
        GameTile gameTilePINew = new GameTile(driver_new, offerIdPI);
        boolean gamePIDownloadingNew = gameTilePINew.waitForDownloading(1000);
        if (gamePIDownloadingNew) {
            logPass(String.format("Verified game '%s' is downloading automatically", gamePIName));
        } else {
            logFailExit(String.format("Failed: game '%s' is not downloading automatically", gamePIName));
        }

        //6
        boolean gamePIPlayable = Waits.pollingWait(gameTilePINew::verifyOverlayPlayableStatusNowPlayable, 90000, 500, 0);
        if (gamePIPlayable) {
            logPass(String.format("Verified game '%s' show playable indicator", gamePIName));
        } else {
            logFailExit(String.format("Failed: game '%s' does not show playable indicator", gamePIName));
        }

        //7
        boolean gamePIlaunchable = playGame(client, driver_new, gamePI);
        if (gamePIlaunchable) {
            logPass(String.format("Verified '%s' launches successfully", gamePIName));
        } else {
            logFailExit(String.format("Failed: Cannot launch '%s'", gamePIName));
        }

        softAssertAll();
    }

}
