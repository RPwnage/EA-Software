package com.ea.originx.automation.lib.macroaction;

import com.ea.originx.automation.lib.pageobjects.social.Friend;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.dialog.SearchForFriendsDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.UnfriendAndBlockDialog;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.pageobjects.social.SocialHubMinimized;
import static com.ea.vx.originclient.templates.OATestBase.sleep;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;

/**
 * Macro class containing static methods for multi-step actions related to social.
 *
 * @author glivingstone
 */
public class MacroSocial {

    private static final Logger _log = LogManager.getLogger(MacroProfile.class);

    /**
     * Constructor - make private as class should not be instantiated
     */
    private MacroSocial() {
    }

    /**
     * Restores and expands friends.
     *
     * @param driver Selenium WebDriver
     * @return true if successfully restored and expanded friends
     */
    public static boolean restoreAndExpandFriends(WebDriver driver) {
        new SocialHubMinimized(driver).restoreSocialHub();
        SocialHub socialHub = new SocialHub(driver);
        Waits.sleep(1000); // Wait for the animation to complete.

        if (socialHub.isFriendsCollapsed()) {
            socialHub.clickFriendsHeader();
        }
        if (socialHub.isOutgoingCollapsed()) {
            socialHub.clickOutgoingHeader();
        }
        if (socialHub.isIncomingCollapsed()) {
            socialHub.clickIncomingHeader();
        }

        return true;
    }

    /**
     * Navigates to a stranger's profile by searching and then clicking on 'View
     * Profile' for said user.
     *
     * @param driver Selenium WebDriver
     * @param user The friend user account whose profile should be navigated to
     * @return true if navigating to the friend's profile was successful, false
     * otherwise
     */
    public static boolean navigateToUserProfileThruSearch(WebDriver driver, UserAccount user) {
        new GlobalSearch(driver).enterSearchText(user.getEmail());
        GlobalSearchResults searchResults = new GlobalSearchResults(driver);
        searchResults.waitForResults();
        if (!searchResults.verifyUserFound(user)) {
            _log.debug("User did not appear after search");
            return false;
        }
        searchResults.viewProfileOfUser(user);
        return true;
    }
    
    /**
     * Navigates to a user profile by searching them in the search for friends
     * dialog.
     * Clicking on 'View Profile' for said user.
     * Send a friend request to said user.
     *  
     * @param driver Selenium WebDriver
     * @param user The friend user account whose profile should be navigated to
     * @return true if a friend request was send, false otherwise
     */
    public static boolean searchForUserSendFriendRequestViaProfile(WebDriver driver, UserAccount user) {
        final String friendAccountUsername = user.getUsername();
        
        // Search a user by email adress
        SearchForFriendsDialog searchFriendsDialog = new SearchForFriendsDialog(driver);
        searchFriendsDialog.enterSearchText(user.getEmail());
        searchFriendsDialog.clickSearch();
        
        GlobalSearchResults searchResults = new GlobalSearchResults(driver);
        searchResults.waitForResults();
        if (!searchResults.verifyUserFound(user)) {
            _log.debug("User did not appear after search");
            return false;
        }
        
        // If the user was found, navigate to his profile
        searchResults.viewProfileOfUser(user);
        ProfileHeader profileHeader = new ProfileHeader(driver);
        profileHeader.waitForProfilePage();
        if(!profileHeader.verifyUsername(user.getUsername())){
            _log.debug("Failed to navigate to the profile of the friend " + friendAccountUsername);
            return false;
        }
        
        // Send a friend request
        profileHeader.clickSendFriendRequest();
        if(Waits.pollingWait(() -> !profileHeader.verifyFriendRequestSentVisible())){
            _log.debug("Failed to send a friend request to " + friendAccountUsername);
            return false;
        }
        
        return true;
    }

