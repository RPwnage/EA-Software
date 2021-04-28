package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the 'Message of the Day' header on the main store page.
 *
 * @author rmcneil
 */
public class MessageOftheDay extends EAXVxSiteTemplate {

    protected static final By MOTD_HEADER_LOCATOR = By.cssSelector("div#storemod");
    protected static final By MOTD_INFO_LOCATOR = By.cssSelector("div.origin-store-sitestripe-messageoftheday");
    protected static final By MOTD_CLOSE_BUTTON_LOCATOR = By.cssSelector("div > i.origin-store-sitestripe-messageoftheday-close");
    protected static final By MOTD_CONTAINER_LOCATOR = By.cssSelector("origin-store-online div.origin-store-sitestripe-wrapper > div.origin-store-sitestripe-messageoftheday");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public MessageOftheDay(WebDriver driver) {
        super(driver);
    }

    /**
     * Clicks on the 'Close' button on the 'Message of the Day'.
     */
    public void closeMOTD() {
        waitForElementClickable(MOTD_CLOSE_BUTTON_LOCATOR).click();
    }

    /**
     * Verifies only one 'Message of the Day' is shown.
     *
     * @return true if only one 'Message of the Day' is shown, false otherwise
     */
    public boolean verifyOnlyOneMOTD() {
        List<WebElement> messageHeaders = driver.findElements(MOTD_HEADER_LOCATOR);
        return messageHeaders.size() == 1;
    }

    /**
     * Verifies the 'Message of the Day' is visible.
     *
     * @return true if the 'Message of the Day' is visible, false otherwise
     */
    public boolean verifyMOTDVisible() {
        return waitIsElementVisible(MOTD_INFO_LOCATOR);
    }

    /**
     * Verifies the 'Message of the Day' is inside the main store container
     * element.
     *
     * @return true if the 'Message of the Day' is inside the main store container element,
     * false otherwise
     */
    public boolean verifyMOTDInStoreContainer() {
        return waitIsElementVisible(MOTD_CONTAINER_LOCATOR);
    }
}