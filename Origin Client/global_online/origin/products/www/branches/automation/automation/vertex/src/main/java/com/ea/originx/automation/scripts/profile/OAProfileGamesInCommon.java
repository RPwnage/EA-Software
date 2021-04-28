package com.ea.originx.automation.scripts.profile;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileGamesTab;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

import java.util.List;

import org.testng.annotations.Test;

/**
 * Tests a non-friends profile for games in common
 *
 * @author sbentley
 */
public class OAProfileGamesInCommon extends EAXVxTestTemplate {

    @TestRail(caseId = 1193771)
    @Test(groups = {"profile", "full_regression"})
    public void OAProfileGamesInCommon(ITestContext context) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.FIVE_SHARED_ENTITLEMENTS);
        UserAccount otherAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.TEN_EXTRA_ENTITLEMENTS);

        userAccount.cleanFriends();
        otherAccount.cleanFriends();

        logFlowPoint("Login to Origin using an account with entitlements");  //1
        logFlowPoint("Search for non-friend user who has similar entitlements using global search");    //2
        logFlowPoint("Navigate to the Profile > Games Tab of other user");  //3
        logFlowPoint("Verify other user's Games profile has both 'Games You Both Own' section and 'Other Games' section");   //4
        logFlowPoint("Verify 'Games You Both Own' shows only games that are in legal");  //5
        logFlowPoint("Verify all other non-friend's games are shown under 'Other Games'");  //6

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged in as " + userAccount.getUsername());
        } else {
            logFailExit("Could not log in as " + userAccount.getUsername());
        }

        //2
        new GlobalSearch(driver).enterSearchText(otherAccount.getUsername());
        GlobalSearchResults globalSearchResults = new GlobalSearchResults(driver);
        if (globalSearchResults.verifyPeopleResults(otherAccount.getUsername())) {
            logPass("Sucessfully searched for " + otherAccount.getUsername());
        } else {
            logFailExit("Could not search for " + otherAccount.getUsername());
        }

        //3
        globalSearchResults.viewProfileOfUser(otherAccount);
        ProfileHeader otherProfile = new ProfileHeader(driver);
        otherProfile.openGamesTab();
        if (otherProfile.verifyGamesTabActive()) {
            logPass("Successfully navigated to the 'Games' tab");
        } else {
            logFailExit("Failed to navigate to the 'Games' tab");
        }

        //4
        ProfileGamesTab gamesTab = new ProfileGamesTab(driver);
        if (gamesTab.verifyBothGamesOwnHeader() && gamesTab.verifyOtherGamesHeader()) {
            logPass("Verified 'Both Games Own' and 'Other Games' headers appears");
        } else {
            logFailExit("Failed to verify headers appeared");
        }

        //5
        List<String> bothOwnGamesTitles = gamesTab.getBothOwnGamesTitles();
        List<String> otherGameTitles = gamesTab.getOtherGamesTitles();

        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryContainsExpectedGamesByTitle(bothOwnGamesTitles)) {
            logPass("Verified 'Both Games Owned' contains games owned");
        } else {
            logFail("Failed to verify 'Both Games Owned' contains games owned");
        }

        //6
        if (!gameLibrary.verifyGameLibraryContainsExpectedGamesByTitle(otherGameTitles)) {
            logPass("Verified 'Other Games' contains games not owned");
        } else {
            logFail("Failed to verify 'Other Games' contains games not owned");
        }

        softAssertAll();
    }

}
