package com.ea.originx.automation.lib.pageobjects.discover;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.NoSuchElementException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents a 'Friends In Common' avatar object of a 'Recommend Friends' tile
 * on the Discovery page's 'Recommend Friends' section.
 *
 * @author palui
 */
public class FriendsInCommonAvatar extends EAXVxSiteTemplate {

    private final WebElement rootElement;

    // The following css selectors are for child elements within the rootElement
    private static final String AVATAR_CSS = "origin-avatar";
    private static final String AVATAR_IMG_CSS = AVATAR_CSS + " .otkavatar-img";
    private static final String USER_CARD_CSS = "origin-avatarlist-usercard origin-user-card";
    private static final String USER_CARD_USERNAME_CSS = USER_CARD_CSS + " .origin-usercard-userinfo .origin-usercard-username";
    private static final String USER_CARD_ANONYMOUS_CSS = USER_CARD_CSS + " .origin-usercard-privacy .otktitle-4";

    private static final By AVATAR_LOCATOR = By.cssSelector(AVATAR_CSS);
    private static final By AVATAR_IMG_LOCATOR = By.cssSelector(AVATAR_IMG_CSS);
    private static final By USER_CARD_LOCATOR = By.cssSelector(USER_CARD_CSS);
    private static final By USER_CARD_USERNAME_LOCATOR = By.cssSelector(USER_CARD_USERNAME_CSS);
    private static final By USER_CARD_ANONYMOUS_LOCATOR = By.cssSelector(USER_CARD_ANONYMOUS_CSS);

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     * @param rootElement Root element of this 'Recommend Friends' tile
     */
    protected FriendsInCommonAvatar(WebDriver driver, WebElement rootElement) {
        super(driver);
        this.rootElement = rootElement;
    }

