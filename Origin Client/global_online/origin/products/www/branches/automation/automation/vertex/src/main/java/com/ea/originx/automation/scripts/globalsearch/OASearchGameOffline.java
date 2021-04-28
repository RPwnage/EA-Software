package com.ea.originx.automation.scripts.globalsearch;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoConstants;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests Global search of Game Library while offline
 *
 * @author mkalaivanan
 */
public class OASearchGameOffline extends EAXVxTestTemplate {

    @TestRail(caseId = 9340)
    @Test(groups = {"globalsearch", "client_only", "full_regression"})
    public void testSearchGameOffline(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final String nonDipLarge = EntitlementInfoConstants.NON_DIP_LARGE_NAME;
        final String dipSmall = EntitlementInfoConstants.DIP_SMALL_NAME;
        final String dragonAge = "Dragon Age";

        AccountManager accountManager = AccountManager.getInstance();
        Criteria criteria = new Criteria.CriteriaBuilder().build();
        criteria.addEntitlement(dipSmall);
        criteria.addEntitlement(nonDipLarge);
        UserAccount userAccount = accountManager.requestWithCriteria(criteria, 360);

        logFlowPoint("Launch Origin and login."); //1
        logFlowPoint("Go offline through the Origin Menu."); //2
        logFlowPoint("Navigate to Game Library."); //3
        logFlowPoint("Enter Search Query:" + nonDipLarge); //4
        logFlowPoint("Verify that Game Library Results have the Search Text:" + nonDipLarge + " in them."); //5
        logFlowPoint("Enter Search Query for " + dragonAge + " that is not present in the game library and Verify 'No Results Found' page is displayed, but the image is not."); //6

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin as " + userAccount.getUsername());
        } else {
            logFailExit("Could not log into Origin as " + userAccount.getUsername());
        }

        //2
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectGoOffline();
        MacroSocial.restoreAndExpandFriends(driver);
        if (new SocialHub(driver).verifyUserOffline()) {
            logPass("Switch to offline mode successful.");
        } else {
            logFailExit("Could not switch to offline mode.");
        }

        //3
        new SocialHub(driver).minimizeSocialHub();
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Navigated to Game Library.");
        } else {
            logFailExit("Could not navigate to Game Library.");
        }

        //4
        GlobalSearch globalSearch = new GlobalSearch(driver);
        globalSearch.enterSearchText(dipSmall);
        GlobalSearchResults searchresults = new GlobalSearchResults(driver);
        if (searchresults.verifySearchDisplayed()) {
            logPass(nonDipLarge + " triggered a search.");
        } else {
            logFail(nonDipLarge + " did not trigger a search.");
        }

        //5
        if (searchresults.verifyGameLibraryResults(dipSmall)) {
            logPass("Relevant game library search results appeared.");
        } else {
            logFail("Game library search results were not valid.");
        }

        //6
        globalSearch.enterSearchText(dragonAge);
        boolean noResultsDisplayed = searchresults.verifyNoResultsDisplayed();
        boolean noImageDisplayed = !searchresults.storeResultTileVisible(dragonAge);
        if (noImageDisplayed && noResultsDisplayed) {
            logPass("Search for " + dragonAge + " resulted in 'No Results found' page and no image displayed.");
        } else {
            logFailExit("Search for " + dragonAge + " displayed search results or the image.");
        }
        softAssertAll();
    }
}