package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'About Store' page.
 *
 * @author mkalaivanan
 */
public class AboutStorePage extends EAXVxSiteTemplate {

    public static final By ABOUT_STORE_NAVIGATION_LOCATOR = By.cssSelector("origin-store-about-hero .origin-about-hero .origin-about-hero-header-wrapper .origin-about-nav .origin-about-nav-item");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public AboutStorePage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'About Store' page reached.
     *
     * @return true if 'About Store' page reached, false otherwise
     */
    public boolean verifyAboutStorePageReached() {
        return waitForAllElementsVisible(ABOUT_STORE_NAVIGATION_LOCATOR).size() > 0;
    }
}