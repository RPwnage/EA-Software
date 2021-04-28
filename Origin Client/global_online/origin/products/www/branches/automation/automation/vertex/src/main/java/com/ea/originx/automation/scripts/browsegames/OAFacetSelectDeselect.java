package com.ea.originx.automation.scripts.browsegames;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.store.BrowseGamesPage;
import com.ea.originx.automation.lib.pageobjects.store.StoreFacet;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test browse games store facet - select/deselect menu options
 *
 * @author palui
 */
public class OAFacetSelectDeselect extends EAXVxTestTemplate {

    @TestRail(caseId = 11733)
    @Test(groups = {"browsegames", "full_regression"})
    public void testFacetSelectDeselect(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();
        String userName = userAccount.getUsername();
        String franchiseName = "A Way Out";
        userAccount.cleanFriends(); // prevent chat window pop-ups from covering the store facet


        logFlowPoint("Launch Origin client and login as user: " + userName); //1
        logFlowPoint("Navigate to 'Browse Games' page"); //2 
        logFlowPoint("Click store facet filter 'FIFA'. Verify the filter option is checked"); //3
        logFlowPoint("Verify all store game tiles are 'FIFA' games"); //5
        logFlowPoint("Click store facet filter 'FIFA' again. Verify the filter option is unchecked"); //5
        logFlowPoint("Verify some store game tile prices can now not be 'FIFA' games"); //6

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        NavigationSidebar navSideBar = new NavigationSidebar(driver);
        BrowseGamesPage browseGamesPage = navSideBar.gotoBrowseGamesPage();
        browseGamesPage.waitForGamesToLoad();
        logPassFail(browseGamesPage.verifyBrowseGamesPageReached(), true);

        //3
        StoreFacet storeFacet = new StoreFacet(driver);
        storeFacet.clickOption("Franchise", franchiseName);
        logPassFail(storeFacet.isOptionChecked("Franchise", franchiseName), true);

        //4
        logPassFail(browseGamesPage.verifyAllStoreGameTitlesContain(franchiseName), true);

        //5
        storeFacet.clickOption("Franchise", franchiseName);
        logPassFail(!storeFacet.isOptionChecked("Franchise", franchiseName), true);

        //6
        logPassFail(!browseGamesPage.verifyAllStoreGameTitlesContain(franchiseName), true);

        softAssertAll();
    }
}
