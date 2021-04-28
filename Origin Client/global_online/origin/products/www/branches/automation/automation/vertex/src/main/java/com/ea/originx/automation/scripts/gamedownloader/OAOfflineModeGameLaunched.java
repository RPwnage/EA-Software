package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.OfflineFlyout;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to verify game launch not interrupted by going offline then online
 *
 * @author palui
 */
public class OAOfflineModeGameLaunched extends EAXVxTestTemplate {

    @TestRail(caseId = 39452)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testOfflineModeGameLaunched(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String username = userAccount.getUsername();

        logFlowPoint("Launch Origin and login as user: " + username); //1
        logFlowPoint(String.format("Navigate to game library and locate '%s' game tile", entitlementName)); //2
        logFlowPoint(String.format("Download '%s' game", entitlementName)); //3
        logFlowPoint(String.format("Launch '%s' game", entitlementName)); //4
        logFlowPoint("Click 'Go Offline' at the Origin menu and verify client is offline"); //5
        logFlowPoint("Verify client displays a message stating it is offline"); //6
        logFlowPoint(String.format("Verify '%s' game is not interrupted by going offline", entitlementName)); //7
        logFlowPoint("Click 'Go Online' at the Origin menu and verify client is online"); //8
        logFlowPoint(String.format("Verify '%s' game is not interrupted by going online", entitlementName)); //9

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        logPassFail(gameTile.waitForDownloadable(), true);

        //3
        logPassFail(MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId), true);

        //4
        gameTile.play();
        logPassFail(Waits.pollingWaitEx(() -> entitlement.isLaunched(client)), true);

        //5
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectGoOffline();
        sleep(1000); // wait for Origin menu to update
        logPassFail(mainMenu.verifyItemEnabledInOrigin("Go Online"), true);

        //6
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60); // Go to SPA page - required after going offline
        new OfflineFlyout(driver).closeOfflineFlyout(); // close 'Offline flyout' which may interfere with game tiles operations
        DiscoverPage discoverPage = new NavigationSidebar(driver).gotoDiscoverPage();
        discoverPage.waitForPageToLoad();
        logPassFail(discoverPage.verifyOfflineMessageExists(), true);

        //7
        logPassFail(entitlement.isLaunched(client), true);

        //8
        mainMenu.selectGoOnline();
        sleep(1000); // wait for Origin menu to update
        boolean mainMenuOnline = mainMenu.verifyItemEnabledInOrigin("Go Offline");
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60); // Go to SPA page - required after going online
        discoverPage.waitForPageToLoad();
        boolean discoverPageOnline = discoverPage.isPageLoaded();
        logPassFail(mainMenuOnline && discoverPageOnline, true);

        //9
        logPassFail(entitlement.isLaunched(client), true);

        softAssertAll();
    }
}
