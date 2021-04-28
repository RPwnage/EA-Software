package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.common.Notification;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

import java.util.Arrays;

/**
 * Set of tests when user gets friend request from other user. Test friend
 * request by accepting and ignoring.
 *
 * @author rchoi
 */
public class OAFriendsInvitations extends EAXVxTestTemplate {

    @TestRail(caseId = 13199)
    @Test(groups = {"social", "client_only", "full_regression"})
    public void testFriendInvitation(ITestContext context) throws Exception {
        final OriginClient client1 = OriginClientFactory.create(context);
        final OriginClient client2 = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();
        UserAccount friend1 = AccountManager.getRandomAccount();
        UserAccount friend2 = AccountManager.getRandomAccount();
        UserAccount friend3 = AccountManager.getRandomAccount();
        UserAccount friend4 = AccountManager.getRandomAccount();
        UserAccount[] friends = {friend1, friend2, friend3, friend4};
        userAccount.cleanFriends();
        Arrays.stream(friends).forEach(UserAccount::cleanFriends);
        Arrays.stream(friends).forEach(friend -> UserAccountHelper.inviteFriend(friend, userAccount));

        logFlowPoint("Log into Origin with " + userAccount.getUsername() + " as userAccount A"); // 1
        logFlowPoint("Log into Origin with " + friend1.getUsername() + " as userAccount B whose friend request will be declined"); // 2
        logFlowPoint("Verify that incoming friend requests display the username of the pending friend."); // 3
        logFlowPoint("Verify that incoming friend requests display avatar of the pending friend."); // 4
        logFlowPoint("Verify that incoming friend requests are alphabetically sorted by the username of the user who sent them."); // 5
        logFlowPoint("Verify for incoming friend requests, there is a check-mark button to accept each friend request."); // 6
        logFlowPoint("Verify for incoming friend requests, there is an X button to ignore them."); // 7
        logFlowPoint("Verify that accepting a incoming friend request shows a checkmark graphic and adds the requester to the user's friends list."); // 8
        logFlowPoint("Verify that ignoring an incoming friend request dismisses it from the list of requests."); // 9
        logFlowPoint("Verify that the requester is not notified if his or her friend request is ignored."); // 10

        // 1
        // main user
        // Set the EACore setting for notification expiry time
        EACoreHelper.extendNotificationExpiryTime(client1.getEACore());
        WebDriver driver1 = startClientObject(context, client1);
        if (MacroLogin.startLogin(driver1, userAccount)) {
            logPass("Successfully logged into Origin with userAccount A " + userAccount.getUsername());
        } else {
            logFailExit("Could not log into Origin with userAccount A " + userAccount.getUsername());
        }

        // 2
        // second user whose friend request will be declined
        MacroSocial.restoreAndExpandFriends(driver1);
        SocialHub socialHub = new SocialHub(driver1);
        WebDriver driver2 = startClientObject(context, client2);
        if (MacroLogin.startLogin(driver2, friend1)) {
            logPass("Successfully logged into Origin with userAccount B " + friend1.getUsername());
        } else {
            logFailExit("Could not log into Origin with userAccount B " + friend1.getUsername());
        }

        // 3
        if (socialHub.verifyListOfUserNameExistsForFriendRequest()) {
            logPass("Verified that incoming friend requests display username");
        } else {
            logFailExit("incoming friend requests does NOT display username");
        }

        // 4
        if (socialHub.verifyAvatarsForFriendRequestExist()) {
            logPass("Verified that incoming friend requests display the avatar image");
        } else {
            logFailExit("incoming friend requests does NOT display the avatar image");
        }

        // 5
        if (socialHub.verifyUsernameSortedAlphabeticallybyFriendRequest()) {
            logPass("Verified that user names for friend request are sorted alphabetically");
        } else {
            logFailExit("User names for friend request are NOT sorted alphabetically");
        }

        // 6
        if (socialHub.verifyFriendsRequestAcceptIcons()) {
            logPass("Verified that there is a check-mark button to accept friend request");
        } else {
            logFailExit("There is no check-mark button for friends request");
        }

        // 7
        if (socialHub.verifyFriendsRequestDeclineIcons()) {
            logPass("Verified that there is an X button for ignoring friends request");
        } else {
            logFailExit("There is no x button for ignoring friends request");
        }

        // 8
        socialHub.acceptIncomingFriendRequest(friend2.getUsername());
        sleep(3000);
        socialHub.acceptIncomingFriendRequest(friend3.getUsername());
        sleep(3000);
        boolean friendRequestAcceptedAndNotExistInFriendRequest = socialHub.verifyFriendExists(friend2.getUsername())
                && socialHub.verifyFriendExists(friend3.getUsername())
                && !socialHub.verifyUserNameExistInFriendRequests(friend2.getUsername())
                && !socialHub.verifyUserNameExistInFriendRequests(friend3.getUsername());
        if (friendRequestAcceptedAndNotExistInFriendRequest) {
            logPass("Verify that accepting a incoming friend request adds the requester to the user's friends list");
        } else {
            logFailExit("accepting a incoming friend request does NOT adds the requester to the user's friends list");
        }

        // 9
        socialHub.declineIncomingFriendRequest(friend4.getUsername());
        sleep(2000);
        boolean userNotFriend = !socialHub.verifyFriendExists(friend4.getUsername());
        boolean friendRequestDeclinedAndNotExistInFriendRequest = userNotFriend
                && !socialHub.verifyUserNameExistInFriendRequests(friend4.getUsername());
        if (friendRequestDeclinedAndNotExistInFriendRequest) {
            logPass("Verify that ignoring an incoming friend request dismisses it from the list of requests.");
        } else {
            logFailExit("ignoring an incoming friend request does NOT dismisses it from the list of requests.");
        }

        // 10
        socialHub.declineIncomingFriendRequest(friend1.getUsername());
        boolean userNotNotified = new Notification(driver2).verifyUserNameNotInMessages(userAccount.getUsername());
        boolean requestNotInFriendRequests = !socialHub.verifyUserNameExistInFriendRequests(friend1.getUsername());
        if (userNotNotified && requestNotInFriendRequests) {
            logPass("Verified userAccount B does not get notified after getting declined");
        } else {
            logFail("userAccount B gets notified when friend request is declined");
        }
        softAssertAll();
    }

}
