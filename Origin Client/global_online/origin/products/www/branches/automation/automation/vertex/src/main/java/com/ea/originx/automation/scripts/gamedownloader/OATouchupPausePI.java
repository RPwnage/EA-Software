package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.OriginClientLogReader;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test that client log file has specific lines for a PI entitlement
 *
 * @author cdeaconu
 */
public class OATouchupPausePI extends EAXVxTestTemplate{
    
    @TestRail(caseId = 9639)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testDownloadPI(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        EntitlementInfo gamePI = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.PI);
        final String gamePIName = gamePI.getName();
        final String offerIdPI = gamePI.getOfferId();
        
        UserAccount userAccount = AccountManager.getEntitledUserAccount(gamePI);
        EACoreHelper.changeToBaseEntitlementPIOverride(client.getEACore());
        
        final String activatingNonRequiredDynamicChunks = "[Dynamic Download] Activating non-required dynamic chunks.";
        final String failedToEnableNonRequiredChunks = "[Dynamic Download] Failed to enable non-required chunks due to one or more required chunks not being complete.";
        /**
         * 'ProcessWin: ' text is incremented in order to find the lines
         * that have an 'Exit result: (code)' text to the right
         */
        final String processWin = "    ProcessWin: ";
        
        logFlowPoint("Log into Origin"); // 1
        logFlowPoint("Navigate to the 'Game Library' page"); // 2
        logFlowPoint("Begin downloading of " + gamePIName); // 3
        logFlowPoint("Verify overlay for " + gamePIName + " indicates it is playable before reaching 100%"); // 4
        logFlowPoint("Wait until game become playable"); // 5
        logFlowPoint("Open the Client Log and verify '" + activatingNonRequiredDynamicChunks + "' line can be found"); // 6
        logFlowPoint("Verify that line 'ProcessWin: (code) Exit result : (number)' is there before '" + activatingNonRequiredDynamicChunks + "' line"); // 7
        logFlowPoint("Verify '" + failedToEnableNonRequiredChunks + "' line is NOT found"); // 8
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();
        GameTile gameTilePI = new GameTile(driver, offerIdPI);
        logPassFail(gameTilePI.waitForDownloadable(), true);

        // 3
        boolean isEntitementDownloading = MacroGameLibrary.startDownloadingEntitlement(driver, offerIdPI);
        logPassFail(isEntitementDownloading, true);
        
        // 4
        GameTile gameTile = new GameTile(driver, offerIdPI);
        logPassFail(gameTile.verifyOverlayPlayableStatusPercent(), false);

        // 5
        // Will wait until installation phase has finished 
        // and game become playable
        boolean isGameReadyToPlay = Waits.pollingWait(() -> gameTile.isDownloading() && gameTile.isReadyToPlay(), 180000, 1000, 0);
        logPassFail(isGameReadyToPlay, true);
         
        // 6 
        final OriginClientLogReader logReader = new OriginClientLogReader(OriginClientData.LOG_PATH);
        boolean isDynamicChunksTextPresent = !logReader.retriveLogLine(client, OriginClientData.CLIENT_LOG, activatingNonRequiredDynamicChunks).equals("");
        logPassFail(isDynamicChunksTextPresent, false);
        
        // 7
        boolean isLineCorrectlyPositioned = logReader.verifyLineAIsThereBeforeLineB(client, OriginClientData.CLIENT_LOG, processWin, activatingNonRequiredDynamicChunks);
        logPassFail(isLineCorrectlyPositioned, false);
        
        // 8
        boolean isFailedToEnableChunksTextNotFound = logReader.retriveLogLine(client, OriginClientData.CLIENT_LOG, failedToEnableNonRequiredChunks).equals("");
        logPassFail(isFailedToEnableChunksTextNotFound, false);
        
        softAssertAll();
    }
}
