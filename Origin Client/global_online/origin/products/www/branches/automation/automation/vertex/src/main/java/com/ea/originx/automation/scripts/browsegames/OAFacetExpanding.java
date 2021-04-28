package com.ea.originx.automation.scripts.browsegames;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.SystemMessage;
import com.ea.originx.automation.lib.pageobjects.store.BrowseGamesPage;
import com.ea.originx.automation.lib.pageobjects.store.StoreFacet;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * A test to verify the Franchise and Studio Store facets
 *
 * @author rmcneil
 */
public class OAFacetExpanding extends EAXVxTestTemplate {

    @TestRail(caseId = 11732)
    @Test(groups = {"browsegames", "full_regression"})
    public void testFacetExpanding(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();
        userAccount.cleanFriends(); // prevent chat window pop-ups from covering the store facet

        logFlowPoint("Log into Origin"); // 1
        logFlowPoint("Navigate to Browse Games Page"); // 2
        logFlowPoint("Click on the Franchise facet menu to open it"); // 3
        logFlowPoint("Click on the Franchise facet menu again to close it"); // 4

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        BrowseGamesPage browseGamesPage = navBar.gotoBrowseGamesPage();
        logPassFail(browseGamesPage.verifyBrowseGamesPageReached(), true);

        // 3
        StoreFacet storeFacet = new StoreFacet(driver);
        SystemMessage siteStripe = new SystemMessage(driver);
        if (siteStripe.isSiteStripeVisible()) {
            siteStripe.closeSiteStripe();
        }
        storeFacet.clickMenu("Franchise");
        logPassFail(storeFacet.areOptionsVisible("Franchise"), true);

        // 4
        storeFacet.clickMenu("Franchise");
        logPassFail(!storeFacet.areOptionsVisible("Franchise"), true);

        softAssertAll();
    }

}
