package com.ea.originx.automation.lib.pageobjects.settings;

import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object representing the 'Settings - Notifications' page.
 *
 * @author palui
 */
public class NotificationsSettings extends SettingsPage {

    private static final String TOGGLE_SLIDES_CSS = PAGE_SECTIONS_CSS
            + ".active-section ORIGIN-SETTINGS-NOTIFICATIONS SECTION#notifications div.origin-settings-details div.otktoggle.origin-settings-toggle";
    private static final By TOGGLE_SLIDES_LOCATOR = By.cssSelector(TOGGLE_SLIDES_CSS);

    /**
     * Enum for notification toggle slide types (representing rows in the
     * 'Settings - Notifications' page).
     */
    public enum NotificationType {
        INCOMING_CHAT_MESSAGE(0), INCOMING_VOICE_CHAT(1), GAME_INVITATIONS(2), GROUP_CHAT_INVITATION(3),
        FRIEND_SIGNS_IN(4), FRIEND_SIGNS_OUT(5), FRIEND_STARTS_GAME(6), FRIEND_QUITS_GAME(7), FRIEND_STARTS_BROADCASTING(8), FRINED_STOPS_BROADCASTING(9),
        ACHIEVEMENT_UNLOCKS(10), DOWNLOAD_COMPLETES(11), DOWNLOAD_FAILS(12), INSTALLATION_COMPLETES(13);
        private final int index;

        private NotificationType(int index) {
            this.index = index;
        }
    };

    /**
     * Enum for notification toggle slide action types (representing columns in
     * the 'Settings - Notifications' page). Note index is from right to left
     * (i.e. SOUND(0) first which actually appears to the right of
     * NOTIFCATION(1)).
     */
    public enum ActionType {
        SOUND(0), NOTIFICATION(1);
        private final int index;

        private ActionType(int index) {
            this.index = index;
        }
    };

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public NotificationsSettings(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify 'Settings - Notifications' page is displayed.
     *
     * @return true if 'Settings - Notifications' is displayed, false otherwise
     */
    public boolean verifyNotificationsSettingsReached() {
        return getActiveSettingsPageType() == PageType.NOTIFICATIONS;
    }

    /**
     * Click a toggle slide on the 'Settings - Notifications' page which currently
     * as 28 entries in 14 rows and 2 columns. A positional scheme is used.
     *
     * @param type Toggle slide type (toggle slide rows on the page)
     * @param action Toggle slide action type (toggle slide columns on the page)
     */
    public void clickToggleSlide(NotificationType type, ActionType action) {
        List<WebElement> toggleSlides = driver.findElements(TOGGLE_SLIDES_LOCATOR);
        int index = type.ordinal() * 2 + action.ordinal();
        toggleSlides.get(index).click();
    }

    /**
     * Toggle all toggle slides in this settings page and verify 'Notifcation
     * Toast' flashes each time.
     *
     * @return true if and only if every toggle results in a 'Notification
     * Toast' flash, false otherwise
     */
    public boolean toggleNotificationsSettingsAndVerifyNotifications() {
        boolean notificationsFlashed = true;
        List<WebElement> toggleSlides = driver.findElements(TOGGLE_SLIDES_LOCATOR);
        // On low resolution (our test machines), SocialHubMinimized covered slide toggles can no longer be clicked.
        // Solution is to scroll to the latest element assuming all toggles fit into at most 3 pages. Do this when
        // completing about a third of the toggles
        int aThird = toggleSlides.size() / 3;
        int count = 0;
        for (WebElement toggle : toggleSlides) {
            toggle.click();
            notificationsFlashed &= verifyNotificationToastFlashed("Notifications - toggle #" + (count + 1));
            if (!notificationsFlashed) {
                return false;
            }
            // Scroll to element after each 1/3 completed
            if (++count % aThird == 0) {
                scrollToElement(toggle);
                sleep(3000); // pause for page to stablize
            }
        }
        return true;
    }
    
    /**
     * Verify that the 'Chat' section is visible.
     * 
     * @return true if the 'Chat' section is visible, false otherwise
     */
    public boolean verifyChatSectionVisible() {
        return verifySectionVisible("Chat");
    }
    
    /**
     * Verify that the 'Friends Activity' section is visible.
     * 
     * @return true if the 'Friends Activity' section is visible, false otherwise
     */
    public boolean verifyFriendsActivitySectionVisible() {
        return verifySectionVisible("Friends activity");
    }
}