    /**
     * Add a friend through UI. This method requires 2 clients at the same time
     *
     * @param driver1 Selenium WebDriver for the first client
     * @param driver2 Selenium WebDriver for the second client
     * @param userAccountA The user account who send the friend invitation
     * @param userAccountB The user account who accept the friend request
     * @return true if the 2 accounts are friends and both appear in the
     * 'SocialHub' dialog, false otherwise
     */
    public static boolean addFriendThroughUI(WebDriver driver1, WebDriver driver2, UserAccount userAccountA, UserAccount userAccountB) {
        // first client setup
        restoreAndExpandFriends(driver1);
        SocialHub socialHub = new SocialHub(driver1);
        socialHub.clickFindFriendsButton();
        SearchForFriendsDialog searchFriendsDialog = new SearchForFriendsDialog(driver1);
        searchFriendsDialog.waitIsVisible();
        boolean isFriendRequestSent = searchForUserSendFriendRequestViaProfile(driver1, userAccountB);
        
        // second client setup
        restoreAndExpandFriends(driver2);
        SocialHub socialHub2 = new SocialHub(driver2);
        socialHub2.acceptIncomingFriendRequest(userAccountA.getUsername());
        sleep(3000);
        boolean isFriendAdded = socialHub2.verifyFriendExists(userAccountA.getUsername());
        socialHub.clickMinimizeSocialHub();
        socialHub2.clickMinimizeSocialHub();
        return isFriendRequestSent && isFriendAdded;
    }
    
    /**
     * Remove all friends from a specific account through UI
     * 
     * @param driver Selenium WebDriver
     * @return true if the list of friends is empty, false otherwise
     */
    public static boolean cleanUpAllFriendsThroughUI(WebDriver driver) {
        restoreAndExpandFriends(driver);
        SocialHub socialHub = new SocialHub(driver);
        if(socialHub.verifyFindFriendsButtonDisplayed()) {
            // if the 'Find Friends' button is visible there are no friends in the list
            return true;
        } else {
            List<String> friends = socialHub.getAllFriendUsernames();
            friends.forEach(f -> socialHub.getFriend(f).unfriend(driver));
            return socialHub.verifyFindFriendsButtonDisplayed();
        }
    }
    
    /**
     * Perform cleanFriends on all users and friends and then add each friend
     * (in friends) to each user (in users).
     *
     * @param users An array of user accounts to establish friendship
     * @param friends An array of user accounts to be added as friends
     */
    public static void cleanAndAddFriendship(UserAccount[] users, UserAccount[] friends) {
        for (UserAccount user : users) {
            user.cleanFriends();
        }
        for (UserAccount friend : friends) {
            friend.cleanFriends();
            for (UserAccount user : users) {
                user.addFriend(friend);
            }
        }
    }

    /**
     * Unfriend and block a friend via 'Profile'
     *
     * @param driver Selenium WebDriver
     * @param friendToBlock The friend user account to be unfriended and blocked
     * @return True if the friend is successfully blocked, false otherwise
     */
    public static boolean unfriendAndBlock(WebDriver driver, UserAccount friendToBlock) {
        if (!MacroProfile.navigateToFriendProfile(driver, friendToBlock.getUsername())) {
            return false;
        }

        new ProfileHeader(driver).selectUnfriendAndBlockFromDropDownMenu();

        UnfriendAndBlockDialog unfriendAndBlockDialog = new UnfriendAndBlockDialog(driver);
        if (!unfriendAndBlockDialog.waitIsVisible()) {
            return false;
        }

        unfriendAndBlockDialog.clickUnfriendAndBlock();
        return unfriendAndBlockDialog.waitIsClose();
    }

    /**
     * Restores the 'Social Hub' if it is not collapsed and opens a chat window with a specific friend
     *
     * @param driver Selenium Web Driver
     * @param accountName the name of the friend's account to open the chat window with
     */
    public static void openChatWithFriend(WebDriver driver, String accountName) {
        SocialHub socialHub = new SocialHub(driver);
        SocialHubMinimized socialHubMinimized = new SocialHubMinimized(driver);
        if (socialHubMinimized.verifyMinimized()) {
            socialHubMinimized.restoreSocialHub();
        }
        Friend friend = socialHub.getFriend(accountName);
        friend.openChat();
    }
}