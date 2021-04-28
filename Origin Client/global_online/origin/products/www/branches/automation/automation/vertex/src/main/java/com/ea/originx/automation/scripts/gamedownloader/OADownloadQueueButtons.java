package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CancelDownload;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueue;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the Buttons on the Download Queue
 *
 * @author glivingstone
 */
public class OADownloadQueueButtons extends EAXVxTestTemplate {

    @TestRail(caseId = 9887)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testDownloadQueuePartialDownload(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo largeEnt = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);
        EntitlementInfo smallEnt = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);

        final String largeEntName = largeEnt.getName();
        final String largeEntOffer = largeEnt.getOfferId();

        final String smallEntName = smallEnt.getName();
        final String smallEntOffer = smallEnt.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(largeEnt, smallEnt);
        String username = userAccount.getUsername();

        logFlowPoint(String.format("Login to Origin as %s", username)); // 1
        logFlowPoint("Navigate to the Game Library."); // 2
        logFlowPoint(String.format("Start downloading %s", largeEntName)); // 3
        logFlowPoint("Verify the User is able to Pause the Download Using the Download Queue"); // 4
        logFlowPoint("Verify the User is able to Resume the Download Using the Download Queue"); // 5
        logFlowPoint("Verify the User is able to Minimize the Download Queue"); // 6
        logFlowPoint("Verify the User is able to Restore the Download Queue"); // 7
        logFlowPoint("Verify the User is able to Cancel the Download Using the Download Queue"); // 8
        logFlowPoint(String.format("Completely download %s", smallEntName)); // 9
        logFlowPoint("Verify the User is able to Close the Download Queue by Clearing the Download"); // 10

        final WebDriver driver = startClientObject(context, client);

        //1
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //3
        logPassFail(MacroGameLibrary.startDownloadingEntitlement(driver, largeEntOffer), true);

        //4
        DownloadManager miniDownloadManager = new DownloadManager(driver);
        miniDownloadManager.clickRightArrowToOpenFlyout();
        DownloadQueueTile largeEntQueueTile = new DownloadQueueTile(driver, largeEntOffer);
        largeEntQueueTile.clickPauseButton();
        GameTile largeEntGameTile = new GameTile(driver, largeEntOffer);
        logPassFail(largeEntGameTile.waitForPaused(), true);

        //5
        largeEntQueueTile.clickDownloadButton();
        logPassFail(largeEntGameTile.waitForDownloading(), true);

        //6
        miniDownloadManager.closeDownloadQueueFlyout();
        DownloadQueue downloadQueue = new DownloadQueue(driver);
        logPassFail(!downloadQueue.isOpen(), true);

        //7
        miniDownloadManager.clickRightArrowToOpenFlyout();
        logPassFail(downloadQueue.isOpen(), true);

        //8
        largeEntQueueTile.clickCancelClearButton();
        CancelDownload cancelDownloadDialog = new CancelDownload(driver);
        cancelDownloadDialog.waitForVisible();
        cancelDownloadDialog.clickYes();
        logPassFail(largeEntGameTile.waitForDownloadable(), true);

        //9
        logPassFail(MacroGameLibrary.downloadFullEntitlement(driver, smallEntOffer), true);

        //10
        miniDownloadManager.clickDownloadCompleteCountToOpenFlyout();        
        downloadQueue.clearCompletedDownloads();
        logPassFail(!downloadQueue.isOpen(), true);

        softAssertAll();
    }
}
