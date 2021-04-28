package com.ea.originx.automation.lib.pageobjects.oig;

import com.ea.originx.automation.lib.pageobjects.template.OAQtSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Class for controlling OIG windows that appear when clicking on items in the
 * OIG Navigation
 *
 * @author sbentley
 */
public class OIGWindow extends OAQtSiteTemplate {

    // Close button for Profile Header Inside Oig
    protected static final By CLOSE_BUTTON_LOCATOR = By.xpath("//*[@id='buttonContainer']/*[@id='btnClose']");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OIGWindow(WebDriver driver) {
        super(driver);
    }

    /**
     * Switches to OIG Window frame. Used with page objects to verify/manipulate
     * elements while in OIG
     */
    public void switchToOIGWindow() {
        Waits.waitForPageThatMatches(driver, OriginClientData.OIG_PAGE_URL_REGEX, 8);
    }

    /**
     * Click the 'X' button to close the OIG window.
     */
    public void closeOIGWindow() {
        waitForElementClickable(CLOSE_BUTTON_LOCATOR).click();
    }
}
