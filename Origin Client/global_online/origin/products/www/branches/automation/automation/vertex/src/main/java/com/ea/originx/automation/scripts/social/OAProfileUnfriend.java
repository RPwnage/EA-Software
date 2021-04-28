package com.ea.originx.automation.scripts.social;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroProfile;
import com.ea.originx.automation.lib.pageobjects.dialog.UnfriendUserDialog;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileFriendsTab;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test for profile un-friend
 *
 * @author palui
 */
public class OAProfileUnfriend extends EAXVxTestTemplate {

    @TestRail(caseId = 13048)
    @Test(groups = {"full_regression", "social"})
    public void testProfileUnfriend(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();
        // Get a new account as friend to ensure friend's privacy policy is not 'private'
        UserAccount friendAccount = AccountManager.getInstance().createNewThrowAwayAccount(); // Default DOB 1986-01-01
        userAccount.cleanFriends();


        logFlowPoint("Launch Origin to register new friend account. Logout and add as friend: " + friendAccount.getUsername()); //1
        logFlowPoint("Login to user account: " + userAccount.getUsername()); //2
        logFlowPoint("From user account, view friend's profile: " + friendAccount.getUsername()); //3
        logFlowPoint("Open profile header dropdown and verify options: 'Unfriend', 'Unfriend and Block', 'Report User'"); //4
        logFlowPoint("Select 'Unfriend' option and verify 'Unfriend User' dialog appears"); //5
        logFlowPoint("Click 'Unfriend'. Verify 'Send Friend Request' button is now visible at friend's profile"); //6
        logFlowPoint("Verify 'No Friends' message is shown in user account's friend's profile 'Friends' tab"); //7
        logFlowPoint("Navigate to current user's (user account) profile"); //8
        logFlowPoint("Open 'Friends' tab and verify friend account no longer present: " + friendAccount.getUsername()); //9
        logFlowPoint("Logout from user account and login to friend account: " + friendAccount.getUsername()); //10
        logFlowPoint("Navigate to current user's (friend account) profile: " + friendAccount.getUsername()); //11
        logFlowPoint("Open 'Friends' tab and verify user account no longer present: " + userAccount.getUsername()); //12

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, friendAccount)) {
            logPass("Verified friend account created successfully: " + friendAccount.getUsername());
        } else {
            logFailExit("Failed: Cannot create friend account: " + friendAccount.getUsername());
        }
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.selectSignOut();
        sleep(2000); // pause before adding friend
        userAccount.addFriend(friendAccount);

        //2
        sleep(2000); // pause for Origin to stablize before re-login
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful to user account: " + userAccount.getUsername());
        } else {
            logFailExit("Failed: Cannot login to user account: " + userAccount.getUsername());
        }

        //3
        if (MacroProfile.navigateToFriendProfile(driver, friendAccount.getUsername())) {
            logPass("Verified user can view friend's profile");
        } else {
            logFailExit("Failed: User cannot view friend's profile");
        }

        //4
        ProfileHeader profileHeader = new ProfileHeader(driver);
        if (profileHeader.verifyDropdownOptions("Unfriend", "Unfriend and block", "Report user")) {
            logPass("Verified profile header dropdown has correct options");
        } else {
            logFailExit("Failed: Profile header dropdown does not have correct options");
        }

        //5
        profileHeader.selectDropdownOption("Unfriend");
        sleep(1000); // pause for 'Unfriend User' dialog to stablize
        UnfriendUserDialog uuDialog = new UnfriendUserDialog(driver);
        if (uuDialog.isOpen()) {
            logPass("Verified 'Unfriend User' dialog appears");
        } else {
            logFailExit("Failed: Cannot open 'Unfriend User' dialog");
        }

        //6
        uuDialog.clickUnfriend();
        sleep(1000); // pause for profile header to stablize
        if (profileHeader.verifySendFriendRequestButtonVisible()) {
            logPass("Verified 'Send Friend Request' button is visible");
        } else {
            logFailExit("Failed: 'Send Friend Request' button is not visible");
        }

        //7
        profileHeader.openFriendsTab();
        sleep(1000); // pause for 'Friends' tab content to stablize
        boolean friendsTabActive = profileHeader.verifyFriendsTabActive();
        boolean noFriend = friendsTabActive && new ProfileFriendsTab(driver).verifyNoFriendsMessage();
        if (noFriend) {
            logPass("Verified 'No Friends' message is shown in friend's profile 'Friends' tab");
        } else {
            logFailExit("Failed: 'No Friends' message is not shown in friend's profile 'Friends' tab");
        }

        //8
        miniProfile.selectProfileDropdownMenuItem("View My Profile");
        sleep(2000); // pause for 'Profile Header' to stablize
        if (profileHeader.verifyOnProfilePage()) {
            logPass("Verified current user's 'Profile' page opens");
        } else {
            logFailExit("Failed: Cannot open current user's 'Profile' page");
        }

        //9
        profileHeader.openFriendsTab();
        sleep(1000); // pause for 'Friends' tab content to stablize
        friendsTabActive = profileHeader.verifyFriendsTabActive();
        noFriend = friendsTabActive && new ProfileFriendsTab(driver).verifyNoFriendsMessage();
        if (noFriend) {
            logPass("Verified 'No Friends' message is shown in user's profile 'Friends' tab");
        } else {
            logFailExit("Failed: 'No Friends' message is not shown in user's profile 'Friends' tab");
        }

        //10
        miniProfile.selectSignOut();
        sleep(1000); // pause for Origin to stablize before relogin
        if (MacroLogin.startLogin(driver, friendAccount)) {
            logPass("Verified login successful to friend account: " + friendAccount.getUsername());
        } else {
            logFailExit("Failed: Cannot login to friend account: " + friendAccount.getUsername());
        }

        //11
        miniProfile.selectProfileDropdownMenuItem("View My Profile");
        sleep(2000); // pause for 'Profile Header' to stablize
        boolean onProfilePage = profileHeader.verifyOnProfilePage();
        boolean onProfileFriendAccount = onProfilePage && profileHeader.verifyUsername(friendAccount.getUsername());
        if (onProfileFriendAccount) {
            logPass("Verified current (friend account) user's 'Profile' page opens");
        } else {
            logFailExit("Failed: Cannot open current (friend account) user's 'Profile' page");
        }

        //12
        profileHeader.openFriendsTab();
        sleep(1000); // pause for 'Friends' tab content to stablize
        friendsTabActive = profileHeader.verifyFriendsTabActive();
        noFriend = friendsTabActive && new ProfileFriendsTab(driver).verifyNoFriendsMessage();
        if (noFriend) {
            logPass("Verified 'No Friends' message is shown in users's profile 'Friends' tab");
        } else {
            logFailExit("Failed: 'No Friends' message is not shown in user's profile 'Friends' tab");
        }
        softAssertAll();
    }
}
