package com.ea.originx.automation.scripts.feature.gamedownloader;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.store.BrowseGamesPage;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * A test which download and launch DiP Small in Origin client, or check if DiP
 * Small in Game Library in browser.
 */
public class OATestDownloadEntitlement extends EAXVxTestTemplate {

    @TestRail(caseId = 3087349)
    @Test(groups = {"gamedownloader_smoke", "client_only"})
    public void testClientDownloadEntitlement(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        OADipSmallGame entitlement = new OADipSmallGame();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);

        logFlowPoint("Login to Origin"); // 1
        logFlowPoint("Navigate to game library"); // 2
        logFlowPoint("Download Small DiP"); // 3
        logFlowPoint("Launch Small DiP"); // 4

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        // 2
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.waitForPageToLoad();
        Thread.sleep(2000);
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        Thread.sleep(5000);
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        if (gameTile.waitForDownloadable()) {
            logPass("Navigated to Game Library.");
        } else {
            logFailExit("Could not navigate to Game Library.");
        }

        // 3
        boolean downloaded = MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId);
        if (downloaded) {
            logPass("Download Small DiP in progress");
        } else {
            logFailExit("Download Small DiP unsuccessful");
        }

        // 4
        gameTile.play();
        if (entitlement.waitForGameLaunch(client)) {
            logPass("Launch Small DiP");
        } else {
            logFailExit("Launch Small DiP unsuccessful");
        }
        softAssertAll();
    }

    @Test(groups = {"browser", "full_regression", "download_smoke", "browser_only"})
    public void testBrowserDownloadEntitlement(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final int BROWSER_GAME_LIBRARY_LOADED_TIMEOUT = 10;

        OADipSmallGame entitlement = new OADipSmallGame();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);

        logFlowPoint("Login to Origin"); // 1
        logFlowPoint("Navigate to game library"); // 2
        logFlowPoint("Look for Small DiP in Game Library"); // 3

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        final BrowseGamesPage browseGamesPage = new BrowseGamesPage(driver);
        browseGamesPage.waitForPageToLoad();

        // 2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        final GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPass("Navigate to game library");

        // 3
        // the library is not loaded completely at this moment, add some sleep
        Thread.sleep(BROWSER_GAME_LIBRARY_LOADED_TIMEOUT * 1000L);
        final GameTile gameTile = new GameTile(driver, entitlementOfferId);
        if (gameTile.verifyOfferID()) {
            logPass("Small DiP found in game library");
        } else {
            logFailExit("Small DiP NOT found in game library");
        }
        softAssertAll();
    }

}
