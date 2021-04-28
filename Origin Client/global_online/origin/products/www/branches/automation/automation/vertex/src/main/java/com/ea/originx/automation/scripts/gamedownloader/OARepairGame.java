package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FileUtil;
import com.ea.vx.originclient.helpers.OriginClientLogReader;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the functionality of repairing a game
 *
 * @author JMittertreiner
 */
public class OARepairGame extends EAXVxTestTemplate {

    @TestRail(caseId = 9834)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testRepairGame(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        OADipSmallGame entitlement = new OADipSmallGame();
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        final String mapCrc = OriginClientData.PROGRAM_DATA_NONXP_PATH
                + "\\LocalContent\\" + entitlementName + "\\map.crc";

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String username = userAccount.getUsername();

        final String totalReqStr = "Total requests: 0";

        logFlowPoint("Launch and log into Origin as " + username); // 1
        logFlowPoint("Install " + entitlementName); // 2
        logFlowPoint("Delete " + mapCrc); // 3
        logFlowPoint("Relog into Origin"); // 4
        logFlowPoint("Open the Client Log and Verify Origin has detected the missing file"); // 5
        logFlowPoint("Repair the game and verify that map.crc is in the LocalContent folder"); // 6
        logFlowPoint("Attempt to repair the game that was just repaired"); // 7
        logFlowPoint("Verify: The 'Origin:Downloader::ContentProtocolPackage::TransferIssueRequests'"
                + " event in the Client Log includes '" + totalReqStr + "' after"
                + " repairing/updating a game that was just repaired");

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin as user");
        } else {
            logFailExit("Could not log into Origin as user");
        }

        // 2
        new NavigationSidebar(driver).gotoGameLibrary();
        if (MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId)) {
            logPass("Successfully installed a game");
        } else {
            logFailExit("Failed to install a game");
        }

        // 3
        if (FileUtil.deleteFile(client, mapCrc)) {
            logPass("Successfully deleted Map.crc");
        } else {
            logFail("Failed to delete Map.crc");
        }

        // 4
        final LoginPage loginPage = new MainMenu(driver).selectLogOut();
        final boolean loggedOut = Waits.pollingWait(() -> loginPage.getCurrentURL().matches(OriginClientData.LOGIN_PAGE_URL_REGEX));
        final boolean loggedIn = MacroLogin.startLogin(driver, userAccount);

        if (loggedOut && loggedIn) {
            logPass("Successfully relogged into Origin as user");
        } else {
            logFail("Could not relog into Origin as user");
        }

        // 5
        final OriginClientLogReader logReader = new OriginClientLogReader(OriginClientData.LOG_PATH);
        if (!logReader.retriveLogLine(client, OriginClientData.CLIENT_LOG, "Unable to load CRC Map").equals("")) {
            logPass("Origin detected the missing file");
        } else {
            logFail("Origin failed to detect the missing file");
        }

        // 6
        new NavigationSidebar(driver).gotoGameLibrary();
        final GameTile dipTile = new GameTile(driver, entitlementOfferId);
        dipTile.repair();
        new DownloadManager(driver).clickRightArrowToOpenFlyout();        

        // Use "playable":false instead of "repairing":true because the amount
        // of time the tile is in the repairing state is very short (less than a
        // second) and the polling tends to miss it
        final boolean startedRepair = dipTile.waitForNotReadyToPlay();
        final boolean finishedRepair = dipTile.waitForReadyToPlay(15);
        final boolean fileExists = FileUtil.isFileExist(client, mapCrc);

        if (startedRepair && finishedRepair && fileExists) {
            logPass("Origin successfully repaired the game");
        } else {
            logFail("Failed to repair the game");
        }

        // 7
        // Cache the number of times totalReqStr appears in the log
        logReader.keyValuesSinceLastScan(client, OriginClientData.CLIENT_LOG, totalReqStr);

        dipTile.repair();
        final boolean startedReRepair = dipTile.waitForNotReadyToPlay();
        final boolean finishedReRepair = dipTile.waitForReadyToPlay(15);
        if (startedReRepair && finishedReRepair) {
            logPass("Re-repaired the repaired game");
        } else {
            logFail("Failed to re-repair the game");
        }

        // 8
        // Verify that origin logged that it made 0 requests while repairing
        final int numReq = logReader.keyValuesSinceLastScan(client, OriginClientData.CLIENT_LOG, totalReqStr);
        if (numReq == 1) {
            logPass("'" + totalReqStr + "' appears in the log after repairing");
        } else {
            logFail("'" + totalReqStr + "' does not appear in the log after repairing");
        }

        softAssertAll();
    }
}
