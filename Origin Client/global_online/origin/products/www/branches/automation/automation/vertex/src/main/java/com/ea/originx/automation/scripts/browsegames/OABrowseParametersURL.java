package com.ea.originx.automation.scripts.browsegames;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.store.BrowseGamesPage;
import com.ea.originx.automation.lib.pageobjects.store.StoreFacet;
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
 * Tests using the 'Browse Games' page with just URLs
 *
 * @author mdobre
 */
public class OABrowseParametersURL extends EAXVxTestTemplate{
    
    @TestRail(caseId = 11749)
    @Test(groups = {"browsegames", "full_regression"})
    public void testBrowseParametersURL(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getRandomAccount();
        String userName = userAccount.getUsername();
        final String baseURL = "https://dev.www.origin.com/can/en-us/store/browse?fq=genre:";
        final String rightParameter = "action";
        final String wrongParameter = "malarky";
        
        logFlowPoint("Log into Origin with an existing user."); //1
        logFlowPoint("Verify the facet is applied as a 'Browse Games' page URL is loaded."); //2
        logFlowPoint("Verify entitlements are loaded on the page"); //3
        logFlowPoint("Verify the given parameters are selected in the facet menu."); //4
        logFlowPoint("Navigate to an URL with incorrect parameters and verify a message appears stating no results were found."); //5
        
        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful as user: " + userName);
        } else {
            logFailExit("Failed: Cannot login as user: " + userName);
        }
        
        //2
        driver.navigate().to(baseURL + rightParameter);
        StoreFacet storeFacet = new StoreFacet(driver);
        if (storeFacet.verifyFacetVisible()) {
            logPass("Successfully verified the facet is applied as the page is loaded.");
        } else {
            logFailExit("Failed to verify the facet is applied as the page is loaded.");
        }
        
        //3
        BrowseGamesPage browseGamesPage = new BrowseGamesPage(driver);
        if (browseGamesPage.getLoadedStoreGameTiles().size() != 0) {
            logPass("Successfully verified entitlements are loaded on the page.");
        } else {
            logFailExit("Failed to verify entitlements are loaded on the page.");
        }
        
        //4
        if (storeFacet.isParameterSelected(rightParameter)) {
            logPass("Successfully verifed the given parameter is in the facet menu.");
        } else {
            logFailExit("Failed to verify the given parameter is in the facet menu.");
        }
        
        //5
        driver.navigate().to(baseURL + wrongParameter);
        if (Waits.pollingWait(()-> browseGamesPage.verifyNoResultsFound())) {
            logPass("Successfully verified a message appears stating no results were found.");
        } else {
            logFailExit("Failed to verify a message appears stating no results were found.");
        }
    }
}