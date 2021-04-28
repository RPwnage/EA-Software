package com.ea.originx.automation.lib.pageobjects.dialog;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Welcome back' dialog
 *
 * @author cdeaconu
 */
public class WelcomeBackDialog extends Dialog{
    
    private static final String WELCOME_BACK_DIALOG_CSS = "origin-dialog-content-access-renew";
    private static final By TITLE_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-header .otkmodal-title");
    private static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(WELCOME_BACK_DIALOG_CSS + " .otkbtn-primary");
    
    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public WelcomeBackDialog(WebDriver driver) {
        super(driver, TITLE_LOCATOR, TITLE_LOCATOR);
    }
    
    /**
     * Clicks the 'Close' CTA
     */
    public void clickCloseCTA() {
        waitForElementClickable(waitForElementVisible(CLOSE_BUTTON_LOCATOR)).click();
    }
}
