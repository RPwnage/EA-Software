package com.ea.originx.automation.lib.pageobjects.social;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.dialog.UnfriendUserDialog;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub.PresenceType;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.NoSuchElementException;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.support.Color;
import java.util.List;
import java.util.Optional;
import static com.ea.originx.automation.lib.resources.OriginClientData.ORIGIN_CLICK_FRIEND_HIGHLIGHT_COLOUR;

/**
 * Component Object for a Friend
 *
 * @author sng
 */
public class Friend extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(Friend.class);
    protected static final String RIGHT_CLICK_MENU_CSS = "div.otkcontextmenu-wrap.origin-social-hub-roster-contextmenu.otkcontextmenu-isvisible";
    protected static final By RIGHT_CLICK_MENU_LOCATOR = By.cssSelector(RIGHT_CLICK_MENU_CSS);
    protected static final By RIGHT_CLICK_ITEM_LIST_LOCATOR = By.cssSelector(RIGHT_CLICK_MENU_CSS + " li[role='presentation'] a");
    protected static final By FRIEND_REAL_NAME = By.cssSelector("h3.origin-social-hub-roster-friend-realname strong");
    protected static final By PRESENCE_LOCATOR = By.cssSelector(".origin-social-hub-roster-friend-activity.otktitle-4");
    protected static final By AVATAR_LOCATOR = By.cssSelector(".otkavatar");
    protected static final By FRIEND_MENU_ICON_LOCATOR = By.cssSelector(".origin-social-hub-roster-friend-menu-icon");
    /**
     * The WebElement in the 'Social Hub' for this 'Friend' object.
     */
    protected final By friendWebElementLocator;

    /**
     * Constructor when the WebElement for the friend is known.
     *
     * @param driver Selenium WebDriver
     * @param friendWebElementLocator The locator for finding the WebElement
     * representing the friend
     */
    protected Friend(WebDriver driver, By friendWebElementLocator) {
        super(driver);
        this.friendWebElementLocator = friendWebElementLocator;
    }

    /**
     * Open a chat with a friend by doubleClicking.
     */
    public void openChat() {
        openChat(false);
    }

    /**
     * Open a chat with a friend.
     *
     * @param rightClick - Specifying true will open the chat through the
     * right-click menu. False will open the chat by double clicking
     */
    public void openChat(boolean rightClick) {
        // Kludge to fix contextClick not enabled sometimes
        // nativeClick(friendWebElement, true);
        final WebElement friendWebElement = waitForElementPresent(friendWebElementLocator);
        if (rightClick) {
            _log.debug("right click to open chat");
            contextClick(friendWebElement);
            waitForElementVisible(waitForChildElementPresent(friendWebElement, RIGHT_CLICK_MENU_LOCATOR));
            getRightClickMenuItemByText("Send Message").click();
        } else {
            _log.debug("double click to open chat");
            doubleClick(friendWebElement);
        }
        sleep(1000); // Let the animation complete.
    }

    /**
     * Verify the option 'View Profile' is visible
     *
     * @return true if the option is visible, false otherwise.
     */
    public boolean verifyViewAndClickProfileVisible() {
        _log.debug("view profile");
        final WebElement friendWebElement = waitForElementPresent(friendWebElementLocator);
        contextClick(friendWebElement);
        waitForElementVisible(waitForChildElementPresent(friendWebElement, RIGHT_CLICK_MENU_LOCATOR), 10);
        boolean isViewProfileVisible = getRightClickMenuItemByText("View Profile").isDisplayed();
        sleep(5000); // waiting for profile options to stabilize
        getRightClickMenuItemByText("View Profile").click(); // clicks on the 'View Profile' in order to make that menu disappear
        return isViewProfileVisible;
    }

    /**
     * View a friend's profile
     */
    public void viewProfile() {
        _log.debug("view profile");
        final WebElement friendWebElement = waitForElementPresent(friendWebElementLocator);
        contextClick(friendWebElement);
        waitForElementVisible(waitForChildElementPresent(friendWebElement, RIGHT_CLICK_MENU_LOCATOR), 10);
        getRightClickMenuItemByText("View Profile").click();
    }

    /**
     * clicks the friend
     */
    public void clickFriend() {
        driver.findElement(friendWebElementLocator).click();
    }

    /**
     * verifies if a left click highlights the friend
     *
     * @return true if the friend is highlighted, false otherwise
     */
    public boolean verifyLeftClickFriend() {
        final WebElement friendWebElement = waitForElementPresent(friendWebElementLocator);
        driver.findElement(friendWebElementLocator).click();
        sleep(5000); // wait for element to stabilize
        return friendWebElement.isDisplayed();
    }

    /**
     * Verifies if the highlight color is grey
     *
     * @return true if the color is grey, false otherwise
     */
    public boolean verifyHighlightedFriend() {
        driver.findElement(friendWebElementLocator).click();
        String textColor = Color.fromString(driver.findElement(friendWebElementLocator).getCssValue("background-color")).asHex();
        return StringHelper.containsIgnoreCase(textColor, ORIGIN_CLICK_FRIEND_HIGHLIGHT_COLOUR);
    }

    /**
     * Removes this friend.
     * @param driver Selenium WebDriver
     */
    public void unfriend(WebDriver driver) {
        _log.debug("unfriend");
        final WebElement friendWebElement = waitForElementPresent(friendWebElementLocator);
        contextClick(friendWebElement);
        waitForElementVisible(waitForChildElementPresent(friendWebElement, RIGHT_CLICK_MENU_LOCATOR), 10);
        getRightClickMenuItemByText("Unfriend").click();
        UnfriendUserDialog unfriendUser = new UnfriendUserDialog(driver);
        unfriendUser.waitIsVisible();
        unfriendUser.clickUnfriend();
        unfriendUser.waitIsClose();
    }

    /**
     * Cancels friend request and blocks the user
     */
    public void ignoreFriendRequestAndBlock() {
        _log.debug("Block");
        if (!verifyContextMenuOpen()) {
            openContextMenu();
        }
        WebElement cancelAndBlock = getRightClickMenuItemByText("Block");
        if (cancelAndBlock != null) {
            waitForElementClickable(cancelAndBlock).click();
        }
    }

    /**
     * Gets a specific WebElement from a right-click menu.
     *
     * @param text The text to search for in the right-click menu
     * @return The WebElement from the right click menu, or null if it could not
     * be found
     */
    private WebElement getRightClickMenuItemByText(String text) {
        if (!verifyContextMenuOpen()) {
            clickFriendMenuIcon();
        }
        final WebElement friendWebElement = waitForElementPresent(friendWebElementLocator);
        List<WebElement> rightClickMenuItems = friendWebElement.findElements(RIGHT_CLICK_ITEM_LIST_LOCATOR);
        WebElement correctItem = null;
        for (WebElement menuItem : rightClickMenuItems) {
            if (menuItem.getAttribute("textContent").equalsIgnoreCase(text)) {
                correctItem = menuItem;
                break;
            }
        }

        return correctItem;
    }

    /**
     * Gets the real name of a friend if it exists.
     *
     * @return The friend's real name they have one, else, an empty optional
     */
    public Optional<String> getRealName() {
        WebElement realNameElement;
        try {
            realNameElement = waitForChildElementPresent(friendWebElementLocator, FRIEND_REAL_NAME, 2);
        } catch (TimeoutException e) {
            return Optional.empty();
        }
        hoverOnElement(realNameElement);
        waitForElementVisible(realNameElement);
        return Optional.of(realNameElement.getText().replace("\u00a0", " ")); //Replace the non-breaking space character
    }

    /**
     * Get a friend's presence from their avatar.
     *
     * @return The friend's presence type
     */
    public PresenceType getPresenceFromAvatar() {
        _log.debug("getting real name");
        final WebElement avatar = waitForChildElementPresent(friendWebElementLocator, AVATAR_LOCATOR);
        String avatarClassAttribute = avatar.getAttribute("class");

        if (avatarClassAttribute.contains("otkavatar-isonline")) {
            return PresenceType.ONLINE;
        } else if (avatarClassAttribute.contains("otkavatar-isingame")) {
            return PresenceType.INGAME;
        } else if (avatarClassAttribute.contains("otkavatar-isaway")) {
            return PresenceType.AWAY;
        } else if (avatarClassAttribute.contains("otkavatar-isoffline")) {
            return PresenceType.OFFLINE;
        } else {
            return null; //TODO: have an UNKNOWN constant
        }
    }

    public void clickFriendMenuIcon() {
        hoverElement(waitForElementVisible(friendWebElementLocator));
        WebElement friendMenuIcon = getChildElemement(FRIEND_MENU_ICON_LOCATOR);
        if (friendMenuIcon != null) {
            waitForElementClickable(friendMenuIcon).click();
        }
    }

    /**
     * Verify that the friend is shown as 'Offline'.
     *
     * @return true if the friend is offline, false otherwise
     */
    public boolean verifyPresenceOffline() {
        return verifyPresence("Offline");
    }

    /**
     * Verify that the friend is shown as 'Away'.
     *
     * @return true if the friend is away, false otherwise
     */
    public boolean verifyPresenceAway() {
        return verifyPresence("Away");
    }

    /**
     * Verify that the friend is shown as 'Online'.
     *
     * @return true if the friend is online, false otherwise
     */
    public boolean verifyPresenceOnline() {
        return verifyPresence("Online");
    }

    /**
     * Verify that the friend is shown as 'In Game'.
     *
     * @return true if the friend is in game, false otherwise.
     */
    public boolean verifyPresenceInGame() {
        //In game presence shows as "Playing + entitlement name"
        return getPresence().contains("Playing");
    }

    /**
     * Verify that the friend's presence matches the expected text.
     *
     * @param expectedPresence The expected text for the friend's presence
     * @return true if the friend's presence exactly matches the expected text,
     * false otherwise
     */
    private boolean verifyPresence(String expectedPresence) {
        return getPresence().equals(expectedPresence);
    }

    /**
     * Get the friend's presence.
     *
     * @return Friend's presence
     */
    private String getPresence() {
        WebElement presence = waitForChildElementPresent(friendWebElementLocator, PRESENCE_LOCATOR);
        return presence.getText();
    }

    /**
     * Opens the context menu on a friend in friends list
     */
    public void openContextMenu() {
        if (verifyContextMenuOpen()) {
            return;
        }
        final WebElement friendWebElement = waitForElementPresent(friendWebElementLocator);
        hoverElement(friendWebElement);
        contextClick(friendWebElement);
    }

    /**
     * Checks if the friend context menu is open
     *
     * @return True if the context menu is open, false otherwise
     */
    public boolean verifyContextMenuOpen() {
        final WebElement friendWebElement = waitForElementPresent(friendWebElementLocator);
        return waitIsChildElementVisible(friendWebElement, RIGHT_CLICK_MENU_LOCATOR, 10);
    }

    /**
     * Returns the child element of the friend element in friends list
     *
     * @return The WebElement child of friend element
     */
    private WebElement getChildElemement(By locator) {
        try {
            WebElement friendWebElement = waitForElementPresent(friendWebElementLocator);
            return friendWebElement.findElement(locator);
        } catch (NoSuchElementException | TimeoutException e) {
            _log.error("Unable to find child element: " + e);
            return null;
        }
    }
}