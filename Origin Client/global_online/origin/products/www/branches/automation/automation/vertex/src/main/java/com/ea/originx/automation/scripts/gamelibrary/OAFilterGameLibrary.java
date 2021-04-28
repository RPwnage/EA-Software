package com.ea.originx.automation.scripts.gamelibrary;


import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOfflineMode;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.FilterMenu;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the game library filter menu functionality across sessions
 *
 * @author tdhillon
 */

public class OAFilterGameLibrary extends EAXVxTestTemplate {

    @TestRail(caseId = 11420)
    @Test(groups = {"gamelibrary", "client_only", "full_regression"})
    public void testFilterGameLibrary(ITestContext context) throws Exception {
        final EntitlementInfo nonSmallDip = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.NON_DIP_SMALL);
        final EntitlementInfo smallDip = new OADipSmallGame();

        final OriginClient client = OriginClientFactory.create(context);

        String smallDipName = smallDip.getName();
        String nonSmallDipName = nonSmallDip.getName();

        UserAccount user = AccountManager.getEntitledUserAccount(smallDip, nonSmallDip);
        final String username = user.getUsername();

        logFlowPoint("Log into Origin with " + username + "and maximize with client window"); //1
        logFlowPoint("Disable Automatic game updates, Navigate to Game Library and install " + smallDipName ); //2
        logFlowPoint("Open the filter menu"); //3
        logFlowPoint("Verify that when online, the user can filter the results by status (Played this week, Favorites, Downloading, Hidden Games, Installed Games, Non-Origin Games, Origin Access Games, Purchased Games)."); //4
        logFlowPoint("Click 'Clear All' and verify all applied filter bubbles are cleared and verify all the games are visible"); //5
        logFlowPoint("Select a pre-made filters: 'Platform-Mac', and verify that a new bubbles appear at the top of the page for the selected pre-made filter"); //6
        logFlowPoint("Logout of the Origin client"); //7
        logFlowPoint("Login again with same user account and verify user lands on Discover page"); //8
        logFlowPoint("Navigate to Game Library" ); //9
        logFlowPoint("Open the filter menu"); //10
        logFlowPoint("Verify 'Platform-Mac' persists across new session"); //11
        logFlowPoint("Enter filter text: " + smallDipName + " and verify a new bubble appears at the top of the page: 'Filter By: " + smallDipName + "'"); //12
        logFlowPoint("Verify that any filter options that apply to no games in the library are disabled. (Ex: The Favorites filter if no games have been marked as favorites)"); //13
        logFlowPoint("Go offline"); //14
        logFlowPoint("Verify that when offline, user can filter the results by status (Installed Games, Non-Origin Games, Origin Access Games)."); //15

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //2
        //Client window needs to be maximized for filter bubbles to appear in flow points
        new MainMenu(driver).clickMaximizeButton();
        AppSettings appSettings = new MainMenu(driver).selectApplicationSettings();
        appSettings.setKeepGamesUpToDate(false); //To stop automatic download of PDLC content for smallDip

        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        MacroGameLibrary.downloadFullEntitlement(driver, smallDip.getOfferId(), new DownloadOptions().setUncheckExtraContent(true));
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //3
        FilterMenu filterMenu = gameLibrary.openFilterMenu();
        logPassFail(filterMenu.isFilterMenuOpen(), true);

        //4
        filterMenu.setPurchasedGames();
        filterMenu.setInstalled();
        boolean isPurchasedFilterPillVisibleOnline = filterMenu.verifyPurchasedFilterPillVisible();
        boolean isInstalledFilterPillVisibleOnline = filterMenu.verifyInstalledFilterPillVisible();
        boolean isGameTileVisibleByNameOnline = gameLibrary.isGameTileVisibleByName(smallDipName);
        boolean isPremadeFilterActiveOnline = isPurchasedFilterPillVisibleOnline && isInstalledFilterPillVisibleOnline
                                                && isGameTileVisibleByNameOnline;
        logPassFail(isPremadeFilterActiveOnline, false);

        //5
        filterMenu.clearAllFilters();
        logPassFail(filterMenu.verifyNoActiveFilters(), false);

        //6
        filterMenu.setMacGame();
        boolean isMacFilterPillVisible1 = filterMenu.verifyMacFilterPillVisible();
        boolean isSmallDipGameTileHidden1 = !gameLibrary.isGameTileVisibleByName(smallDipName);
        boolean isNonSmallDipGameTileVisible1 = gameLibrary.isGameTileVisibleByName(nonSmallDipName);
        boolean isMacFilterWorking = isMacFilterPillVisible1 && isSmallDipGameTileHidden1 && isNonSmallDipGameTileVisible1;
        logPassFail(isMacFilterWorking, false);

        //7
        LoginPage loginPage = new MainMenu(driver).selectLogOut();
        logPassFail(loginPage.isOnLoginPage(), true);

        //8
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //9
        navBar = new NavigationSidebar(driver);
        gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //10
        filterMenu = gameLibrary.openFilterMenu();
        logPassFail(filterMenu.isFilterMenuOpen(), true);

        //11
        boolean isMacFilterPillVisible2 = filterMenu.verifyMacFilterPillVisible();
        boolean isSmallDipGameTileHidden2 = !gameLibrary.isGameTileVisibleByName(smallDipName);
        boolean isNonSmallDipGameTileVisible2 = gameLibrary.isGameTileVisibleByName(nonSmallDipName);
        boolean isMacFilterActiveAcrossSessions = isMacFilterPillVisible2 && isSmallDipGameTileHidden2 && isNonSmallDipGameTileVisible2;
        logPassFail(isMacFilterActiveAcrossSessions, false);
        filterMenu.clearAllFilters();

        //12
        filterMenu.enterFilterText(smallDipName);
        logPassFail(gameLibrary.isGameTileVisibleByName(smallDipName) && filterMenu.isFilterPillFilterByTextVisible(smallDipName), false);
        filterMenu.clearAllFilters();

        //13
        gameLibrary.clearAllFavoritesIfExists();
        logPassFail(filterMenu.verifyFavoriteFilterDisabled(), false);

        //14
        MacroOfflineMode.goOffline(driver);
        logPassFail(MacroOfflineMode.isOffline(driver), true);

        //15
        filterMenu.setPurchasedGames();
        filterMenu.setInstalled();
        boolean isPurchasedFilterPillVisibleOffline = filterMenu.verifyPurchasedFilterPillVisible();
        boolean isInstalledFilterPillVisibleOffline = filterMenu.verifyInstalledFilterPillVisible();
        boolean isGameTileVisibleByNameOffline = gameLibrary.isGameTileVisibleByName(smallDipName);
        boolean isPremadeFilterActiveOffline = isPurchasedFilterPillVisibleOffline && isInstalledFilterPillVisibleOffline
                && isGameTileVisibleByNameOffline;
        logPassFail(isPremadeFilterActiveOffline, false);

        softAssertAll();
    }
}
