package com.ea.originx.automation.scripts.globalsearch;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

public class OABasicSearch extends EAXVxTestTemplate {

    @Test(groups = {"globalsearch", "long_smoke"})
    public void testBasicSearch(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccountA = AccountManager.getRandomAccount();
        final String userNameA = userAccountA.getUsername();
        final UserAccount userAccountB = AccountManager.getRandomAccount();
        final String userNameB = userAccountB.getUsername();

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String entitlementName = entitlement.getName();

        logFlowPoint("Launch and log into Origin with any random account");//1
        logFlowPoint("Search any random user and verify the results are correct");//2
        logFlowPoint("Search any entitlement and verify the results are correct");//3

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccountA)) {
            logPass("Successfully logged in as: " + userNameA);
        } else {
            logFailExit("Failed: to login to a user: " + userNameA);
        }

        //2
        GlobalSearch globalSearch = new GlobalSearch(driver);
        GlobalSearchResults searchResults = new GlobalSearchResults(driver);
        globalSearch.enterSearchText(userAccountB.getEmail());
        searchResults.waitForResults();
        if (searchResults.verifyUserFound(userAccountB)) {
            logPass("Searched a user: " + userNameB + " and verified the results are correct");
        } else {
            logFailExit("Failed: The results to search a user:" + userNameB + " are incorrect");
        }
        globalSearch.clearSearchText(false);

        //3
        globalSearch.enterSearchText(entitlementName);
        searchResults.waitForResults();
        if (Waits.pollingWait(() -> searchResults.storeResultTileVisible(entitlementName))) {
            logPass("Searched an entitlement: " + entitlementName + " and verified the results are correct");
        } else {
            logFailExit("Failed: The results to search an entitlement: " + entitlementName + "  are incorrect");
        }

        softAssertAll();
    }
}

