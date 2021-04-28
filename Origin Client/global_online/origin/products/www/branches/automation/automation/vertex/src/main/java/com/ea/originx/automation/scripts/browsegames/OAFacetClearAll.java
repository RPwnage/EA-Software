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
 * Test browse games store facet - clear all link
 *
 * @author palui
 */
public class OAFacetClearAll extends EAXVxTestTemplate {

    @TestRail(caseId = 2998166)
    @Test(groups = {"browsegames", "full_regression"})
    public void testFacetClearAll(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();
        String userName = userAccount.getUsername();
        userAccount.cleanFriends(); // prevent chat window pop-ups from covering the store facet

        logFlowPoint("Launch Origin client and login as user: " + userName); //1
        logFlowPoint("Navigate to 'Browse Games' Page"); //2
        logFlowPoint("Select store facet filters: 'Franchise - SimCity' and 'Platform - PC Download'"); //3
        logFlowPoint("Verify store facet 'Clear All' link is enabled. Click the link."); //4
        logFlowPoint("Verify 'Franchise - simcity' and 'Platform - PC Download' filters have been cleared (unchecked)"); //5
        logFlowPoint("Verify 'Clear All' link is disabled"); //6

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        NavigationSidebar navSideBar = new NavigationSidebar(driver);
        BrowseGamesPage browseGamesPage = navSideBar.gotoBrowseGamesPage();
        browseGamesPage.waitForGamesToLoad();
        logPassFail(browseGamesPage.verifyBrowseGamesPageReached(), true);
        SystemMessage siteStripe = new SystemMessage(driver);
        if (siteStripe.isSiteStripeVisible()) {
            siteStripe.closeSiteStripe();
        }

        //3
        StoreFacet storeFacet = new StoreFacet(driver);
        storeFacet.clickOption("Franchise", "SimCity");
        storeFacet.clickOption("Platform", "PC Download");
        boolean option2Checked = storeFacet.isOptionChecked("Franchise", "SimCity");
        boolean option3Checked = storeFacet.isOptionChecked("Platform", "PC Download");
        logPassFail(option2Checked && option3Checked, true);

        //4
        logPassFail(storeFacet.isClearAllLinkEnabled(), true);
        browseGamesPage.waitForGamesToLoad();
        storeFacet.clickClearAllLink();

        //5
        boolean option2Cleared = !storeFacet.isOptionChecked("Franchise", "SimCity");
        boolean option3Cleared = !storeFacet.isOptionChecked("Platform", "PC Download");
        logPassFail(option2Cleared && option3Cleared, true);

        //6
        logPassFail(!storeFacet.isClearAllLinkEnabled(), true);
        softAssertAll();
    }
}
