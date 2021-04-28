package com.ea.originx.automation.lib.pageobjects.profile;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * The 'Friends' tab on the 'Profile' page.
 *
 * @author lscholte
 */
public class ProfileFriendsTab extends EAXVxSiteTemplate {

    protected static final String FRIENDS_TAB_CSS = ".origin-profile-tab.origin-profile-friends-tab";
    protected static final String FRIENDS_TAB_XPATH = "//div[contains(@class, 'origin-profile-tab origin-profile-friends-tab')]";

    protected static final By MY_FRIENDS_TITLE_LOCATOR = By.xpath(FRIENDS_TAB_XPATH + "//h2[contains(@class,'otktitle-3 row-header') and contains(text(), 'My friends')]");

    protected static final String FRIENDS_TAB_NO_FRIENDS_MESSAGE_BODY_CSS = FRIENDS_TAB_CSS + " .otktitle-3.header-text";
    protected static final By FRIENDS_TAB_NO_FRIENDS_MESSAGE_TITLE_LOCATOR = By.cssSelector(FRIENDS_TAB_CSS + " .otktitle-2.header-text");
    protected static final By FRIENDS_TAB_NO_FRIENDS_MESSAGE_BODY_LOCATOR = By.cssSelector(FRIENDS_TAB_NO_FRIENDS_MESSAGE_BODY_CSS);
    protected static final By FRIENDS_TAB_NO_FRIENDS_FIND_FRIENDS_LINK_LOCATOR = By.cssSelector(FRIENDS_TAB_NO_FRIENDS_MESSAGE_BODY_CSS + " a.find-friends-link");

    protected static final String FRIEND_TILES_CSS = "origin-profile-page-friends-friendtile";
    protected static final String FRIEND_TILE_CSS_TMPL = FRIEND_TILES_CSS + "[username='%s']";
    protected static final By FRIEND_TILES_LOCATOR = By.cssSelector(FRIEND_TILES_CSS);
    protected static final By FRIEND_TILES_USERNAME_LOCATOR = By.cssSelector(FRIEND_TILES_CSS + " .origin-profile-page-friends-friendtile-username");
    protected static final By FRIEND_TILES_AVATAR_LOCATOR = By.cssSelector(FRIEND_TILES_CSS + " .origin-profile-page-friends-friendtile-avatar");

    protected static final By VIEW_MORE_BUTTON_LOCATOR = By.cssSelector(FRIENDS_TAB_CSS + " .show-more-less");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ProfileFriendsTab(WebDriver driver) {
        super(driver);
    }

    /**
     * Clicks the 'View More' button to show additional friends.
     */
    public void clickViewMore() {
        waitForElementClickable(VIEW_MORE_BUTTON_LOCATOR).click();
    }

    /**
     * Clicks the 'Find Friends' link that is found within the 'No Friends'
     * message.
     */
    public void clickFindFriendsLink() {
        waitForElementClickable(FRIENDS_TAB_NO_FRIENDS_FIND_FRIENDS_LINK_LOCATOR).click();
    }

    /**
     * Clicks on a specific user's friend tile to navigate to their profile.
     *
     * @param username The name of the user whose friend tile should be clicked
     */
    public void clickFriendTile(String username) {
        List<WebElement> names = waitForAllElementsVisible(FRIEND_TILES_USERNAME_LOCATOR);
        getElementWithText(names, username).click();
    }

    /**
     * Clicks the first friend tile that appears to navigate to their profile.
     */
    public void clickFirstFriendTile() {
        waitForAllElementsVisible(FRIEND_TILES_USERNAME_LOCATOR).get(0).click();
    }

    /**
     * Verify that there are no friends shown.
     *
     * @return true if no friends, false otherwise
     */
    public boolean verifyNoFriends() {
        return waitIsElementVisible(FRIENDS_TAB_NO_FRIENDS_FIND_FRIENDS_LINK_LOCATOR, 2);
    }

    /**
     * Verify the 'No Friends' message is displayed and that it is
     * correct.
     *
     * @return true if the 'No Friends' message is displayed and correct, false
     * otherwise
     */
    public boolean verifyNoFriendsMessage() {
        WebElement noFriendsMessageTitle, noFriendsMessageBody;

        try {
            noFriendsMessageTitle = waitForElementVisible(FRIENDS_TAB_NO_FRIENDS_MESSAGE_TITLE_LOCATOR);
            noFriendsMessageBody = waitForElementVisible(FRIENDS_TAB_NO_FRIENDS_MESSAGE_BODY_LOCATOR);
        } catch (TimeoutException e) {
            return false;
        }

        boolean titleCorrect = StringHelper.containsAnyIgnoreCase(noFriendsMessageTitle.getText(), OriginClientData.NO_FRIENDS_TITLE);
        final String expectedBody[] = {"friends", "list","find"};
        boolean bodyCorrect = StringHelper.containsIgnoreCase(noFriendsMessageBody.getText(), expectedBody);
        return titleCorrect && bodyCorrect;
    }

