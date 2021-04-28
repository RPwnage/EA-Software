package com.ea.originx.automation.scripts.feature.myhome;

import com.ea.originx.automation.lib.macroaction.MacroDiscover;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.discover.QuickLaunchTile;
import com.ea.originx.automation.lib.pageobjects.discover.RecentlyPlayedSection;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.TestCleaner;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * This tests the Quick Launch tile launching a game on My Home
 *
 * @author sbentley
 */
public class OAMyHomeQuickLaunch extends EAXVxTestTemplate {

    OriginClient client = null;
    EntitlementInfo entitlement = null;

    @TestRail(caseId = 963049)
    @Test(groups = {"myhome", "client_only", "full_regression"})
    public void testMyHomeQuickLaunch(ITestContext context) throws Exception {

        client = OriginClientFactory.create(context);
        entitlement = new OADipSmallGame();
        final String entitlementOfferID = entitlement.getOfferId();
        final UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);

        //clear the local app data and uninstall
        new TestCleaner(client).clearLocalSettings(true);
        entitlement.enableSilentUninstall(client);

        logFlowPoint("Log into Origin Client with an account that has games");  //1
        logFlowPoint("Navigate to Game Library");   //2
        logFlowPoint("Start a game (install if need to)");  //3
        logFlowPoint("Close game"); //4
        logFlowPoint("Navigate to My Home and verify Recently Played section appears"); //5
        logFlowPoint("Verify last game closed appears in Recently Played section"); //6
        logFlowPoint("Verify Quick Launch tile has game title");    //7
        logFlowPoint("Verify Quick Launch tile has Play above game title"); //8
        logFlowPoint("Verify Quick Launch tile displays achievements"); //9
        logFlowPoint("Hover over tile and verify animation expands tile");  //10
        logFlowPoint("Click on Quick Launch tile and verify game is launched"); //11

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with account: " + userAccount.getUsername());
        } else {
            logFailExit("Could not log into Origin with account: " + userAccount.getUsername());
        }

        //2
        NavigationSidebar navSideBar = new NavigationSidebar(driver);
        //If you immediately go to game library from home on sign in, game tiles won't load
        DiscoverPage myHome = new DiscoverPage(driver);
        myHome.verifyDiscoverPageReached();
        GameLibrary gameLibrary = navSideBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigate to Game Library");
        } else {
            logFailExit("Could not navigate to Game Library");
        }

        //3
        GameTile gameTile = new GameTile(driver, entitlementOfferID);
        GameSlideout ogd = gameTile.openGameSlideout();
        Waits.pollingWait(() -> ogd.waitForSlideout());
        boolean gameHasAchievements = ogd.verifyAchievementsNavLinkVisible();
        ogd.clickSlideoutCloseButton();

        MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferID);
        gameTile.waitForReadyToPlay(180); //Wait for game to be installed
        gameTile.play();
        MacroGameLibrary.handlePlayDialogs(driver);

        if (entitlement.isLaunched(client)) {
            logPass("Successfully launched: " + entitlement.getName());
        } else {
            logFailExit("Could not launch: " + entitlement.getName());
        }

        //4
        entitlement.killLaunchedProcess(client);
        if (!entitlement.isLaunched(client)) {
            logPass("Successfully closed game");
        } else {
            logFailExit("Could not close game");
        }

        //5
        navSideBar.gotoDiscoverPage();
        RecentlyPlayedSection recentlyPlayed = new RecentlyPlayedSection(driver);
        if (recentlyPlayed.verifyRecentlyPlayedAreaVisisble()) {
            logPass("Successfully located Quick Launch area");
        } else {
            logFailExit("Could not locate Quick Launch area");
        }

        //6
        QuickLaunchTile quickLaunchTile = new QuickLaunchTile(driver, entitlementOfferID);
        if (quickLaunchTile.verifyTileVisible()) {
            logPass("Successfully verified recently played game appears in Quick Launch area");
        } else {
            logFailExit("Failed to verify recently played game appears in Quick Launch area");
        }

        //7
        if (quickLaunchTile.verifyQuickLaunchTitle(entitlement)) {
            logPass("Successfully verified Quick Launch tile's title");
        } else {
            logFail("Failed to verify Quick Launch tile's title");
        }

        //8
        //String appears in all caps on tile, but text can be mixed case so to lower comparison is needed
        if (quickLaunchTile.verifyEyebrowStatePlayVisible()) {
            logPass("Successfully verified Quick Launch tile in Play state");
        } else {
            logFail("Failed to verify Quick Launch tile in Play state");
        }

        //9
        String passAchievementString = gameHasAchievements
                ? "Successfully verified game has achievements"
                : "Tested game does not have achievements";

        String failAchievementString = gameHasAchievements
                ? "Could not find achievements on game tile"
                : "Achievements displayed on game tile for game without achievements";

        //We want achievements list to show for games that do, but don't for games that don't have achievements
        boolean verifyAchievements = gameHasAchievements
                ? quickLaunchTile.verifyAchievementsVisible()
                : !quickLaunchTile.verifyAchievementsVisible();

        if (verifyAchievements) {
            logPass(passAchievementString);
        } else {
            logFail(failAchievementString);
        }

        //10
        if (quickLaunchTile.verifyExpandHoverAnimation()) {
            logPass("Successfully verified animation on hover");
        } else {
            logFail("Failed to verify animation on hover");
        }

        //11
        if (MacroDiscover.verifyQuickLaunchGameLaunchEntitlement(client, driver, entitlement)) {
            logPass("Successfully launched game via Quick Launch tile");
        } else {
            logFail("Could not launch game via Quick Launch");
        }

        softAssertAll();
    }
}
