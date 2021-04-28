package com.ea.originx.automation.lib.pageobjects.common;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Page object that represents Take Over page
 *
 * @author mkalaivanan
 */
public class TakeOverPage extends EAXVxSiteTemplate {

    protected final By TAKEOVER_WRAPPER = By.cssSelector(".l-origin-store-takeover-wrapper");
    protected final By CONTINUE_TO_BUTTON_LOCATOR = By.cssSelector(" .origin-store-takeover-cta .otkbtn[ng-click*='dismiss']");

    public TakeOverPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Click on Continue To Button
     */
    public void clickOnContinueToButton() {
        waitForElementClickable(CONTINUE_TO_BUTTON_LOCATOR).click();
    }

    /**
     * Verify 'Continue to' button exists
     *
     * @return true if continue to button locator exists else returns false
     */
    public boolean verifyContinueButtonVisible() {
        return waitIsElementVisible(CONTINUE_TO_BUTTON_LOCATOR);
    }

    /**
     * Scroll to the Continue button
     */
    public void scrollToContinueToButton() {
        scrollToElement(waitForElementVisible(CONTINUE_TO_BUTTON_LOCATOR));
    }

    /**
     * Checks if the takeover is present
     *
     * @return True if the takeover is present, false otherwise
     */
    public boolean isVisible() {
        return waitIsElementVisible(TAKEOVER_WRAPPER, 2);
    }

    /**
     * Close the TakeOver page if open (does nothing otherwise)
     */
    public void closeIfOpen() {
        if (isVisible()) {
            scrollToContinueToButton();
            clickOnContinueToButton();
        }
    }
}