    /**
     * Verify the number of friends in the 'Friends' tab header is
     * correct.
     *
     * @param expectedNumberDisplayed The expected number of displayed friends
     * @param expectedNumberTotal The expected number of total friends
     * @return true if the header contains the correct number of displayed and
     * total friends, false otherwise
     */
    public boolean verifyMyFriendsHeader(int expectedNumberDisplayed, int expectedNumberTotal) {
        WebElement header = waitForElementVisible(MY_FRIENDS_TITLE_LOCATOR);
        return header.getText().contains(expectedNumberDisplayed + " of " + expectedNumberTotal);
    }

    /**
     * Verify that the correct number of friends are currently displayed.
     *
     * @param expectedNumber The expected number of friends to display
     * @return true if the correct number of friends are displayed
     */
    public boolean verifyNumberOfFriends(int expectedNumber) {
        return waitForAllElementsVisible(FRIEND_TILES_LOCATOR).size() == expectedNumber;
    }

    /**
     * Verify that the 'View More' button is visible.
     *
     * @return true if the 'View More' button is visible
     */
    public boolean verifyViewMoreVisible() {
        return waitIsElementVisible(VIEW_MORE_BUTTON_LOCATOR);
    }

    /**
     * Verify that the friend tiles are in alphabetical order (ignoring case).
     *
     * @return true if the entire set of displayed friend tiles are in
     * alphabetical order. False if at least one friend tiles is not in
     * alphabetical order.
     */
    public boolean verifyMyFriendsAlphabeticalOrder() {
        List<WebElement> friends = waitForAllElementsVisible(FRIEND_TILES_LOCATOR);

        for (int i = 0; i < friends.size() - 1; ++i) {
            String currentFriend = friends.get(i).getText().toLowerCase();
            String nextFriend = friends.get(i + 1).getText().toLowerCase();
            if (currentFriend.compareTo(nextFriend) > 0) {
                return false;
            }
        }

        return true;
    }

    /**
     * Verify that each of the friend tiles has a username.
     *
     * @param expectedNumber The expected number of usernames to be displayed
     * @return true if the correct number of usernames are displayed
     */
    public boolean verifyUsernamesDisplayed(int expectedNumber) {
        List<WebElement> usernames = waitForAllElementsVisible(FRIEND_TILES_USERNAME_LOCATOR);
        if (usernames.size() != expectedNumber) {
            return false;
        }

        for (WebElement username : usernames) {
            if (!username.isDisplayed() || username.getText().trim().isEmpty()) {
                //Username is empty or only whitespace
                //or it is not displayed
                return false;
            }
            System.out.println("Friend's username" + username);
        }
        return true;
    }

    /**
     * Verify that each of the friend tiles has an avatar.
     *
     * @param expectedNumber The expected number of avatars to be displayed
     * @return true if the correct number of avatars are displayed
     */
    public boolean verifyAvatarsDisplayed(int expectedNumber) {
        if (verifyNoFriends()) {
            return expectedNumber == 0;
        }
        List<WebElement> avatars = waitForAllElementsVisible(FRIEND_TILES_AVATAR_LOCATOR);
        if (avatars.size() != expectedNumber) {
            return false;
        }

        for (WebElement avatar : avatars) {
            if (!avatar.isDisplayed()) {
                return false;
            }
        }
        return true;
    }

    /**
     * Verify a 'Friend Tile' is visible given a username.
     *
     * @param username The username of the 'Friend Tile' to search for
     * @return true if a matching 'Friend Tile' is found, false otherwise
     */
    public boolean verifyFriendTileVisible(String username) {
        if (verifyNoFriends()) {
            return false;
        }
        List<WebElement> names = waitForAllElementsVisible(FRIEND_TILES_USERNAME_LOCATOR);
        return getElementWithText(names, username) != null;
    }

    /**
     * Get the friend tile WebElements given an user name.
     *
     * @param userName user name
     * @return profile friend tile WebElement for the given userName on this friends list
     * or throw NoSuchElementException
     */
    private WebElement getFriendTileElement(String userName) {
        scrollToElement(waitForElementVisible(By.cssSelector(String.format(FRIEND_TILE_CSS_TMPL, userName)))); //added this scroll as the element is hidden
        return waitForElementVisible(By.cssSelector(String.format(FRIEND_TILE_CSS_TMPL, userName)));
    }

    /**
     * Get a profile friend tile given an user name
     *
     * @param userName user name
     * @return profile friend tile WebElement for the given userName on this friends list
     * or throw NoSuchElementException
     */
    public ProfileFriendTile getProfileFriendTile(String userName) {
        return new ProfileFriendTile(driver, getFriendTileElement(userName));
    }
}