package com.ea.originx.automation.scripts.globalsearch;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;
import java.util.*;

/**
 * Tests that the 'Global Search' returns results that 'sound like' the entered
 * search term.
 *
 * @author jmittertreiner
 */
public class OASoundsLikeSearch extends EAXVxTestTemplate {

    @TestRail(caseId = 11735)
    @Test(groups = {"globalsearch", "full_regression"})
    public void testOASoundsLikeSearch(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Log into Origin."); // 1
        logFlowPoint("Enter 'sut' into 'Global Search' and verify products with 'set' in the title are returned."); // 2
        logFlowPoint("Enter 'mye' into 'Global Search' and verify 'Remember Me' and 'Devil May Cry' are returned."); // 3
        logFlowPoint("Enter 'fiva' into 'Global Search' and verify 'FIFA' products are returned."); // 4
        logFlowPoint("Enter 'Darcks' into 'Global Search' and verify products with 'Dark' in the title are returned."); // 5
        logFlowPoint("Enter 'comxzqer' into 'Global Search' and verify products with 'Conquer' in the title are returned."); // 6
        logFlowPoint("Enter 'sbpace' into 'Global Search' and verify products with 'space' in the title are returned."); // 7

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful to user account: " + userAccount.getUsername());
        } else {
            logFailExit("Failed: Cannot login to user account: " + userAccount.getUsername());
        }

        GlobalSearch globalSearch = new GlobalSearch(driver);
        GlobalSearchResults searchResults = new GlobalSearchResults(driver);

        // 2
        globalSearch.enterSearchText("sut");
        searchResults.waitForResults();
        List<String> sutExpected = Arrays.asList("set", "scout", "sid", "cut");
        if (Waits.pollingWait(() -> searchResults.verifyAnyTitleContainAnyOf(sutExpected))) {
            logPass("One of the games contained one of: " + sutExpected);
        } else {
            logFail("No games appeared containing any of: " + sutExpected);
        }

        // 3
        globalSearch.enterSearchText("mye");
        searchResults.waitForResults();
        List<String> myeExpected = Arrays.asList("me", "may");
        if (Waits.pollingWait(() -> searchResults.verifyAnyTitleContainAnyOf(myeExpected))) {
            logPass("One of the games contained one of: " + myeExpected);
        } else {
            logFail("No games appeared containing any of: " + myeExpected);
        }

        // 4
        globalSearch.enterSearchText("fiva");
        searchResults.waitForResults();
        List<String> fivaExpected = Collections.singletonList("fifa");
        if (Waits.pollingWait(() -> searchResults.verifyAnyTitleContainAnyOf(fivaExpected))) {
            logPass("One of the games contained one of: " + fivaExpected);
        } else {
            logFail("No games appeared containing any of: " + fivaExpected);
        }

        // 5
        globalSearch.enterSearchText("Darcks");
        searchResults.waitForResults();
        List<String> darcksExpected = Collections.singletonList("dark");
        if (Waits.pollingWait(() -> searchResults.verifyAnyTitleContainAnyOf(darcksExpected))) {
            logPass("One of the games contained one of: " + darcksExpected);
        } else {
            logFail("No games appeared containing any of: " + darcksExpected);
        }

        // 6
        globalSearch.enterSearchText("comxzqer");
        searchResults.waitForResults();
        List<String> comxzqerExpected = Collections.singletonList("conquer");
        if (Waits.pollingWait(() -> searchResults.verifyAnyTitleContainAnyOf(comxzqerExpected))) {
            logPass("One of the games contained one of: " + comxzqerExpected);
        } else {
            logFail("No games appeared containing any of: " + comxzqerExpected);
        }

        // 7
        globalSearch.enterSearchText("sbpace");
        searchResults.waitForResults();
        List<String> sbpaceExpected = Collections.singletonList("space");
        if (Waits.pollingWait(() -> searchResults.verifyAnyTitleContainAnyOf(sbpaceExpected))) {
            logPass("One of the games contained one of: " + sbpaceExpected);
        } else {
            logFail("No games appeared containing any of: " + sbpaceExpected);
        }

        client.stop();
        softAssertAll();
    }
}
