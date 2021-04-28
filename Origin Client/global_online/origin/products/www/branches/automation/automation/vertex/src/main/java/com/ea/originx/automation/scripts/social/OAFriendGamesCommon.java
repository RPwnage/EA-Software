package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileGamesTab;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that the 'Games' tab on a friend's profile has the correct headers and
 * number of games in each section
 *
 * @author lscholte
 */
public class OAFriendGamesCommon extends EAXVxTestTemplate {

    @TestRail(caseId = 13067)
    @Test(groups = {"social", "full_regression"})
    public void testFriendGamesCommon(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final int gamesBothOwnCount = 5;
        final int otherGamesCount = 10;

        UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.FIVE_SHARED_ENTITLEMENTS);
        UserAccount friendAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.TEN_EXTRA_ENTITLEMENTS);

        userAccount.cleanFriends();
        friendAccount.cleanFriends();

        userAccount.addFriend(friendAccount);

        logFlowPoint("Login to Origin as " + userAccount.getUsername()); //1
        logFlowPoint("View the friend's profile through the social hub"); //2
        logFlowPoint("Navigate to the 'Games' tab on the friend's profile"); //3
        logFlowPoint("Verify the 'Games You Both Own' header displays the correct number"); //4
        logFlowPoint("Verify the 'Games You Both Own' entitlements are in alphabetical order"); //5
        logFlowPoint("Verify the 'Other Games' header displays the correct number"); //6
        logFlowPoint("Verify the 'Other Games' entitlements are in alphabetical order"); //7
        logFlowPoint("Verify there is no 'View All' option at the bottom of either section"); //8

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin as " + userAccount.getUsername());
        } else {
            logFailExit("Failed to log into Origin as " + userAccount.getUsername());
        }

        //2
        if (MacroProfile.navigateToFriendProfile(driver, friendAccount.getUsername())) {
            logPass("Successfully opened the friend's profile through the social hub");
        } else {
            logFailExit("Failed to open the friend's profile through the social hub");
        }

        //3
        ProfileHeader friendProfile = new ProfileHeader(driver);
        friendProfile.openGamesTab();
        if (friendProfile.verifyGamesTabActive()) {
            logPass("Successfully navigated to the 'Games' tab in the friend's profile");
        } else {
            logFailExit("Failed to navigate to the 'Games' tab in the friend's profile");
        }

        //4
        ProfileGamesTab gamesTab = new ProfileGamesTab(driver);
        if (gamesTab.verifyGamesBothOwnHeaderNumber(gamesBothOwnCount, gamesBothOwnCount)) {
            logPass("The 'Games You Both Own' header has the correct number of entitlements");
        } else {
            logFail("The 'Games You Both Own' header does not have the correct number of entitlements");
        }

        //5
        if (gamesTab.verifyGamesBothOwnAlphabeticalOrder()) {
            logPass("The entitlements in 'Games You Both Own' are in alphabetical order");
        } else {
            logFail("The entitlements in 'Games You Both Own' are not in alphabetical order");
        }

        //6
        if (gamesTab.verifyOtherGamesHeaderNumber(otherGamesCount, otherGamesCount)) {
            logPass("The 'Other Games' header has the correct number of entitlements");
        } else {
            logFail("The 'Other Games' header does not have the correct number of entitlements");
        }

        //7
        if (gamesTab.verifyOtherGamesAlphabeticalOrder()) {
            logPass("The entitlements in 'Other Games' are in alphabetical order");
        } else {
            logFail("The entitlements in 'Other Games' are not in alphabetical order");
        }

        //8
        boolean gamesBothOwnViewAllVisible = gamesTab.verifyGamesBothOwnViewAllVisible();
        boolean otherGamesViewAllVisible = gamesTab.verifyOtherGamesViewAllVisible();
        if (!gamesBothOwnViewAllVisible && !otherGamesViewAllVisible) {
            logPass("The 'View All' button is not visible in any section");
        } else {
            logFail("The 'View All' button is visible in one or more sections");
        }
        softAssertAll();
    }

}
