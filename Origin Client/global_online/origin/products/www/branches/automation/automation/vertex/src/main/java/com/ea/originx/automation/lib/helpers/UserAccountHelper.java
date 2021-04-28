package com.ea.originx.automation.lib.helpers;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.utils.SocialService;
import com.ea.vx.originclient.utils.Waits;
import java.util.ArrayList;
import java.util.List;

/**
 *
 */
public class UserAccountHelper {

    /**
     * Adds multiple friends from the given list to a given user with the given criteria.
     *
     * @param currentUser The user to add friends to
     * @param numberOfFriends The number of friends to be added
     * @param criteria The criteria to get the accounts
     * @return The list of friends added to the user
     */
    public static List<UserAccount> addMultipleFriendsWithCriteria(UserAccount currentUser, int numberOfFriends, Criteria criteria) {

        List<UserAccount> friends = new ArrayList<>();
        for (int i = 0; i < numberOfFriends; i++) {
            final UserAccount friend = AccountManager.getInstance().requestWithCriteria(criteria);
            friend.cleanFriends();
            friends.add(friend);
            currentUser.addFriend(friend);
        }

        return friends;
    }

    /**
     * Adds multiple friends from the given list to a given user.
     *
     * @param currentUser The user to add friends to
     * @param numberOfFriends The number of friends to be added
     * @return The list of friends added to the user
     */
    public static List<UserAccount> addMultipleFriends(UserAccount currentUser, int numberOfFriends) {
        Criteria criteria = new Criteria.CriteriaBuilder().build();
        return addMultipleFriendsWithCriteria(currentUser, numberOfFriends, criteria);
    }

    /**
     * Sends an invite to a user and accepts the invitation on that user.
     *
     * @param currentUser The user to add friends to
     * @param friends The accounts to send an invite to
     */
    public static void addFriends(UserAccount currentUser, UserAccount... friends) {
        for (UserAccount friend : friends) {
            SocialService.inviteFriend(currentUser, friend.getNucleusUserId());
            SocialService.confirmInvitation(friend, currentUser.getNucleusUserId());
            // There's an throttle of 2 invites/second and 25 invites/minute
            // See https://developer.ea.com/display/EADPSocial/Friends+-+PC+Integration
            Waits.sleep(600);
        }
    }

    /**
     * Sends a friend invite to another user.
     *
     * @param currentUser The user that should invite a friend
     * @param friend Account to which request needs to be sent
     */
    public static void inviteFriend(UserAccount currentUser, UserAccount friend) {
        SocialService.inviteFriend(currentUser, friend.getNucleusUserId());
    }

    /**
     * Rejects a friend invitation.
     *
     * @param currentUser The user that should reject a friend request
     * @param friend Account that needs to be rejected
     */
    public static void rejectInvite(UserAccount currentUser, UserAccount friend) {
        SocialService.rejectInvitation(currentUser, friend.getNucleusUserId());
    }

    /**
     * Blocks a friend.
     *
     * @param currentUser The user that should block a friend
     * @param friend Account that needs to be blocked
     */
    public static void blockFriend(UserAccount currentUser, UserAccount friend) {
        SocialService.muteFriend(currentUser, friend.getNucleusUserId());
    }
}
