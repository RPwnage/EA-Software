package com.ea.originx.automation.lib.pageobjects.common;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the 'Offline Flyout' which pops up (under the
 * 'Offline Mode' button at the top right corner of the menu bar) after going
 * offline.
 *
 * @author palui
 */
public class OfflineFlyout extends EAXVxSiteTemplate {

    protected static final By TITLE_LOCATOR = By.cssSelector("#offline-flyout .origin-offline-flyout h2.otktitle-2.origin-message-title");
    protected static final By MESSAGE_LOCATOR = By.cssSelector("#offline-flyout .origin-offline-flyout p.otktitle-3.origin-message-content");

    protected static final By GO_ONLINE_BUTTON_LOCATOR = By.cssSelector("#offline-flyout .origin-offline-reconnectbutton-button");

    protected static final String OFFLINE_FLYOUT_TITLE = "You're offline";
    protected static final String OFFLINE_FLYOUT_MESSAGE
            = "Origin is in offline mode. To get access to all Origin features, please go online.";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver for the client
     */
    public OfflineFlyout(WebDriver driver) {
        super(driver);
    }

    /**
     * Closes the offline flyout if it is open. Does nothing if it is not
     * already open.
     */
    public void closeOfflineFlyout() {
        try {
            //Clicking the offline flyout will close it
            waitForElementClickable(MESSAGE_LOCATOR, 5).click();
        } catch (TimeoutException e) {
            //Do nothing because the offline flyout is already closed
        }
    }

    /**
     * Verify 'Offline Flyout' title matches the expected title.
     *
     * @return true if title matches, false otherwise
     */
    public boolean verifyOfflineFlyoutTitle() {
        WebElement title;
        try {
            title = waitForElementVisible(TITLE_LOCATOR);
        } catch (Exception e) {
            _log.warn(String.format("Exception: Cannot locate 'Offline Flyout' title - %s", e.getMessage()));
            return false;
        }
        return title.getText().equals(OFFLINE_FLYOUT_TITLE);
    }

    /**
     * Verify 'Offline Flyout' is visible.
     *
     * @return true if flyout is visible, false otherwise
     */
    public boolean verifyOfflineFlyoutVisible() {
        return waitIsElementVisible(TITLE_LOCATOR, 2);
    }

    /**
     * Verify 'Offline Flyout' message contains the expected message.
     *
     * @return true if message contains the expected message, false otherwise
     */
    public boolean verifyOfflineFlyoutMessage() {
        WebElement message;
        try {
            message = waitForElementVisible(MESSAGE_LOCATOR);
        } catch (Exception e) {
            _log.warn(String.format("Exception: Cannot locate 'Offline Flyout' message - %s", e.getMessage()));
            return false;
        }
        return message.getText().contains(OFFLINE_FLYOUT_MESSAGE);
    }

    /**
     * Click 'Go Online' button at the 'Offline Flyout' to go online.
     *
     * @return true if button can be and has been clicked, false otherwise
     */
    public boolean clickGoOnlineButton() {
        try {
            waitForElementClickable(GO_ONLINE_BUTTON_LOCATOR).click();
        } catch (Exception e) {
            _log.warn(String.format("Exception: Cannot click 'Go Online' button - %s", e.getMessage()));
            return false;
        }
        return true;
    }
}