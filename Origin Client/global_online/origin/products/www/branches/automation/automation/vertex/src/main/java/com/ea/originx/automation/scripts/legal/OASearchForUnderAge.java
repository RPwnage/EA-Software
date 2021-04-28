package com.ea.originx.automation.scripts.legal;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.dialog.SearchForFriendsDialog;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Verify that an underage user does not show up when searched for
 *
 * @author jmittertreiner
 */
public class OASearchForUnderAge extends EAXVxTestTemplate {

    @TestRail(caseId = 9595)
    @Test(groups = {"legal", "client_only", "full_regression"})
    public void testSearchForUnderAge(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount underAgeUser = AccountManagerHelper.createNewThrowAwayAccount(9);
        UserAccount adultUser = AccountManager.getRandomAccount();
        adultUser.cleanFriends();


        logFlowPoint("Created and log into an account as an underage user, then log out"); //1
        logFlowPoint("Log into an adult user"); //2
        logFlowPoint("Search for the Underage username in the global search box"); //3
        logFlowPoint("Verify the underage user does not appear on the search results"); //4
        logFlowPoint("Open the 'Search For Friends' dialog from the social hub"); //5
        logFlowPoint("Search for the Underage username"); //6
        logFlowPoint("Verify the underage user does not appear on the list"); //7

        //1
        WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, underAgeUser);
        new MainMenu(driver).selectLogOut(true);
        if (Waits.pollingWait(new LoginPage(driver)::isOnLoginPage)) {
            logPass("Created an underage user account");
        } else {
            logFailExit("Failed to create an underage user account");
        }

        //2
        if (MacroLogin.startLogin(driver, adultUser)) {
            logPass("Successfully logged into Origin with the adult user " + adultUser.getUsername());
        } else {
            logFailExit("Could not log into Origin with the adult user " + adultUser.getUsername());
        }

        // 3
        new GlobalSearch(driver).enterSearchText(underAgeUser.getUsername());
        GlobalSearchResults searchResults = new GlobalSearchResults(driver);
        if (searchResults.verifySearchDisplayed()) {
            logPass("Successfully searched for the underage user through the global search");
        } else {
            logFailExit("Failed to search for the underage user through the global search");
        }

        // 4
        if (searchResults.verifyNoPeopleResultsDisplayed()) {
            logPass("The underage user: (" + underAgeUser.getUsername() + ") does not show up in the search");
        } else {
            logFail("The underage user: (" + underAgeUser.getUsername() + ") shows up in the search");
        }

        //5
        MacroSocial.restoreAndExpandFriends(driver);
        new SocialHub(driver).clickFindFriendsButton();
        SearchForFriendsDialog searchFriendsDialog = new SearchForFriendsDialog(driver);
        if (searchFriendsDialog.waitIsVisible()) {
            logPass("Successfully opened the find friends dialog");
        } else {
            logFailExit("Failed to open the find friends dialog");
        }

        // 6
        searchFriendsDialog.enterSearchText(underAgeUser.getUsername());
        searchFriendsDialog.clickSearch();
        if (searchResults.verifySearchDisplayed()) {
            logPass("Successfully searched for the underage user through the find friends dialog");
        } else {
            logFailExit("Failed to search for the underage user through the find friends dialog");
        }

        // 7
        if (searchResults.verifyNoResultsDisplayed()) {
            logPass("The underage user: (" + underAgeUser.getUsername() + ") does not show up in the search");
        } else {
            logFail("The underage user: (" + underAgeUser.getUsername() + ") shows up in the search");
        }
        softAssertAll();
    }

}

