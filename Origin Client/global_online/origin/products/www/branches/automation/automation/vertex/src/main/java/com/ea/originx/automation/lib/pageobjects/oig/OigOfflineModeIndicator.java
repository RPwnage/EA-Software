package com.ea.originx.automation.lib.pageobjects.oig;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.pageobjects.template.OAQtSiteTemplate;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents 'Offline Mode Indicator' inside the OIG.
 *
 * @author palui
 */
public class OigOfflineModeIndicator extends OAQtSiteTemplate {

    protected static final By OFFLINE_MODE_BUTTON_LOCATOR = By.xpath("//*[@id='btnOfflineMode']");
    protected static final String OFFLINE_MODE_BUTTON_TEXT = "OFFLINE MODE";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OigOfflineModeIndicator(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the OIG 'Offline Mode' button label matches the expected text 'OFFLINE MODE'.
     *
     * @return true if button text matches, false otherwise
     */
    public boolean verifyOigOfflineModeButtonText() {
        WebElement offlineModeButton;
        try {
            EAXVxSiteTemplate.switchToOigOfflineModeIndicator(driver);
            offlineModeButton = waitForElementVisible(OFFLINE_MODE_BUTTON_LOCATOR, 10);
        } catch (Exception e) {
            _log.warn(String.format("Exception: Cannot locate OIG 'Offline Mode' button - %s", e.getMessage()));
            return false;
        }
        return offlineModeButton.getText().equals(OFFLINE_MODE_BUTTON_TEXT); // check exact match
    }

    /**
     * Switch to the OIG 'Offline Mode Indicator' and click the 'OFFLINE MODE'
     * link button.
     */
    public void click() {
        try {
            EAXVxSiteTemplate.switchToOigOfflineModeIndicator(driver);
            waitForElementClickable(OFFLINE_MODE_BUTTON_LOCATOR).click();
        } catch (Exception e) {
            _log.warn(String.format("Exception: Cannot click OIG 'Offline Mode Indicator' button - %s", e.getMessage()));
        }
    }
}