package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSettings;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueue;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;
import java.util.List;
import java.util.Arrays;

/**
 * Test starting to repair base game also adds DLC to the queue for repair.
 * 
 * @author sunlee
 */
public class OACascadeRepairDLC extends EAXVxTestTemplate{
        
    @TestRail(caseId = 9952)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testCascadeRepairDLC (ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo baseGame =  EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_DIGITAL_DELUXE);
                
        String baseGameOfferID = baseGame.getOfferId();
        EntitlementInfo expansion = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_DLC_DIGITAL_SOUNDTRACK);
        String expansionOfferID = expansion.getOfferId();
        
        List<String> expansionOfferIDList = Arrays.asList(expansionOfferID);
        
        // use account which has Sims 4 Digital Deluxe and DLC, 'Digital Soundtrack'
        UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.ENTITLEMENTS_DOWNLOAD_PLAY_TEST_ACCOUNT);

        logFlowPoint("Launch Origin and login to Origin with an account that has DLC"); // 1
        logFlowPoint("Ensure 'Automatically keep my games up to date' setting is enabled"); // 2
        logFlowPoint("Download and install base game with DLC"); // 3
        logFlowPoint("Reparing the entitlement added installed DLC to queue"); // 4
        logFlowPoint("Base entitlement is added to the queue before the DLC"); // 5

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroSettings.setKeepGamesUpToDate(driver, true), true);
        
        // 3
        new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(MacroGameLibrary.downloadFullEntitlementWithSelectedDLC(driver, baseGameOfferID, new DownloadOptions(), expansionOfferIDList), true);

        // 4
        DownloadManager miniDownloadManager = new DownloadManager(driver);
        miniDownloadManager.openDownloadQueueFlyout();
        new DownloadQueue(driver).clearCompletedDownloads();
        final GameTile baseGameTile = new GameTile(driver, baseGameOfferID);
        baseGameTile.repair();
        miniDownloadManager.clickRightArrowToOpenFlyout();
        DownloadQueueTile baseDownloadQueueTile = new DownloadQueueTile(driver, baseGameOfferID);
        DownloadQueueTile expansionDownloadQueueTile = new DownloadQueueTile(driver, expansionOfferID);
        boolean isExpansionAddedToQueue = Waits.pollingWait(() -> expansionDownloadQueueTile.verifyIsInQueue());
        logPassFail(isExpansionAddedToQueue, true);
        
        // 5
        sleep(2000);
        logPassFail(Waits.pollingWait(() -> baseDownloadQueueTile.isGameDownloading()), true);

        softAssertAll();
    }
}