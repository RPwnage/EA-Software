package com.ea.originx.automation.lib.pageobjects.originaccess;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Product Landing' page header of a pre-order game.
 *
 * @author caleung
 */
public class ProductLandingPageHeader extends EAXVxSiteTemplate {

    protected static final String PRODUCT_LANDING_PAGE_HEADER_CSS = "#storecontent origin-store-access-productlanding-header .origin-store-access-productlanding-header";
    protected static final By PRODUCT_LANDING_PAGE_HEADER_LOCATOR = By.cssSelector(PRODUCT_LANDING_PAGE_HEADER_CSS);
    protected static final By PRODUCT_LANDING_PAGE_HEADER_TITLE_LOCATOR = By.cssSelector(PRODUCT_LANDING_PAGE_HEADER_CSS + " .origin-store-access-productlanding-header-title");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ProductLandingPageHeader(WebDriver driver) {
        super(driver);
    }

    /**
     * Wait for the header of product landing page to load.
     */
    public void waitForProductLandingPageHeaderToLoad() {
        waitForPageToLoad();
        waitForAngularHttpComplete(20);
        waitForElementVisible(By.cssSelector("#footer"));
        waitForAnimationComplete(By.cssSelector("#footer"));
    }

    /**
     * Verify product landing page reached.
     *
     * @return true if the product landing page is present on the page, false otherwise
     */
    public boolean verifyProductLandingPageReached() {
        return waitIsElementPresent(PRODUCT_LANDING_PAGE_HEADER_LOCATOR);
    }

    /**
     * Verify title on header contains entitlement name.
     *
     * @param name Entitlement name
     * @return true if header title contains game name, false otherwise
     */
    public boolean verifyProductLandingPageHeaderTitle(String name) {
        return StringHelper.containsIgnoreCase(getProductLandingPageHeaderText(), name);
    }

    /**
     * Gets the title of the header.
     *
     * @return Title of the header
     */
    public String getProductLandingPageHeaderText() {
        return driver.findElement(PRODUCT_LANDING_PAGE_HEADER_TITLE_LOCATOR).getText();
    }
}