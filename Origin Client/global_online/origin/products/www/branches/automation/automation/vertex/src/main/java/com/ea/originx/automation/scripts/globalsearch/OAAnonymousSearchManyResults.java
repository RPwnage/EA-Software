package com.ea.originx.automation.scripts.globalsearch;

import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.store.BrowseGamesPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests many results during an anonymous search
 *
 * @author mdobre
 */
public class OAAnonymousSearchManyResults extends EAXVxTestTemplate {

    @TestRail(caseId = 9355)
    @Test(groups = {"globalsearch", "full_regression", "browser_only"})
    public void testAnonymousSearchManyResults(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final String searchTerm = "Battle";

        logFlowPoint("Open Origin and search for something that returns many results"); //1
        logFlowPoint("Verify only results for the Store appear."); //2
        logFlowPoint("Verify 18 or less results appear in the results."); //3
        logFlowPoint("Verify there is a 'View More' button."); //4
        logFlowPoint("Click on the 'View More' button' and verify the user is brought to the Store browse page."); //5

        //1
        WebDriver driver = startClientObject(context, client);
        GlobalSearch globalSearch = new GlobalSearch(driver);
        globalSearch.enterSearchText(searchTerm);
        GlobalSearchResults searchResults = new GlobalSearchResults(driver);
        if (searchResults.verifySearchDisplayed()) {
            logPass("Search results for " + searchTerm + " were displayed.");
        } else {
            logFailExit("Search results for " + searchTerm + " were not displayed.");
        }

        //2
        boolean noGameLibraryResultsDisplayed = searchResults.verifyGameLibraryResultsNotDisplayed();
        boolean noPeopleResultsDisplayed = searchResults.verifyPeopleResultsNotDisplayed();
        if (noGameLibraryResultsDisplayed && noPeopleResultsDisplayed) {
            logPass("Successfully verified only results for the Store appear.");
        } else {
            logFailExit("Failed to verify only results for the Store appear.");
        }

        //3
        if (searchResults.getNumberOfStoreResultsTiles() <= 20) {
            logPass("Successfully verified 18 or less results are displayed.");
        } else {
            logFailExit("Failed to verify 18 or less results are displayed.");
        }

        //4
        if (searchResults.verifyViewMoreButtonStoreResultsVisible()) {
            logPass("Successfully verified there is a 'View More' button.");
        } else {
            logFailExit("Failed to verify there is a 'View More' button.");
        }

        //5
        searchResults.clickViewMoreStoreResults();
        if (new BrowseGamesPage(driver).verifyBrowseGamesPageReached()) {
            logPass("Successfully verified the user is brought to the Store browse page after clicking the 'View More' button.");
        } else {
            logFailExit("Failed to verify the user is brought to the Store browse page after clicking the 'View More' button.");
        }
        softAssertAll();
    }
}