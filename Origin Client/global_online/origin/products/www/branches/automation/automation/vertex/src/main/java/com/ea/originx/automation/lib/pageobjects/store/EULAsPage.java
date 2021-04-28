package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'End User License Agreement' page on a browser.<br>
 * NOTE: The content of this page is accessible as a new tab on browser tab
 * only. If the EULAsPage is opened from the client, the client driver cannot
 * access it.
 *
 * @author palui
 */
public class EULAsPage extends EAXVxSiteTemplate {

    protected static final String EULAS_PAGE_TITLE_CSS = "#content .product-eulas #mod-article > .mod-header > h1";
    protected static By EULAS_PAGE_TITLE_LOCATOR = By.cssSelector(EULAS_PAGE_TITLE_CSS);
    protected String EULAS_PAGE_TITLE = "Eulas and Other Disclosures";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public EULAsPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Get EULAs page title.
     *
     * @return EULAs page title String
     */
    public String getTitle() {
        return waitForElementVisible(EULAS_PAGE_TITLE_LOCATOR).getText();
    }

    /**
     * Verify EULAs page reached.
     *
     * @return true if title is visible and is as expected, false otherwise
     */
    public boolean verifyEULAsPageReached() {
        return StringHelper.containsIgnoreCase(getTitle(), EULAS_PAGE_TITLE);
    }
}