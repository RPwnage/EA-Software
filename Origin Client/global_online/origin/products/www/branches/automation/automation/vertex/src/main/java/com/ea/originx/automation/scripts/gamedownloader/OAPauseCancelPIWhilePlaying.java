package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
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
 * Test to try clicking pause and cancel buttons for the PI entitilement while
 * the game is launched
 *
 * @author nvarthakavi
 */
public class OAPauseCancelPIWhilePlaying extends EAXVxTestTemplate {

    @TestRail(caseId = 9937)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testPauseCancelPIWhilePlaying(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlementPI = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.PI);
        final String entitlementNamePI = entitlementPI.getName();
        final String offerIdPI = entitlementPI.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlementPI);
        String username = userAccount.getUsername();

        logFlowPoint(String.format("Launch Origin and login as user %s entitled to '%s'", username, entitlementNamePI));//1
        logFlowPoint(String.format("Navigate to the Game Library and verify game '%s' appear as downloadable", entitlementNamePI)); //2
        logFlowPoint(String.format("Start downloading game '%s' and Verify the game is downloading", entitlementNamePI)); //3
        logFlowPoint(String.format("Download the game '%s' until playable and launch the game and Verify it is launched", entitlementNamePI)); //4
        logFlowPoint(String.format("Verify game '%s''s pause and cancel buttons are disabled", entitlementNamePI)); //5

        //1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();
        GameTile gameTilePI = new GameTile(driver, offerIdPI);
        logPassFail(gameTilePI.waitForDownloadable(), true);

        //3
        MacroGameLibrary.startDownloadingEntitlement(driver, offerIdPI);
        DownloadQueueTile queueTilePI = new DownloadQueueTile(driver, offerIdPI);
        boolean entitlementPIDownloading = Waits.pollingWait(queueTilePI::isGameDownloading);
        logPassFail(entitlementPIDownloading, true);

        //4
        gameTilePI.waitForReadyToPlay(200);
        // Change download rate to 5MB in order to be able to check other step before completing the download.
        TestScriptHelper.setMaxDownloadRateBpsOutOfGame(driver, "5000000");
        gameTilePI.play();
        logPassFail(Waits.pollingWaitEx(() -> entitlementPI.isLaunched(client)), true);

        //5
        new DownloadManager(driver).openDownloadQueueFlyout();
        logPassFail(Waits.pollingWait(() -> queueTilePI.isPauseButtonDisabled() && queueTilePI.isDownloadCancelButtonDisabled(), 20000, 500, 0), true);

        softAssertAll();
    }
}
