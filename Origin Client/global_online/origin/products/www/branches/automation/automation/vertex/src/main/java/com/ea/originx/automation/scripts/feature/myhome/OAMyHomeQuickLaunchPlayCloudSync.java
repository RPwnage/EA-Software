package com.ea.originx.automation.scripts.feature.myhome;

import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.discover.QuickLaunchTile;
import com.ea.originx.automation.lib.pageobjects.discover.RecentlyPlayedSection;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.TestCleaner;
import com.ea.vx.originclient.utils.Waits;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the syncing of a played game on Quick Launch from client to browser
 *
 * @author sbentley
 */
public class OAMyHomeQuickLaunchPlayCloudSync extends EAXVxTestTemplate {

    @TestRail(caseId = 1044267)
    @Test(groups = {"myhome", "client_only", "full_regression"})
    public void testMyHomeQuickLaunchPlayCloudSync(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final OADipSmallGame entitlement = new OADipSmallGame();
        final String entitlementOfferID = entitlement.getOfferId();
        final UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);

        //clear the local app data and uninstall
        new TestCleaner(client).clearLocalSettings(true);
        entitlement.enableSilentUninstall(client);

        logFlowPoint("Launch origin and login to a user who has entitled games");   //1
        logFlowPoint("Navigate to Game Library start and close a game");   //2
        logFlowPoint("Navigate to My Home and verify Recently Played section appears"); //3
        logFlowPoint("Verify recently closed game appears in Recently Played section"); //4
        logFlowPoint("Open Origin in a browser and log into Origin");    //5
        logFlowPoint("Navigate to My Home and verify Recently Played section appears"); //6
        logFlowPoint("Verify recently played game appears in Recently Played section"); //7

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with " + userAccount.getUsername() + " with entitlement: " + entitlement.getName());
        } else {
            logFailExit("Could not log into Origin with account: " + userAccount.getUsername());
        }

        //2
        NavigationSidebar navSideBar = new NavigationSidebar(driver);
        DiscoverPage myHome = new DiscoverPage(driver);
        myHome.verifyDiscoverPageReached();
        navSideBar.gotoGameLibrary().verifyGameLibraryPageReached();
        GameTile gameTile = new GameTile(driver, entitlementOfferID);
        Waits.pollingWaitEx(() -> MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferID));
        Waits.pollingWait(() -> gameTile.isReadyToPlay(), 60000, 1000, 10000);
        gameTile.play();
        Waits.pollingWaitEx(() -> entitlement.isLaunched(client));
        if (entitlement.killLaunchedProcess(client)) {
            logPass("Successfully navigated to Game Library started and closed game");
        } else {
            logFailExit("Could not start and close game");
        }

        //3
        navSideBar.gotoDiscoverPage();
        RecentlyPlayedSection recentlyPlayed = new RecentlyPlayedSection(driver);
        if (recentlyPlayed.verifyRecentlyPlayedAreaVisisble()) {
            logPass("Successfully located Quick Launch area");
        } else {
            logFailExit("Could not locate Quick Launch area");
        }

        //4
        QuickLaunchTile quickLaunchTile = new QuickLaunchTile(driver, entitlementOfferID);
        if (quickLaunchTile.verifyTileVisible()) {
            logPass("Successfully verified recently played game appears in Quick Launch area");
        } else {
            logFailExit("Failed to verify recently played game appears in Quick Launch area");
        }

        //5
        final WebDriver browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        browserDriver.get(driver.getCurrentUrl());
        if (MacroLogin.startLogin(browserDriver, userAccount)) {
            logPass("Successfully logged into Origin with account: " + userAccount.getUsername());
        } else {
            logFailExit("Could not log into Origin with account: " + userAccount.getUsername());
        }

        //6
        navSideBar = new NavigationSidebar(browserDriver);
        myHome = navSideBar.gotoDiscoverPage();
        recentlyPlayed = new RecentlyPlayedSection(browserDriver);
        if (recentlyPlayed.verifyRecentlyPlayedAreaVisisble()) {
            logPass("Successfully verified Recently Played section appears");
        } else {
            logFailExit("Could not veirfy Recently Played section appears");
        }

        //7
        quickLaunchTile = new QuickLaunchTile(browserDriver, entitlementOfferID);
        if (quickLaunchTile.verifyTileVisible()) {
            logPass("Successfully verified recently played games appears on other platform");
        } else {
            logFail("Could not verify recently played game appears on other platform");
        }

        softAssertAll();

    }
}
