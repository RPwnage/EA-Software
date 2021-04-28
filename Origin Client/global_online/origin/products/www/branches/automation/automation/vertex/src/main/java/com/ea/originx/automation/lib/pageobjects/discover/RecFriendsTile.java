package com.ea.originx.automation.lib.pageobjects.discover;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.util.ArrayList;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.NoSuchElementException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents a 'Recommended Friends' tile on the 'My Home' page's 'Recommended
 * Friends' section.
 *
 * @author palui
 */
public class RecFriendsTile extends EAXVxSiteTemplate {

    private final WebElement rootElement;

    // The following css selectors are for child elements within the rootElement
    private static final String TILE_CSS = ".origin-friendtile";
    private static final String TILE_CONTAINER_CSS = TILE_CSS + " .origin-tile-container";

    private static final String TILE_OVERLAY_CSS = TILE_CONTAINER_CSS + " .origin-tile-overlay";
    private static final String TILE_DISPLAY_CSS = TILE_CSS + " .origin-tile-support-text origin-friends-tiledisplay";

    private static final By TILE_BIG_IMAGE_LOCATOR = By.cssSelector(TILE_CONTAINER_CSS + " .origin-friendtile-bgimage");

    private static final By ADD_FRIEND_BUTTON_LOCATOR = By.cssSelector(TILE_OVERLAY_CSS + " .origin-tile-overlay-content .otkbtn-primary");
    private static final By DISMISS_LINK_LOCATOR = By.cssSelector(TILE_OVERLAY_CSS + " .origin-tile-overlay-content .origin-tile-secondarycta");

    private static final By USER_NAME_LOCATOR = By.cssSelector(TILE_CSS + " .origin-tile-support-text .origin-friendtile-originId");
    private static final By FRIENDS_IN_COMMON_STRING_LOCATOR = By.cssSelector(TILE_CSS + " .origin-friendtile-text");

    // CSS for searching all 'Friends In Common' avatars
    private static final By FRIENDS_IN_COMMON_AVATARS_LOCATOR = By.cssSelector(TILE_DISPLAY_CSS + " .origin-avatarlist .origin-avatarlist-item");
    // XPATH template for searching for a particular avatar
    private static final String FRIENDS_IN_COMMON_AVATAR_XPATH_TMPL = "origin-tile-support-text//li[contains(@class,'origin-avatarlist-item')]//origin-avatar[contains(@nucleausis,'%s')]/..";

    // Definitions for anonymous users (subject to change as development is still in progress)
    public static final String ANONYMOUS_NUCLEUS_ID_SUBSTRING = "ANONYMOUS"; // Can be ANONYMOUS2 etc

    public static final String USER_CARD_ANONYMOUS = "This friend has a private profile"; // Configurable
    public static final String[] USER_CARD_ANONYMOUS_KEYWORDS = {"friend", "has", "private", "profile"};

    public static final UserAccount ANONYMOUS_USER = null;

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     * @param rootElement Root element of this 'Recommended Friends' tile
     */
    protected RecFriendsTile(WebDriver driver, WebElement rootElement) {
        super(driver);
        this.rootElement = rootElement;
    }

