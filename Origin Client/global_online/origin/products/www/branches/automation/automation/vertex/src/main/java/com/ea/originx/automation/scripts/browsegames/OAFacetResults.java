package com.ea.originx.automation.scripts.browsegames;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.store.BrowseGamesPage;
import com.ea.originx.automation.lib.pageobjects.store.StoreFacet;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that the store's sorting facets work
 *
 * @author jmittertreiner
 */
public class OAFacetResults extends EAXVxTestTemplate {

    @TestRail(caseId = 2998171)
    @Test(groups = {"browsegames", "full_regression"})
    public void testFacetResults(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getRandomAccount();
        String searchTerm = "Battlefront";
        int numTiles = 0;

        logFlowPoint("Login as user " + userAccount.getUsername() + " and navigate to browse games"); // 1
        logFlowPoint("Search for " + searchTerm + " and view the store results"); // 2
        logFlowPoint("Click on 'Alphabetical - A to Z' and Verify the products are sorted"); // 3
        logFlowPoint("Verify the number of products remains the same"); // 4
        logFlowPoint("Click on 'Alphabetical - Z to A' and Verify the products are sorted"); // 5
        logFlowPoint("Verify the number of products remains the same"); // 6

        // 1
        WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, userAccount);
        BrowseGamesPage browseGamesPage = new NavigationSidebar(driver).gotoBrowseGamesPage();
        logPassFail(browseGamesPage.verifyBrowseGamesPageReached(), true);

        // 2
        new GlobalSearch(driver).enterSearchText(searchTerm);
        new GlobalSearchResults(driver).clickViewAllStoreResults();
        logPassFail(browseGamesPage.verifyBrowseGamesPageReached(), true);

        // 3
        StoreFacet storeFacet = new StoreFacet(driver);
        storeFacet.clickOption("Sort", "Alphabetical - A to Z");
        numTiles = browseGamesPage.getNumberOfGameTilesLoaded();
        logPassFail(browseGamesPage.verifyTilesSortedByName(), true);

        // 4
        logPassFail(browseGamesPage.getNumberOfGameTilesLoaded() == numTiles, true);

        // 5
        storeFacet.clickOption("Sort", "Alphabetical - Z to A");
        numTiles = browseGamesPage.getNumberOfGameTilesLoaded();
        logPassFail(browseGamesPage.verifyTilesSortedByNameRev(), true);

        // 6
        logPassFail(browseGamesPage.getNumberOfGameTilesLoaded() == numTiles, true);
        client.stop();
        softAssertAll();
    }
}
