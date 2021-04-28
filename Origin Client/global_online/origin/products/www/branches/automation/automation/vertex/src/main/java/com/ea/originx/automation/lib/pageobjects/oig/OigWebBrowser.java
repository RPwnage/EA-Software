package com.ea.originx.automation.lib.pageobjects.oig;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.pageobjects.template.OAQtSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the web browser on the OIG.
 *
 * @author mkalaivanan
 */
public class OigWebBrowser extends OAQtSiteTemplate {

    protected static final By BACK_BUTTON_LOCATOR =  By.xpath("//*[@id='backButton']");
    protected static final By FORWARD_BUTTON_LOCATOR = By.xpath("//*[@id='forwardButton']");
    protected static final By URL_TEXT_LOCATOR = By.xpath("//*[@id='urlText']");
    protected static final By CLOSE_BUTTON_LOCATOR = By.xpath("//*[@id='buttonContainer']/*[@id='btnClose']");

    protected static final String DEFAULT_WEB_SITE_URL = "www.google.ca";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OigWebBrowser(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the web browser is open by checking for the visibility
     * of the 'Back' button, 'Forward' button, and the website URL.
     *
     * @return true if 'Back' button, 'Forward' button, and the
     * website URL are present, false otherwise
     */
    public boolean verifyOigWebBrowserOpen() {
        EAXVxSiteTemplate.switchToOigWebBrowserWindow(driver);
        return waitForElementVisible(URL_TEXT_LOCATOR, 20).getText().contains(DEFAULT_WEB_SITE_URL);
    }

    /**
     * Close the web browser window.
     */
    public void closeWebBrowser() {
        EAXVxSiteTemplate.switchToOigWebBrowserWindow(driver);
        clickOnElement(waitForElementVisible(CLOSE_BUTTON_LOCATOR, 10));
    }
}