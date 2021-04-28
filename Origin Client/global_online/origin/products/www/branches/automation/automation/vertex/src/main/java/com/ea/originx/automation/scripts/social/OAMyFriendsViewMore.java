package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroProfile;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileFriendTile;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileFriendsTab;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.SocialService;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to verify the functionality of the 'View More' button and status of friend
 * in the 'Friends' tab of the profile page with more than 100 friends
 *
 * @author lscholte
 * @author rocky
 */
public class OAMyFriendsViewMore extends EAXVxTestTemplate {

    @TestRail(caseId = 13065)
    @Test(groups = {"social", "full_regression"})
    public void testMyFriendsViewMore(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);   // for checking friends in friends tab
        final OriginClient clientB = OriginClientFactory.create(context);  // for checking status of friend

        final OADipSmallGame dipSmallGame = new OADipSmallGame();

        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.MORE_THAN_100_FRIENDS);
        final UserAccount friendAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.HAVE_ONE_FRIEND);

        final int defaultFriendCount = OriginClientData.DEFAULT_FRIENDS_TAB_FRIEND_COUNT;
        final int defaultViewMoreFriendCount = OriginClientData.DEFAULT_VIEW_MORE_FRIEND_COUNT;
        final int totalFriendCount = SocialService.getFriendCount(userAccount.getAuthToken(), userAccount.getNucleusUserId());

        logFlowPoint("Log into client A with the user A account which has more than 100 friends"); //1
        logFlowPoint("Open the 'Friends' tab from current user's profile"); //2
        logFlowPoint(String.format("Verify the title of the section is 'My Friends - %d of %d'", defaultFriendCount, totalFriendCount)); //3
        logFlowPoint(String.format("Verify the 'My Friends' section displays the default friend count: %s by counting friend tiles", defaultFriendCount)); //4
        logFlowPoint(String.format("Verify there is a 'View More' button that increases the viewed friends by %s when clicked.", defaultViewMoreFriendCount)); //5
        logFlowPoint(String.format("Verify there is a 'View More' button that increases the viewed friends by %s when clicked again.", defaultViewMoreFriendCount)); //6
        logFlowPoint(String.format("Re-open friends tab and verify the 'My Friends' section header displays default friend count: %s correctly", defaultFriendCount)); //7
        logFlowPoint("Verify The friends in 'My Friends' section are displayed in alphabetical order"); //8
        logFlowPoint("Verify all friends have an avatar"); //9
        logFlowPoint("Verify clicking friend's username shows the friend's profile page and returning to current user's friends tab page"); //10
        logFlowPoint("Successfully log into Client B with friend B account."); //11
        logFlowPoint("Verify the friend B of user A account is showing as 'online' from the other account's friends tab after logging in"); //12
        logFlowPoint("Set friend B as away and verify it shows as 'away' from the other account's friends tab"); //13
        logFlowPoint("Set friend B as ingame by playing game and verify it shows as 'ingame' from the other account's friends tab"); //14
        logFlowPoint("Set friend B as offline and verify it shows as 'offline' from the other account's friends tab"); //15

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass(String.format("Logged into client A with user A account which has %d friends", totalFriendCount));
        } else {
            logFailExit("Could not log into Origin");
        }

        //2
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.waitForPageToLoad();
        miniProfile.selectViewMyProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        profileHeader.openFriendsTab();
        if (profileHeader.verifyFriendsTabActive()) {
            logPass("Successfully opened the 'Friends' tab from current user's profile");
        } else {
            logFailExit("Failed to open the 'Friends' tab from current user's profile");
        }

        //3
        ProfileFriendsTab friendsTab = new ProfileFriendsTab(driver);
        if (friendsTab.verifyMyFriendsHeader(defaultFriendCount, totalFriendCount)) {
            logPass(String.format("Verified the title of the section is 'My Friends - %d of %d'", defaultFriendCount, totalFriendCount));
        } else {
            logFail(String.format("Failed: couldn't verify the title of the section is 'My Friends - %d of %d'", defaultFriendCount, totalFriendCount));
        }

        //4
        if (friendsTab.verifyNumberOfFriends(defaultFriendCount)) {
            logPass(String.format("Verified the 'My Friends' section displays the default friend count: %s by counting friend tiles", defaultFriendCount));
        } else {
            logFail(String.format("Failed: The 'My Friends' section does not display the default friend count: %s by counting friend tiles", defaultFriendCount));
        }

        //5
        friendsTab.clickViewMore();
        boolean isHeaderDisplayedCorrectly = friendsTab.verifyMyFriendsHeader(defaultFriendCount + defaultViewMoreFriendCount, totalFriendCount);
        boolean isFriendCountDisplayedCorrectly = friendsTab.verifyNumberOfFriends(defaultFriendCount + defaultViewMoreFriendCount);
        if (isHeaderDisplayedCorrectly && isFriendCountDisplayedCorrectly) {
            logPass(String.format("Verified there is a 'View More' button that increases the viewed friends by %s when clicked.", defaultViewMoreFriendCount));
        } else {
            logFail(String.format("Failed: Couldn't verify 'View More' button that increases the viewed friends by %s when clicked.", defaultViewMoreFriendCount));
        }

        //6
        friendsTab.clickViewMore();
        isHeaderDisplayedCorrectly = friendsTab.verifyMyFriendsHeader(totalFriendCount, totalFriendCount);
        isFriendCountDisplayedCorrectly = friendsTab.verifyNumberOfFriends(totalFriendCount);
        if (isHeaderDisplayedCorrectly && isFriendCountDisplayedCorrectly) {
            logPass(String.format("Verified there is a 'View More' button that increases the viewed friends by %s when clicked again.", defaultViewMoreFriendCount));
        } else {
            logFail(String.format("Failed: Couldn't verify 'View More' button that increases the viewed friends by %s when clicked again.", defaultViewMoreFriendCount));
        }

        //7
        profileHeader.openGamesTab();
        profileHeader.openFriendsTab();
        isHeaderDisplayedCorrectly = friendsTab.verifyMyFriendsHeader(defaultFriendCount, totalFriendCount);
        isFriendCountDisplayedCorrectly = friendsTab.verifyNumberOfFriends(defaultFriendCount);
        if (isHeaderDisplayedCorrectly && isFriendCountDisplayedCorrectly) {
            logPass(String.format("Re-opened friends tab and verified the 'My Friends' section header displays default friend count: %s correctly", defaultFriendCount));
        } else {
            logFail(String.format("Failed: the 'My Friends' section header does not display default friend count: %s correctly after re-opening friends tab", defaultFriendCount));
        }

        //8
        if (friendsTab.verifyMyFriendsAlphabeticalOrder()) {
            logPass("Verified The 'My Friends' section is displayed in alphabetical order");
        } else {
            logFail("Failed: the 'My Friends' section is not displayed in alphabetical order");
        }

        //9
        if (friendsTab.verifyAvatarsDisplayed(defaultFriendCount)) {
            logPass("Verified all friends have a avatar");
        } else {
            logFail("Failed: not all friends have a avatar");
        }

        //10
        if (MacroProfile.verifyFirstFriendProfilePage(driver, userAccount.getUsername())) {
            logPass("Verified opening a friend's profile by clicking friend's username and returning to current user's friends tab page");
        } else {
            logFail("Failed to open a friend's profile by clicking friend's username and/or to return to current user's friends tab page ");
        }

        //11
        WebDriver driver2 = startClientObject(context, clientB);
        if (MacroLogin.startLogin(driver2, friendAccount)) {
            logPass("Successfully logged into client B with friend B account");
        } else {
            logFailExit("Could not log into client B with friend B account");
        }

        //12
        ProfileFriendTile friendTile = friendsTab.getProfileFriendTile(friendAccount.getUsername());
        if (Waits.pollingWaitEx(()->friendTile.getPresenceFromAvatar() == ProfileFriendTile.PresenceType.ONLINE)) {
            logPass("Verified status for the friend B of user A account is showing as 'online' from the other account's friends tab in profile page");
        } else {
            logFail("Failed to see 'online' status from the other account's friends tab in profile page after friend B logged into client");
        }

        //13
        MainMenu mainMenu = new MainMenu(driver2);
        mainMenu.selectStatusAway();
        if (Waits.pollingWaitEx(()->friendTile.getPresenceFromAvatar() == ProfileFriendTile.PresenceType.AWAY)) {
            logPass("The friend B's status was set to 'away' and verified it shows as 'away' from the other account's friends tab");
        } else {
            logFail("Failed to see 'away' status from the other account's friends tab after setting friend B's status to 'away'");
        }

        //14
        new NavigationSidebar(driver2).gotoGameLibrary();
        GameTile dipSmallTile = new GameTile(driver2, dipSmallGame.getOfferId());
        MacroGameLibrary.downloadFullEntitlement(driver2, dipSmallGame.getOfferId());
        dipSmallTile.play();
        if (Waits.pollingWait(()->friendTile.getPresenceFromAvatar() == ProfileFriendTile.PresenceType.INGAME, 100000, 500, 0)) {
            logPass("While friend B played game, verified the status shows as 'ingame' from the other account's friends tab");
        } else {
            logFail("Failed to see 'ingame' status from the other account's friends tab while friend B played game");
        }

        //15
        mainMenu.selectGoOffline();
        if (Waits.pollingWaitEx(()->friendTile.getPresenceFromAvatar() == ProfileFriendTile.PresenceType.OFFLINE)) {
            logPass("The friend B's status was set to offline and verified it shows as 'offline' from the other account's friends tab");
        } else {
            logFail("Failed to see 'offline' status from the other account's friends tab after setting friend B's status to 'offline'");
        }
        softAssertAll();
    }
}
