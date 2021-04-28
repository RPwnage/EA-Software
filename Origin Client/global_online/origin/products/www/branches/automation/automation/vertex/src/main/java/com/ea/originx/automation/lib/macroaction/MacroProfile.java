package com.ea.originx.automation.lib.macroaction;

import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileFriendsTab;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.vx.originclient.utils.Waits;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;

/**
 * Macro class containing static methods for multi-step actions related to the profile
 * pages and the mini profile.
 *
 * @author lscholte
 */
public class MacroProfile {

    private static final Logger _log = LogManager.getLogger(MacroProfile.class);

    /**
     * Constructor - make private as class should not be instantiated
     */
    private MacroProfile() {
    }

    /**
     * Navigates to a friend's profile by navigating to the current user's
     * profile, going to the friend's tab, and then finding the friend.
     *
     * @param driver Selenium WebDriver
     * @param friendUsername The name of the friend whose profile should be
     * navigated to
     * @return true if navigating to the friend's profile was successful, false
     * otherwise
     */
    public static boolean navigateToFriendProfile(WebDriver driver, String friendUsername) {
        new MiniProfile(driver).selectViewMyProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        
        if (!profileHeader.verifyOnProfilePage()) {
            _log.debug("Failed to navigate to the logged in user's profile page");
            return false;
        }

        profileHeader.openFriendsTab();
        if (!profileHeader.verifyFriendsTabActive()) {
            _log.debug("Failed to open the friends tab of the logged in user's profile");
            return false;
        }

        ProfileFriendsTab friendsTab = new ProfileFriendsTab(driver);
        friendsTab.clickFriendTile(friendUsername);
        profileHeader.waitForAchievementsTabToLoad(); // added this wait as to make sure the profile page is stabilized
        if (!profileHeader.verifyUsername(friendUsername)) {
            _log.debug("Failed to open the friend's profile page");
            return false;
        }
        return true;
    }

    /**
     * Navigates to a user's friends tab in their profile.
     *
     * @param driver Selenium WebDriver
     * @return true if navigating to the friends tab was successful, false
     * otherwise
     */
    public static boolean navigateToFriendsTab(WebDriver driver) {
        new MiniProfile(driver).clickProfileUsername();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        
        if (!profileHeader.verifyOnProfilePage()) {
            _log.debug("Failed to navigate to the logged in user's profile page");
            return false;
        }

        profileHeader.openFriendsTab();
        if (!profileHeader.verifyFriendsTabActive()) {
            _log.debug("Failed to open the friends tab of the logged in user's profile");
            return false;
        }

        return true;
    }

    /**
     * Verify profile page load for first friend in friends tab and user has reached to the right friends tab after
     *
     * @param driver Selenium WebDriver
     * @param userName user name for user
     * @return true if the friend's profile page is opened and user has reached to the right friends tab after.
     */
    public static boolean verifyFirstFriendProfilePage(WebDriver driver, String userName) {
        _log.debug("clicking first friend tile from friends tab");
        new ProfileFriendsTab(driver).clickFirstFriendTile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        boolean isProfileOpened = Waits.pollingWaitEx(() -> profileHeader.verifyOnProfilePage());
        new MiniProfile(driver).selectViewMyProfile();
        profileHeader.openFriendsTab();
        boolean isFriendsTabOpened = profileHeader.verifyFriendsTabActive();
        boolean isCurrentUserProfilePage = profileHeader.verifyUsername(userName);
        return isProfileOpened && isFriendsTabOpened && isCurrentUserProfilePage;
    }

    /**
     * Navigates to the Game section of Profile.
     *
     * @param driver Selenium WebDriver
     * @return True if successfully navigate to Game section of Profile, false
     * if not able     * to.
     */
    public static boolean navigateToGameSection(WebDriver driver) {
        new MiniProfile(driver).selectViewMyProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        if (!profileHeader.verifyOnProfilePage()) {
            _log.debug("Unable to navigate to profile page.");
            return false;
        }

        profileHeader.openGamesTab();
        return profileHeader.verifyGamesTabActive();
    }
}