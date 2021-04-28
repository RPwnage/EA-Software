package com.ea.originx.automation.lib.pageobjects.profile;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object for the 'Profile Offline' page.
 *
 * @author palui
 */
public class ProfileOfflinePage extends EAXVxSiteTemplate {

    protected static final By PROFILE_OFFLINE_PAGE_LOCATOR = By.cssSelector("origin-profile-offline .origin-offline-cta");
    protected static final By PROFILE_OFFLINE_HEADER_LOCATOR = By.cssSelector("origin-profile-offline .origin-offline-cta .origin-message-title");
    protected static final By GO_ONLINE_BUTTON_LOCATOR = By.cssSelector("origin-profile-offline .origin-offline-cta .origin-offline-reconnectbutton-button");

    protected static final String PROFILE_OFFLINE_HEADER = "offline";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ProfileOfflinePage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verifies the Origin 'Profile Offline' page is visible (displayed and has
     * size > 0).
     *
     * @return true if displayed, false otherwise
     */
    public boolean verifyOnOfflinePage() {
        return waitIsElementVisible(PROFILE_OFFLINE_PAGE_LOCATOR);
    }

    /**
     * Verifies the Origin 'Profile Offline' header matches the expected text.
     *
     * @return true if matches, false otherwise
     */
    public boolean verifyOfflineHeader() {
        WebElement header = waitForElementVisible(PROFILE_OFFLINE_HEADER_LOCATOR);
        return StringHelper.containsIgnoreCase(header.getText(), PROFILE_OFFLINE_HEADER);
    }

    /**
     * Click the 'Go Online' button on the 'Profile Offline' page.
     */
    public void clickGoOnline() {
        waitForElementVisible(GO_ONLINE_BUTTON_LOCATOR).click();
    }
}