    /**
     * Verify if the 'Recommended Friends' tile is visible.
     *
     * @return true if this 'Recommended Friends' tile is visible, false otherwise
     */
    public boolean verifyRecFriendsTileVisible() {
        return waitIsElementVisible(rootElement, 2);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Child element utilities
    ////////////////////////////////////////////////////////////////////////////
    /**
     * @param childLocator Child locator within rootElement of this 'Recommend
     * Friends' tile
     * @return Child WebElement if found, or throw NoSuchElementException
     */
    private WebElement getChildElement(By childLocator) {
        return rootElement.findElement(childLocator);
    }

    /**
     * @param childLocator Child locator within rootElement of this 'Recommend
     * Friends' tile
     * @return Child WebElement text if found, or null if not found
     */
    private String getChildElementText(By childLocator) {
        try {
            return rootElement.findElement(childLocator).getAttribute("textContent");
        } catch (NoSuchElementException e) {
            return null;
        }
    }

    /**
     * Click child WebElement as specified by the child locator.
     *
     * @param childLocator Child locator within rootElement of this 'Recommend
     * Friends' tile
     */
    private void clickChildElement(By childLocator) {
        waitForElementClickable(getChildElement(childLocator)).click();
    }

    /**
     * Verify the child element is visible given its locator.
     *
     * @param childLocator Child locator within rootElement of this 'Recommend
     * Friends' tile
     * @return true if childElement is visible, false otherwise
     */
    private boolean verifyChildElementVisible(By childLocator) {
        return waitIsElementVisible(getChildElement(childLocator));
    }

    /**
     * Scroll to child WebElement and hover on it.
     *
     * @param childLocator child locator within rootElement of this 'Recommend
     * Friends' tile
     */
    private void scrollToAndHoverOnChildElement(By childLocator) {
        WebElement childElement = waitForElementVisible(getChildElement(childLocator));
        scrollToElement(childElement);
        sleep(500);
        hoverOnElement(childElement);
        sleep(500);
    }

    ////////////////////////////////////////////////////////////////////////////
    // User
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Get the nucleus ID of the current 'Recommended Friends' tile.
     *
     * @return Nucleus ID of the current 'Recommended Friends' tile
     */
    public String getNucleusId() {
        return rootElement.getAttribute("userId");
    }

    /**
     * Get the username of the current 'Recommended Friends' tile.
     *
     * @return username of the current 'Recommended Friends' tile
     */
    public String getUsername() {
        return getChildElementText(USER_NAME_LOCATOR);
    }

    /**
     * Verify the username matches the given username.
     *
     * @param username Expected username
     * @return true if this 'Recommended Friends' tile has matching username,
     * false otherwise
     */
    public boolean verifyUsername(String username) {
        return getUsername().equalsIgnoreCase(username);
    }

    ////////////////////////////////////////////////////////////////////////////
    // 'Friends In Common' text string
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Get the 'Friends In Common' text.
     *
     * @return 'Friends In Common' text String
     */
    public String getFriendsInCommonText() {
        return getChildElementText(FRIENDS_IN_COMMON_STRING_LOCATOR);
    }

    /**
     * Get the number of 'Friends In Common'.
     *
     * @return number of friends in common as per the 'Friends in Common' text
     * string
     */
    public int getFriendsInCommonCount() {
        return (int) StringHelper.extractNumberFromText(getFriendsInCommonText());
    }

    ////////////////////////////////////////////////////////////////////////////
    // 'Friends In Common' avatars
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Get a list of all 'Friends In Common' avatar WebElements.
     *
     * @return List of all 'Friends In Common' avatar WebElements on this
     * 'Recommended Friends' tile
     */
    public List<WebElement> getAllFriendsInCommonAvatarElements() {
        int nTries = 10;
        List<WebElement> friendAvatarElements = new ArrayList<>();
        // Retry as we may get an empty list if we retrieve the elements too soon
        for (int i = 0; i < nTries; i++) {
            friendAvatarElements = rootElement.findElements(FRIENDS_IN_COMMON_AVATARS_LOCATOR);
            if (!friendAvatarElements.isEmpty()) {
                break;
            }
            sleep(1000); // gap each iteration to allow avatars to finish update
        }
        return friendAvatarElements;
    }

    /**
     * Get a list of all 'Friends In Common' avatar objects.
     *
     * @return list of all 'Friends In Common' avatar objects on this 'Recommend
     * Friends' tile
     */
    public List<FriendsInCommonAvatar> getAllFriendsInCommonAvatars() {
        List<WebElement> avatarElements = getAllFriendsInCommonAvatarElements();
        List<FriendsInCommonAvatar> avatars = new ArrayList<>();
        for (WebElement element : avatarElements) {
            avatars.add(new FriendsInCommonAvatar(driver, element));
        }
        return avatars;
    }

    /**
     * Get a list of all 'Friends In Common' usernames.
     *
     * @return List of all 'Friends In Common' avatar usernames on this
     * 'Recommended Friends' tile
     */
    public List<String> getAllFriendsInCommonUsernames() {
        List<FriendsInCommonAvatar> avatars = getAllFriendsInCommonAvatars();
        List<String> usernames = new ArrayList<>();
        for (FriendsInCommonAvatar avatar : avatars) {
            usernames.add(avatar.getUsername());
        }
        return usernames;
    }

    /**
     * Verify if a user is in a list of 'Friends In Common' avatars.
     *
     * @param avatars List of 'Friends In Common' avatars
     * @param user User account for matching to the list of avatars
     *
     * @return true if user is in the list, false otherwise
     */
    private boolean verifyFriendInCommonExists(List<FriendsInCommonAvatar> avatars, UserAccount user) {
        if (avatars == null || avatars.isEmpty()) {
            throw new RuntimeException("List of 'Friends In Common' avatars must not be null");
        }
        for (FriendsInCommonAvatar friendInCommonAvatar : avatars) {
            if (friendInCommonAvatar.verifyUser(user)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Verify if a user is a 'Friend In Common' on this 'Recommended Friends' tile.
     *
     * @param user User account for checking if it is a 'Friend In Common'
     *
     * @return true if user is a 'Friend In Common', false otherwise
     */
    public boolean verifyFriendInCommonExists(UserAccount user) {
        return verifyFriendInCommonExists(getAllFriendsInCommonAvatars(), user);
    }

    /**
     * Verify if an array of users are 'Friends In Common' on this 'Recommend
     * Friends' tile
     *
     * @param users An array of user accounts for checking if they are 'Friends In
     * Common'
     * @return true if all users are 'Friends In Common', false otherwise
     */
    public boolean verifyFriendsInCommonExist(UserAccount... users) {
        if (users == null) {
            throw new RuntimeException("List of users must not be null");
        }
        List<FriendsInCommonAvatar> avatars = getAllFriendsInCommonAvatars();
        for (UserAccount user : users) {
            if (!verifyFriendInCommonExists(avatars, user)) {
                return false;
            }
        }
        return true;
    }

    /**
     * Get the 'Friends In Common' avatar element.
     *
     * @param nucleusId Nucleus ID of the user to search for in the 'Friends In
     * Common' avatars. Note this can be an 'anonymous' user with nucleus id
     * that contains 'ANONYMOUS' (or whatever it may be when finalized)
     * @return WebElement of the 'Friends In Common' avatar with matching
     * nucleusId, or throw NoSuchElementException exception
     *
     * @TODO Handle multiple anonymous 'Friends In Common' (Current assumption
     * is they will all have the same NucleusId)
     */
    private WebElement getFriendsInCommonAvatarElement(String nucleusId) {
        By tileLocator = By.xpath(String.format(FRIENDS_IN_COMMON_AVATAR_XPATH_TMPL, nucleusId));
        return getChildElement(tileLocator);
    }

    /**
     * Get the FriendsInCommonAvatar object.
     *
     * @param nucleusId User Nucleus ID to match to the 'Friends In Common'
     * avatars on this 'Recommended Friends' tile
     * @return 'Friends In Common' avatar with matching nucleusId, or throw
     * NoSuchElementException
     */
    public FriendsInCommonAvatar getFriendsInCommonAvatar(String nucleusId) {
        return new FriendsInCommonAvatar(driver, getFriendsInCommonAvatarElement(nucleusId));
    }

    /**
     * Get the number of 'Friends In Common' avatars.
     *
     * @return Number of friends in common by counting the number of friend
     * avatars
     */
    public int getFriendsInCommonAvatarsCount() {
        return getAllFriendsInCommonAvatarElements().size();
    }

    ////////////////////////////////////////////////////////////////////////////
    // Tile - 'Add Friend' button and 'Dismiss' link
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Hover on the 'Recommended Friends' button.
     */
    public void hoverOnTile() {
        scrollToAndHoverOnChildElement(TILE_BIG_IMAGE_LOCATOR);
    }

    /**
     * Verify the 'Add Friend' button is visible.
     *
     * @return true if this 'Recommended Friends' tile has a 'Add Friends' button
     * visible, false otherwise
     */
    public boolean verifyAddFriendButtonVisible() {
        hoverOnTile();
        return verifyChildElementVisible(ADD_FRIEND_BUTTON_LOCATOR);
    }

    /**
     * Get the 'Add Friend' button text.
     *
     * @return 'Add Friend' button text of this 'Recommended Friends' tile
     */
    public String getAddFriendButtonText() {
        return getChildElementText(ADD_FRIEND_BUTTON_LOCATOR);
    }

    /**
     * Verify the 'Add Friend' button text is as expected.
     *
     * @return true if the 'Add Friend' button text contains the keyword for
     * this 'Recommended Friends' tile, false otherwise
     */
    public boolean verifyAddFriendButtonTextMatches() {
        final String[] EXPECTED_ADD_FRIEND_BUTTON_KEYWORDS = {"Add", "Friend"};
        return StringHelper.containsIgnoreCase(getAddFriendButtonText(), EXPECTED_ADD_FRIEND_BUTTON_KEYWORDS);
    }

    /**
     * Click the 'Add Friend' button of this 'Recommended Friends' tile.
     */
    public void clickAddFriendButton() {
        hoverOnTile();
        clickChildElement(ADD_FRIEND_BUTTON_LOCATOR);
    }

    /**
     * Get the 'Dismiss' link text.
     *
     * @return 'Dismiss' link text of this 'Recommended Friends' tile
     */
    public String getDismissLinkText() {
        return getChildElementText(DISMISS_LINK_LOCATOR);
    }

    /**
     * Verify the 'Dismiss' link is visible.
     *
     * @return true if this 'Recommended Friends' tile has a 'Dismiss' link
     * visible, false otherwise
     */
    public boolean verifyDismissLinkVisible() {
        return verifyChildElementVisible(DISMISS_LINK_LOCATOR);
    }

    /**
     * Verify the 'Dismiss' link text is as expected.
     *
     * @return true if the 'Dismiss' link text contains the keyword for this
     * 'Recommended Friends' tile, false otherwise
     */
    public boolean verifyDismissLinkTextMatches() {
        final String[] EXPECTED_DISMISS_LINK_KEYWORDS = {"Dismiss"};
        return StringHelper.containsIgnoreCase(getDismissLinkText(), EXPECTED_DISMISS_LINK_KEYWORDS);
    }

    /**
     * Click the 'Dismiss' link of this 'Recommended Friends' tile.
     */
    public void clickDismissLink() {
        clickChildElement(DISMISS_LINK_LOCATOR);
    }
}
