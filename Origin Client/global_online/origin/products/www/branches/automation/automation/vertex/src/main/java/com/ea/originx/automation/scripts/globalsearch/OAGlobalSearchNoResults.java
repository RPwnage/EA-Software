package com.ea.originx.automation.scripts.globalsearch;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Verifies 'Global Search' for no results
 *
 * @author cbalea
 */
public class OAGlobalSearchNoResults extends EAXVxTestTemplate {
    
    @TestRail(caseId = 9346)
    @Test(groups = {"globalsearch", "full_regression"})
    public void testGlobalSearchNoResults(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getRandomAccount();
        final String searchText = "j$19&ye1";
        
        logFlowPoint("Launch and log in to Origin with an existing user"); // 1
        logFlowPoint("Enter something which will not have results into the search box and "
                + "verify there is a message stating there are no results"); // 2
        logFlowPoint("Verify there is a carousel with some tiles for games"); // 3
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin");
        } else {
            logFailExit("Failed to log into Origin");
        }
        
        // 2
        new GlobalSearch(driver).enterSearchText(searchText);
        GlobalSearchResults globalSearchResults = new GlobalSearchResults(driver);
        globalSearchResults.waitForResults();
        if(globalSearchResults.verifyNoResultsDisplayed()){
            logPass("Successfully verified that no results were found");
        } else {
            logFailExit("Failed: results were found");
        }
        
        // 3
        Waits.sleep(2000); // waiting for carousel to stabilize
        boolean isFeaturedGamesCarouselResultDisplayed = globalSearchResults.getNumberOfDisplayedFeaturedGamesResults() >= 1;
        boolean isFeaturedGamesCarouselTitleDisplayed = globalSearchResults.verifyFeaturedGamesTitleDisplayed();
        boolean isFeaturedGamesCarouselImagesDisplayed = globalSearchResults.verifyFeaturedGamesImagesDisplayed();
        if(isFeaturedGamesCarouselResultDisplayed && isFeaturedGamesCarouselTitleDisplayed && isFeaturedGamesCarouselImagesDisplayed){
            logPass("Successfully verified 'Featured Game Carousel' is present");
        } else {
            logFailExit("Failed to verify the presence of 'Featured Game Carousel'");
        }
        softAssertAll();
    }
}
