package com.ea.originx.automation.scripts.tools;

import com.ea.originx.automation.lib.macroaction.MacroDiscover;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.discover.QuickLaunchTile;
import com.ea.originx.automation.lib.pageobjects.discover.RecentlyPlayedSection;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.GameInfo;
import com.ea.originx.automation.lib.resources.games.GameInfoDepot;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.LocalXMLHelper;
import com.ea.vx.originclient.net.helpers.CrsHelper;
import com.ea.vx.originclient.utils.EntitlementService;
import com.ea.vx.originclient.utils.FileUtil;
import com.ea.vx.originclient.utils.TestCleaner;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests offers in GameInfo csv for appearance in Quick Launch as well as
 * launching the associated game.
 *
 * @author sbentley
 */
public class OAQuickLaunchGame extends EAXVxTestTemplate {

    @Test(groups = {"client_only"})
    public void testQuickLaunchGame(ITestContext context) throws Exception {
        // Get the report id and the current test-result-id to get the index number to load the corresponding PDP
        String reportID = CrsHelper.getReportId();
        String testCaseName = "com.ea.originx.automation.scripts.tools.OAQuickLaunchGame";

        // Get the TestCase Id from the testcase Name
        int testCaseID = CrsHelper.getTestCaseId(testCaseName);
        int firstTestResult = CrsHelper.getTestResultId(reportID, testCaseID, true);
        int currentTestResultId = Integer.parseInt(resultLogger.getCurrentTestResultId());
        int index = currentTestResultId - firstTestResult;
        GameInfo game = GameInfoDepot.getGameInfo(index);

        final OriginClient client = OriginClientFactory.create(context);

        //Since non-default locations are used, need to explicitly delete locations
        FileUtil.deleteFolder(client, "E:\\Quick Launch Test Games\\");
        FileUtil.deleteFolder(client, "E:\\Quick Launch Test Download Cache\\");

        //clear the local app data and uninstall
        new TestCleaner(client).clearLocalSettings(true);

        final String offerId = game.getOfferId();
        final String offerName = game.getName();
        final String processName = game.getProcessName();
        final String type = game.getType();
        final boolean isAutomatable = (game.getAutomatableInstall().equals("Automatable") && game.getAutomatablePlay().equals("Automatable"));

        final UserAccount userAccount = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();

        if (!type.contains("base")
                || !processName.contains(".exe")
                || !isAutomatable) {
            logFlowPoint("Tested offer is not an automatable game: " + offerName);
            logPass("Offer is not an automatable game");
            return;
        }

        logFlowPoint("Log into an account " + userAccount.getUsername() + " with the entitled game: " + offerName); //1
        logFlowPoint("Download the game to make it appear in Quick Launch");    //2
        logFlowPoint("Go to My Home and verify game appears in Quick Launch");  //3
        logFlowPoint("Verify correct title is displayed on Quick Launch tile"); //4
        logFlowPoint("Verify eyebrow state is in Play state");  //5
        logFlowPoint("Verify achievements are displayed on Quick Launch tile (depending on the offer)");    //6
        logFlowPoint("Verify tile expands when hovered over");  //7
        logFlowPoint("Verify game is launch when clicking on Quick Launch tile");   //8

        WebDriver driver = startClientObject(context, client);

        sleep(5000); //wait for AppData\Roaming\Origin folder to be created
        LocalXMLHelper.AddSettingElementToLocalXML(client, "Setting", "E:\\Quick Launch Test Games\\", "10", "DownloadInPlaceDir");
        LocalXMLHelper.AddSettingElementToLocalXML(client, "Setting", "E:\\Quick Launch Test Download Cache\\cache", "10", "CacheDir");

        MacroLogin.startLogin(driver, userAccount);
        EntitlementService.grantEntitlement(userAccount, offerId);

        NavigationSidebar navSideBar = new NavigationSidebar(driver);
        GameLibrary gameLibary = navSideBar.gotoGameLibrary();

        //1
        if (gameLibary.verifyGameLibraryHasExpectedGames(offerId)) {
            logPass("Successfully logged into an account with entitlement: " + offerName);
        } else {
            logFailExit("Account did not have entitlement: " + offerName);
        }

        //Check game for achievements
        GameTile gameTile = new GameTile(driver, offerId);
        GameSlideout ogd = gameTile.openGameSlideout();
        Waits.pollingWait(() -> ogd.waitForSlideout());
        boolean gameHasAchievements = ogd.verifyAchievementsNavLinkVisible();
        ogd.clickSlideoutCloseButton();

        //2
        if (MacroGameLibrary.downloadFullEntitlement(driver, offerId)) {
            logPass("Successfully downloaded game");
        } else {
            logFailExit("Failed to download game");
        }

        //3
        navSideBar.gotoDiscoverPage();
        driver.navigate().refresh(); //Refresh to show Quick Launch
        RecentlyPlayedSection recentlyPlayedSection = new RecentlyPlayedSection(driver);
        QuickLaunchTile quickLaunchTile = new QuickLaunchTile(driver, offerId);
        if (recentlyPlayedSection.verifyRecentlyPlayedAreaVisisble() && quickLaunchTile.verifyTileVisible()) {
            logPass("Successfully verified game appears in Quick Launch when installed");
        } else {
            logFailExit("Failed to verify game appears in Quick Launch when installed");
        }

        //4
        String tileTitle = quickLaunchTile.getQuickLaunchTitle();
        if (tileTitle.equals(offerName)) {
            logPass("Successfully verified Quick Launch tile's title");
        } else {
            logFail(String.format("Failed to verify Quick Launch %s's title: %s", offerName, tileTitle));
        }

        //5
        if (quickLaunchTile.verifyEyebrowStatePlayVisible()) {
            logPass("Successfully verified Quick Launch tile in Play state");
        } else {
            String tileState = quickLaunchTile.getEyebrowStateVisible();
            logFail("Failed to verify Quick Launch tile in Play state. Tile in state: " + tileState);
        }

        //6
        //We want achievements list to show for games that do, but don't for games that don't have achievements
        boolean quickLaunchTileAchievements = quickLaunchTile.verifyAchievementsVisible();
        boolean verifyAchievements = gameHasAchievements
                ? quickLaunchTileAchievements
                : !quickLaunchTileAchievements;

        if (verifyAchievements) {
            logPass(String.format("Game has achievements: %s, Quick Launch tile has achievements: %s", gameHasAchievements, quickLaunchTileAchievements));
        } else {
            logFail(String.format("Game has achievements: %s, Quick Launch tile has achievements: %s", gameHasAchievements, quickLaunchTileAchievements));
        }

        //7
        if (quickLaunchTile.verifyExpandHoverAnimation()) {
            logPass("Successfully verified animation on hover");
        } else {
            logFail("Failed to verify animation on hover");
        }

        //8
        if (MacroDiscover.verifyQuickLaunchGameLaunchEntitlement(client, driver, offerName, offerId, processName)) {
            logPass("Successfully launched game via Quick Launch tile");
        } else {
            logFail("Could not launch game via Quick Launch");
        }

    }
}
