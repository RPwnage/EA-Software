package com.ea.originx.automation.scripts.globalsearch;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.utils.Waits;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoConstants;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.helpers.ContextHelper;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the global search box in the Navigation bar
 *
 * @author rmcneil
 */
public class OAGlobalSearchBox extends EAXVxTestTemplate {

    @TestRail(caseId = 9337)
    @Test(groups = {"globalsearch", "full_regression"})
    public void testGlobalSearchBox(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        final String BATTLEFIELD = EntitlementInfoConstants.BF4_STANDARD_NAME;

        final String SEARCH_TERM = "Ba";
        final String SEARCH_TERM_LONG = "Battlefield";

        AccountManager accountManager = AccountManager.getInstance();
        Criteria criteria = new Criteria.CriteriaBuilder().build();
        criteria.addEntitlement(BATTLEFIELD);
        UserAccount user = accountManager.requestWithCriteria(criteria, 360);

        logFlowPoint("Launch and log in to Origin and verify search field is empty and placeholder reads \"Search Games and More\"");   //1
        logFlowPoint("Type in search bar and verify user can input text to the field"); //2
        logFlowPoint("Verify that as the user types, the url updates with the search query");   //3
        logFlowPoint("Verify that as the user types, the search results begin appearing and updating with each character"); //4
        logFlowPoint("Press backspace or delete to erase entered text and verify that if the user presses the backspace/delete key and removes a character, the search results update again as each character is deleted"); //5
        logFlowPoint("Click elsewhere in the browser/client and verify if the field is deselected the \"Search Games and More\" placeholder text returns and the search box is no longer searching");   //6
        logFlowPoint("Click the back button of the browser/client and Verify that if the user hits the back button it returns to the page before the search began");    //7
        logFlowPoint("Click the forward button of the browser/client and verify the \"Forward\" button is active after using the \"Back\" button and will return the user to the previous page");   //8
        logFlowPoint("Click the \"X\" button in the search bar and verify clicking on the x clears the input field and resets the search function");    //9

        //1
        WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, user);
        GlobalSearch globalSearch = new GlobalSearch(driver);
        if (globalSearch.verifyEmptyGlobalSearch()) {
            logPass("Successfully verified empty search box");
        } else {
            logFailExit("Could not verify empty search box");
        }

        //2
        globalSearch.enterSearchText(SEARCH_TERM);
        if (globalSearch.verifySearchBoxValue(SEARCH_TERM)) {
            logPass("Successfully verified user can enter text into the search box");
        } else {
            logFailExit("Could not enter text into the search box");
        }

        //3
        if (driver.getCurrentUrl().contains("?searchString=" + SEARCH_TERM)) {
            logPass("Succesffully verified search term appears in URL");
        } else {
            logFail("Could not find search term in URL");
        }

        //4
        //Search should get more refined (dececreased results) as you add more characters
        globalSearch.clearSearchText(false);
        globalSearch.addTextToSearch(SEARCH_TERM_LONG.substring(0, 1));
        GlobalSearchResults searchResults = new GlobalSearchResults(driver);
        int firstLetterResults = searchResults.getNumberOfStoreResults();
        boolean searchResultsDecrease = true;
        int stringLength = SEARCH_TERM_LONG.length();

        for (int i = 1; i < stringLength; ++i) {
            String character = (i + 1) < stringLength ? SEARCH_TERM_LONG.substring(i, (i + 1)) : SEARCH_TERM_LONG.substring(i);
            globalSearch.addTextToSearch(character);

            //Wait for search to finish
            Waits.sleep(1000);

            if (searchResults.getNumberOfStoreResults() > firstLetterResults) {
                searchResultsDecrease = false;
                break;
            }
        }

        if (searchResultsDecrease) {
            logPass("Sucessfully verified search updates per character added");
        } else {
            logFail("Search did not update per character added");
        }

        //5
        //Search should get more broad (increase results) as characters are deleted
        int fullLengthNumberOfResults = searchResults.getNumberOfStoreResults();
        boolean searchResultsIncrease = true;
        for (int i = 0; i < (stringLength - 1); ++i) {
            globalSearch.deleteLetterByBackspace();
            if (searchResults.getNumberOfStoreResults() < fullLengthNumberOfResults) {
                searchResultsIncrease = false;
                break;
            }
        }

        if (searchResultsIncrease) {
            logPass("Sucessfully verified search updates per character deleted");
        } else {
            logFail("Search did not update per character deleted");
        }

        //6
        globalSearch.clearSearchText(true);
        new NavigationSidebar(driver).clickStoreLink();
        if (globalSearch.verifyEmptyGlobalSearch()) {
            logPass("Successfully verified deleting text and clicking off search returns search to default, unfocused state");
        } else {
            logFailExit("Could not verify search box returned to original state");
        }

        //7
        String previousUrl = driver.getCurrentUrl();
        globalSearch.enterSearchText(SEARCH_TERM);
        String searchURL = driver.getCurrentUrl();
        driver.navigate().back();
        if (previousUrl.equals(previousUrl)) {
            logPass("Successfully verified selecting Back will return to page before search");
        } else {
            logFail("Could not navigate to page before search");
        }

        //8
        driver.navigate().forward();
        if (searchURL.equals(driver.getCurrentUrl())) {
            logPass("Sucessfully verified selecting Forward will return to search results");
        } else {
            logFail("Could not navigate forward to search results");
        }

        //9
        globalSearch.clearSearchText(true);
        if (globalSearch.verifyEmptyGlobalSearch() && previousUrl.equals(driver.getCurrentUrl())) {
            logPass("Verified selecting X button clears Global Search and returns user to page before search");
        } else {
            logFail("Could not clear Global Search and return to page before search");
        }

        softAssertAll();
    }

}