    /**
     * Verify the 'Recommended Friends' tile is visible.
     *
     * @return true if this 'Recommend Friends' tile is visible, false otherwise
     */
    public boolean verifyFriendsInCommonAvatarVisible() {
        return waitIsElementVisible(rootElement, 2);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Child element utilities
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Get the child element.
     *
     * @param childLocator Child locator within rootElement of this 'Recommend
     * Friends' tile
     * @return Child WebElement if found, or throw NoSuchElementException
     */
    private WebElement getChildElement(By childLocator) {
        return rootElement.findElement(childLocator);
    }

    /**
     * Get the child element's text.
     *
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
     * Verify the child element is visible.
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
     * @param childLocator Child locator within rootElement of this 'Recommend
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
    // 'Friends In Common' avatar
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Get the nucleus ID of the current 'Friends In Common' avatar.
     *
     * @return Nucleus Id of the current 'Friends In Common' avatar
     */
    public String getNucleusId() {
        return getChildElement(AVATAR_LOCATOR).getAttribute("nucleusid");
    }

    /**
     * Get the username of the current 'Friends In Common' avatar.
     *
     * @return username of the current "Friends In Common' avatar. This is a
     * hidden attribute
     */
    public String getUsername() {
        return getChildElement(AVATAR_IMG_LOCATOR).getAttribute("alt");
    }

    /**
     * Verify the username of the 'Friends In Common' avatar is
     * the same as the given username.
     *
     * @param username The expected username
     * @return true if this 'Friends In Common' avatar has matching username,
     * false otherwise
     */
    public boolean verifyUsername(String username) {
        return getUsername().equalsIgnoreCase(username);
    }

    /**
     * Verify this is the avatar for a given user.
     *
     * @param user User account to match
     * @return true if this avatar is for the given user (with matching username
     * and nucleusId), false otherwise
     */
    public boolean verifyAvatar(UserAccount user) {
        boolean nucleusIdOK = getNucleusId().equalsIgnoreCase(user.getNucleusUserId());
        boolean usernameOK = getUsername().equalsIgnoreCase(user.getUsername());
        return nucleusIdOK && usernameOK;
    }

    /**
     * Verify the avatar is for an anonymous user.
     *
     * @return true if avatar is for an anonymous user, false otherwise
     */
    public boolean verifyAvatarAnonymous() {
        return StringHelper.containsIgnoreCase(getNucleusId(), RecFriendsTile.ANONYMOUS_NUCLEUS_ID_SUBSTRING);
    }

    /**
     * Hover on this avatar (to show the usercard).
     */
    public void hoverOnAvatar() {
        scrollToAndHoverOnChildElement(AVATAR_IMG_LOCATOR);
    }

    ////////////////////////////////////////////////////////////////////////////
    // 'Friends In Common' usercard (which pops up when hovering on the avatar)
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Verify that a usercard is visible.
     *
     * @return true if user card is visible, false otherwise
     */
    public boolean verifyUsercardVisible() {
        return verifyChildElementVisible(USER_CARD_LOCATOR);

    }

    /**
     * Get the Nucleus ID of a usercard for this avatar.
     *
     * @return Nucleus ID of the usercard for this avatar
     */
    public String getUsercardNucleusId() {
        return getChildElement(USER_CARD_LOCATOR).getAttribute("nucleusid");
    }

    /**
     * Get the username on the usercard for this avatar.
     *
     * @return Username on the usercard for this avatar. This is a visual
     * attribute which should always exist. Currently for anonymous user it is
     * not present. In that case a null string is returned (for now)
     */
    public String getUsercardUsername() {
        return getChildElementText(USER_CARD_USERNAME_LOCATOR);
    }

    /**
     * Get the username on the usercard for this avatar for an anonymous user.
     *
     * @return username on the usercard for this avatar. This is a visual
     * attribute which should always exist. Currently for anonymous user it is
     * not present. In that case a null string is returned (for now)
     */
    public String getUsercardAnonymous() {
        return getChildElementText(USER_CARD_ANONYMOUS_LOCATOR);
    }

    /**
     * Verify usercard on this avatar is for the given user.
     *
     * @param user User account to match
     * @return true if usercard on this avatar is for the user, false otherwise
     */
    public boolean verifyUsercard(UserAccount user) {
        boolean nucleusIdOK = getUsercardNucleusId().equalsIgnoreCase(user.getNucleusUserId());
        boolean usernameOK = getUsercardUsername().equalsIgnoreCase(user.getUsername());
        return nucleusIdOK && usernameOK;
    }

    /**
     * Verify the usercard on this avatar is for an anonymous user.
     *
     * @return true if usercard on this avatar is for an anonymous user, false
     * otherwise
     */
    public boolean verifyUsercardAnonymous() {
        boolean nucleusIdOK = StringHelper.containsIgnoreCase(getUsercardNucleusId(), RecFriendsTile.ANONYMOUS_NUCLEUS_ID_SUBSTRING);
        boolean usernameOK;
        String usercardUsername = getUsercardAnonymous();
        if (usercardUsername != null) {
            usernameOK = StringHelper.containsIgnoreCase(usercardUsername, RecFriendsTile.USER_CARD_ANONYMOUS_KEYWORDS);
        } else {
            usernameOK = false;
        }
        return nucleusIdOK && usernameOK;
    }

    /**
     * Verify the usercard and the avatar are for an anonymous user.
     *
     * @return true if both avatar and its usercard are for an anonymous user,
     * false otherwise
     */
    public boolean verifyAnonymous() {
        return verifyAvatarAnonymous() && verifyUsercardAnonymous();
    }

    /**
     * Verify the avatar is for the given user.
     *
     * @param user user account to match (can be anonymous)
     * @return true if both avatar and its usercard is for the user, false
     * otherwise
     */
    public boolean verifyUser(UserAccount user) {
        if (user != RecFriendsTile.ANONYMOUS_USER) {
            return verifyAvatar(user);
        } else {
            return verifyAvatarAnonymous();
        }
    }
}