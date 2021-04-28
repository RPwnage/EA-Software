package com.ea.originx.automation.lib.pageobjects.profile;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * The friend tile from profile friends tab
 *
 * @author rocky
 */
public class ProfileFriendTile extends EAXVxSiteTemplate {

    private final WebElement rootElement;

    // The following CSS selectors are for child elements within the rootElement
    protected static final By FRIEND_AVATAR_STATUS = By.className("otkavatar");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ProfileFriendTile(WebDriver driver, WebElement element) {
        super(driver);
        this.rootElement = element;
    }

    /**
     * The enum for presence type.
     */
    public enum PresenceType {
       ONLINE(0), INGAME(1), AWAY(2), OFFLINE(3);

        private final int index;

        PresenceType(int index) {
            this.index = index;
        }
    }

    /**
     * Get the child element given its locator.
     *
     * @param childLocator Child locator within rootElement of this 'ProfileFriend'
     * tile.
     * @return Child WebElement if found, or throw NoSuchElementException
     */
    private WebElement getChildElement(By childLocator) {
        return rootElement.findElement(childLocator);
    }

    /**
     * Get a friend's presence from their avatar.
     *
     * @return The friend's presence type
     */
    public PresenceType getPresenceFromAvatar() {
        final WebElement avatar = getChildElement(FRIEND_AVATAR_STATUS);
        final String avatarClassAttribute = avatar.getAttribute("class").replace("origin-telemetry-user-avatar otkavatar otkavatar-", "");
        switch (avatarClassAttribute) {
            case "isonline":
                return PresenceType.ONLINE;
            case "isingame":
                return PresenceType.INGAME;
            case "isaway":
                return PresenceType.AWAY;
            case "isoffline":
                return PresenceType.OFFLINE;
            default:
                _log.debug("Unknown presence returned from avatar: " + avatarClassAttribute);
                return null;
        }
    }
}