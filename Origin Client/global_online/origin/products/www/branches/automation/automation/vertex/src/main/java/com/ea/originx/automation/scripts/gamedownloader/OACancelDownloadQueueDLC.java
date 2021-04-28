package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.resources.games.Battlefield4;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests canceling DLC download from queue
 *
 * @author rchoi
 */
public class OACancelDownloadQueueDLC extends EAXVxTestTemplate {

    @TestRail(caseId = 9641)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testPurchaseEntitlement(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        Battlefield4 baseGame = new Battlefield4();

        String baseGameOfferID, baseGameName, expansionOfferID, expansionName;
        if (OSInfo.isProduction()) { // Standard Edition + Legacy Operations
            baseGameOfferID = baseGame.getOfferId();
            baseGameName = baseGame.getName();
            expansionOfferID = baseGame.BF4_LEGACY_OPERATIONS_OFFER_ID;
            expansionName = baseGame.BF4_LEGACY_OPERATIONS_NAME;
        } else { // Premium edition + Final Stand
            baseGameOfferID = baseGame.getOfferId();
            baseGameName = baseGame.getName();
            expansionOfferID = baseGame.BF4_FINAL_STAND_OFFER_ID;
            expansionName = baseGame.BF4_FINAL_STAND_NAME;
        }

        // use account which has BF 4 Premium and Expansion 'Final Stand'
        UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.DLC_TEST_ACCOUNT);

        logFlowPoint("Login to Origin"); // 1
        logFlowPoint("Navigate to game library"); // 2
        logFlowPoint("Start downloading " + baseGameName + " and verify if " + expansionName + " is queued"); // 3
        logFlowPoint("Cancel downloading for " + expansionName + " from queue manager and verify if " + expansionName + " does NOT exist in queue manager"); // 4

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(new GameLibrary(driver).verifyGameLibraryPageReached(), true);

        // 3
        MacroGameLibrary.startDownloadingEntitlement(driver, baseGameOfferID, new DownloadOptions().setUncheckExtraContent(false));
        sleep(1000);  // Wait for all items to be queued
        DownloadQueueTile expansionDownloadQueueTile = new DownloadQueueTile(driver, expansionOfferID);
        DownloadManager miniDownloadManager = new DownloadManager(driver);
        miniDownloadManager.clickRightArrowToOpenFlyout();        
        logPassFail(expansionDownloadQueueTile.verifyIsInQueue(), true);

        // 4
        expansionDownloadQueueTile.clickCancelClearButton();
        boolean isExpensionRemovedFromQueue = Waits.pollingWaitEx(() -> !expansionDownloadQueueTile.verifyIsInQueue());
        logPassFail(isExpensionRemovedFromQueue, true);

        softAssertAll();
    }
}
