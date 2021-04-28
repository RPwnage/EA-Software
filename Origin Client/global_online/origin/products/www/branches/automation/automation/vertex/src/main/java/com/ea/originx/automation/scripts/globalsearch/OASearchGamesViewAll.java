package com.ea.originx.automation.scripts.globalsearch;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.FilterMenu;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the game library related searches in the Global Search bar
 *
 * @author rmcneil
 */
public class OASearchGamesViewAll extends EAXVxTestTemplate {

    @TestRail(caseId = 9339)
    @Test(groups = {"globalsearch", "full_regression"})
    public void testSearchGamesViewAll(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final String firstGame = "Battlefield";
        final String secondGame = "Sim";
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIM_CITY_STANDARD);
        final String entitlementName = entitlement.getName();
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.TEN_EXTRA_ENTITLEMENTS);
        final String username = userAccount.getUsername();

        logFlowPoint("Log into Origin as " + username); //1
        logFlowPoint("Enter a game into the Global Search Bar and verify search results were displayed."); //2
        logFlowPoint("Verify tiles that appear in search results are above the store/people results."); //3
        logFlowPoint("Verify tiles show up in one line."); //4
        logFlowPoint("Verify search results are sorted alphabetically."); //5
        logFlowPoint("Verify that a hyperlink 'View all in game library' is present."); //6
        logFlowPoint("Verify clicking the 'View All' link takes the user to the 'Game Library'."); //7
        logFlowPoint("Search for another game and verify search results were displayed."); //8
        logFlowPoint("Hover over any game tile and verify the 'Details' CTA button appears."); //9
        logFlowPoint("Click on both the tile, then on the CTA button and verify the 'Owned Game Details' page is displayed."); //10

        //1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        GlobalSearch globalSearch = new GlobalSearch(driver);
        globalSearch.enterSearchText(firstGame);
        GlobalSearchResults searchResults = new GlobalSearchResults(driver);
        logPassFail(searchResults.verifySearchDisplayed(), true);

        //3
        sleep(4000); //waiting for page to stabilize
        logPassFail(searchResults.verifySearchResultsAboveStoreAndPeople(), true);
        
        //4
        logPassFail(searchResults.verifyGameLibrarySearchResultDisplayedOneLine(), true);
        
        //5
        logPassFail(searchResults.verifyGameLibraryResultsOrder(), true);

        //6
        logPassFail(searchResults.verifyViewAllGameLibraryLink(), true);
        
        //7
        searchResults.clickViewAllGameLibraryResults();
        GameLibrary gameLibrary = new GameLibrary(driver);
        gameLibrary.openFilterMenu();
        new MainMenu(driver).clickMaximizeButton();
        logPassFail(gameLibrary.verifyGameLibraryPageReached() && new FilterMenu(driver).verifyFilterTextMatchesGame(firstGame), true);

        //8
        new NavigationSidebar(driver).clickGameLibraryLink();
        globalSearch.enterSearchText(secondGame);
        logPassFail(searchResults.verifySearchDisplayed(), true);
        
        //9
        logPassFail(searchResults.hoverOnGameAndClickDetails(), true);
        
        //10
        searchResults.clickOnSecondTileDetailsButton();
        GameSlideout gameSlideout = new GameSlideout(driver);
        gameSlideout.waitForSlideout();
        logPassFail(gameSlideout.verifyGameTitle(entitlementName), true);

        softAssertAll();
    }
}