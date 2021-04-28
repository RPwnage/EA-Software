package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueue;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
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
 * Test behavior of downloading, launching and exit a PI entitlement
 *
 * @author cdeaconu
 */
public class OAGameDetailsPI extends EAXVxTestTemplate{
    
    @TestRail(caseId = 9606)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testDownloadPI(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        EntitlementInfo gamePI = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.PI);
        final String gamePIName = gamePI.getName();
        final String offerIdPI = gamePI.getOfferId();
        
        UserAccount userAccount = AccountManager.getEntitledUserAccount(gamePI);
        
        EACoreHelper.changeToBaseEntitlementPIOverride(client.getEACore());
        
        logFlowPoint("Log into Origin with User A"); // 1
        logFlowPoint("Navigate to 'Game Library' and verify game " + gamePIName + " appears as downloadable"); // 2
        logFlowPoint("Begin downloading of " + gamePIName); // 3
        logFlowPoint("Verify overlay for " + gamePIName + " indicates it is playable before reaching 100%"); // 4
        logFlowPoint("Verify that there is a disabled 'Play' icon button to the right of the progress bar."); // 5
        logFlowPoint("Wait until game becomes playable"); // 6
        logFlowPoint("Verify that the progress bar turns green"); // 7
        logFlowPoint("Verify that the 'Playable' indicator ungreys."); // 8
        logFlowPoint("Verify that the user is able to launch game by clicking the 'Play' button"); // 9
        logFlowPoint("Verify the Progress Bar remains colored green"); // 10
        logFlowPoint("Exit the game"); // 11
        logFlowPoint("Verify that the download returns back to Green and the Play button returns to being active by exiting the game."); // 12
        logFlowPoint("Wait until download is complete"); // 13
        logFlowPoint("Verify that the progress bar is removed and replaced with a 'Play' button by completing the download."); // 14
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();
        GameTile gameTilePI = new GameTile(driver, offerIdPI);
        logPassFail(gameTilePI.waitForDownloadable(), true);
        
        // 3
        // Change download rate to 10MB in order to be able to check all the steps before completing the download.
        TestScriptHelper.setMaxDownloadRateBpsOutOfGame(driver, "10000000");
        boolean startedDownloading = MacroGameLibrary.startDownloadingEntitlement(driver, offerIdPI);
        logPassFail(startedDownloading, true);
        
        // 4
        GameTile gameTile = new GameTile(driver, offerIdPI);
        logPassFail(gameTile.verifyOverlayPlayableStatusPercent(), true);
        
        // 5
        DownloadManager miniDownloadManager = new DownloadManager(driver);
        miniDownloadManager.clickRightArrowToOpenFlyout();     
        DownloadQueueTile downloadQueueTile = new DownloadQueueTile(driver, offerIdPI);
        boolean isDownloadQueueOpen = new DownloadQueue(driver).isOpen();
        boolean isPlayButtonDisabled = downloadQueueTile.isPlayButtonDisabled();
        logPassFail(isDownloadQueueOpen && isPlayButtonDisabled, true);
        
        // 6
        // Will wait until installation phase has finished 
        // and game become playable
        boolean isGameReadyToPlay = Waits.pollingWait(() -> gameTile.isDownloading() && gameTile.isReadyToPlay(), 180000, 1000, 0);
        logPassFail(isGameReadyToPlay, true);
        
        // 7
        logPassFail(downloadQueueTile.isProgressBarGreen(), false);
        
        // 8
        logPassFail(downloadQueueTile.isPlayableIndicatorGreen(), false);
        
        // 9
        downloadQueueTile.clickPlayButton();
        boolean isPiGameLaunched = gamePI.isLaunched(client);
        logPassFail(isPiGameLaunched, true);
        
        // 10
        logPassFail(downloadQueueTile.isProgressBarGreen(), false);
        
        // 11
        gamePI.killLaunchedProcess(client);
        boolean isPiGameClosed = gamePI.isNotLaunched(client);
        logPassFail(isPiGameClosed, true);
        
        // 12
        logPassFail(downloadQueueTile.isPlayButtonActive() && downloadQueueTile.isProgressBarGreen(), false);
        
        // 13  
        // Change download rate back to "No limit".
        TestScriptHelper.setMaxDownloadRateBpsOutOfGame(driver, "0");
        boolean isDownloadCompleted =   MacroGameLibrary.finishDownloadingEntitlement(driver, offerIdPI);
        logPassFail(isDownloadCompleted, true);
        
        // 14
        logPassFail(downloadQueueTile.isPlayButtonActive() && !downloadQueueTile.isProgressBarVisible(), false);
        
        softAssertAll();
    }
}