package com.ea.originx.automation.scripts.globalsearch;

import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.FilterMenu;
import com.ea.originx.automation.lib.pageobjects.store.BrowseGamesPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the search results page for both user search and game search
 *
 * @author cvanichsarn
 */
public class OAGlobalSearchGamesUsers extends EAXVxTestTemplate {

    @TestRail(caseId = 1016684)
    @Test(groups = {"globalsearch", "release_smoke"})
    public void testGlobalSearchGamesUsers(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String entitlementName = entitlement.getName();
        final UserAccount user = AccountManager.getEntitledUserAccount(entitlement);
        final String userSearchString = "automation";
        final String gameSearchString = "Battlefield";

        logFlowPoint("Log into Origin with an account that owns " + entitlementName);   //1
        logFlowPoint("Type '" + userSearchString + "' into the global search box");   //2
        logFlowPoint("Verify only 4 users appear in the results page");   //3
        logFlowPoint("Verify clicking 'View All' expands the results to beyond 4 results");   //4
        logFlowPoint("Verify scrolling down the page will produce additional " + userSearchString + " related users");   //5
        logFlowPoint("Type '" + gameSearchString + "' into the global search box");   //6
        logFlowPoint("Verify the results page displays " + entitlementName + " in the 'Game Library' results");   //7
        logFlowPoint("Click 'View all in game library' and verify the user is taken to a filtered 'Game Library' based on the search text");   //8
        logFlowPoint("Type '" + gameSearchString + "' into the global search box");   //9
        logFlowPoint("Verify the results page displays " + entitlementName + " in the store results");   //10
        logFlowPoint("Click 'View all in store'");   //11
        logFlowPoint("Verify scrolling down the page will produce additional " + gameSearchString + " related products");   //12

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //2
        //Client window needs to be maximized for filter bubbles to appear in flow points
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        if (isClient) {
            new MainMenu(driver).clickMaximizeButton();
        }
        GlobalSearch globalSearch = new GlobalSearch(driver);
        globalSearch.enterSearchText(userSearchString);
        GlobalSearchResults searchResults = new GlobalSearchResults(driver);
        logPassFail(searchResults.verifySearchDisplayed(), true);

        //3
        logPassFail(searchResults.getNumberOfDisplayedPeopleResults() == 4, true);

        //4
        searchResults.clickViewAllPeopleResults();
        int numPeopleResults = searchResults.getNumberOfDisplayedPeopleResults();
        logPassFail(numPeopleResults >= 4, true);

        //5
        searchResults.scrollToBottom();
        searchResults.waitForMoreResultsToLoad();
        logPassFail(Waits.pollingWait(() -> searchResults.getNumberOfDisplayedPeopleResults()>numPeopleResults, 30000, 5000, 0), true);

        //6
        globalSearch.enterSearchText(gameSearchString);
        logPassFail(searchResults.verifySearchDisplayed(), true);

        //7
        logPassFail(searchResults.verifyGameLibraryContainsOffer(entitlementName), true);

        //8
        searchResults.clickViewAllGameLibraryResults();
        logPassFail(new FilterMenu(driver).isFilterPillFilterByTextVisible(gameSearchString), true);

        //9
        globalSearch.enterSearchText(gameSearchString);
        logPassFail(searchResults.verifySearchDisplayed(), true);

        //10
        logPassFail(searchResults.verifyStoreResults(gameSearchString), true);

        //11
        searchResults.clickViewAllStoreResults();
        BrowseGamesPage browseGamesPage = new BrowseGamesPage(driver);
        logPassFail(browseGamesPage.verifyBrowseGamesPageReached() && browseGamesPage.verifyTextInPageTitle(gameSearchString), true);

        //12
        int numStoreTiles = browseGamesPage.getLoadedStoreGameTiles().size();
        browseGamesPage.scrollToBottom();
        browseGamesPage.waitForMoreResultsToLoad();
        int numStoreTilesAfterScrolling = browseGamesPage.getLoadedStoreGameTiles().size();
        logPassFail(numStoreTiles < numStoreTilesAfterScrolling, true);

        softAssertAll();
    }
}