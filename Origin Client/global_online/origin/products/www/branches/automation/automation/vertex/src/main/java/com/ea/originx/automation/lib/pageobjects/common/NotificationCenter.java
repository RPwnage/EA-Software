package com.ea.originx.automation.lib.pageobjects.common;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.util.List;
import java.util.stream.Collectors;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * The 'Notification Center' that is located in the Navigate Side Bar
 *
 * @author sbentley
 * @author mdobre
 */
public class NotificationCenter extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(NotificationCenter.class);

    protected static final By NOTIFICATIONS_LOCATOR = By.cssSelector("#origin-content .origin-navigation-tertiary .origin-inbox-mini-header");
    protected static final By NOTIFICATIONS_GIFT_LOCATOR = By.cssSelector("#origin-content .origin-navigation-tertiary .origin-inbox-flyout .origin-inbox-item-title");
    protected static final By NOTIFICATIONS_ITEMS_LOCATOR = By.cssSelector("#origin-content .origin-navigation-tertiary .origin-inbox-flyout .origin-inbox-item");
    protected static final String NOTIFICATION_INBOX_ITEM = ".origin-inbox-item-title"; // used to get all gift notifications

    /**
     * Constructor
     *
     * @param driver
     */
    public NotificationCenter(WebDriver driver) {
        super(driver);
    }

    /**
     * Hover on the Notifications link in the side nav
     */
    public void expandNotificationCenter() {
        NavigationSidebar navigationSideBar = new NavigationSidebar(driver);
        if (!navigationSideBar.isSidebarOpen()) {
            navigationSideBar.openSidebar();
        }

        if (!isMobile()) {
            hoverOnElement(waitForElementVisible(NOTIFICATIONS_LOCATOR));
        } else {
            clickOnElement(waitForElementClickable(NOTIFICATIONS_LOCATOR));
        }
        
    }

    /**
     * Click the first 'You Got a Gift' from Notification sub-menu
     */
    public void clickYouGotGiftNotification() {
        expandNotificationCenter();
        waitForElementVisible(NOTIFICATIONS_GIFT_LOCATOR).click();
    }

    /**
     * Verify that Notifications have at least one pending gift
     *
     * @return true if there is a pending gift, false otherwise
     */
    public boolean verifyGiftReceivedVisible() {
        expandNotificationCenter();
        return waitIsElementVisible(NOTIFICATIONS_GIFT_LOCATOR);
    }

    /**
     * Verify that Notifications have a pending gift.
     *
     * @param senderName First and the last name (e.g. 'Jane Doe') of the person who sent you the gift.
     * @return true if the gift associated with the given name is found,
     * false otherwise.
     */
    public boolean verifyGiftReceivedVisible(String senderName) {
        expandNotificationCenter();
        List<WebElement> giftItems = getGiftNotificationItems();
        if (giftItems == null || giftItems.isEmpty()) {
            _log.debug("Unable to find gift items");
            return false;
        }

        return giftItems.stream().map((giftItem) -> giftItem.getText()).anyMatch((description) -> (description != null && description.contains(senderName)));
    }

    /**
     * Clicks on an unopened gift in 'Notification Center'.
     *
     * @param senderName  First and the last name (e.g. 'Jane Doe') of the person who sent you the gift.
     */
    public void clickOnGiftNotification(String senderName) {
        expandNotificationCenter();
        List<WebElement> giftItems = getGiftNotificationItems();
        if (giftItems == null || giftItems.isEmpty()) {
            _log.debug("Unable to find gift items");
            return;
        }

        WebElement giftNotification = giftItems.stream().filter(giftItem -> giftItem.getText() != null && giftItem.getText().contains(senderName)).findFirst().get();

        if (giftNotification == null) {
            _log.debug("Unable to find gifted item in Notification Center");
            return;
        }

        waitForElementClickable(giftNotification).click();
    }

    private List<WebElement> getNotificationItems() {
        return waitForAllElementsVisible(NOTIFICATIONS_ITEMS_LOCATOR);
    }

    private List<WebElement> getGiftNotificationItems() {
        List<WebElement> notificationItems = getNotificationItems();
        if (notificationItems.isEmpty()) {
            _log.debug("Unable to find notification items");
            return null;
        }

        return notificationItems.stream().filter(item -> {
            return waitIsChildElementVisible(item, By.cssSelector(NOTIFICATION_INBOX_ITEM), 10);
        }).collect(Collectors.toList());
    }
}
