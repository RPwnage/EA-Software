package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Your membership has been updated' dialog
 * 
 * @author mdobre
 */
public class UpdatedMembershipConfirmationDialog extends Dialog{
    
    private static final String UPDATE_MEMBERSHIP_CONFIRMATION_CSS = "origin-dialog-content-access-change-confirmation";
    private static final By UPDATE_MEMBERSHIP_CONFIRMATION_LOCATOR = By.cssSelector(UPDATE_MEMBERSHIP_CONFIRMATION_CSS);
    private static final By CLOSE_LOCATOR = By.cssSelector(".otkmodal-close");
    private static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(UPDATE_MEMBERSHIP_CONFIRMATION_CSS + " .otkbtn-primary");
    
    /**
     * Constructor
     * 
     * @param driver Selenium WebDriver
     */
    public UpdatedMembershipConfirmationDialog(WebDriver driver) {
        super(driver, UPDATE_MEMBERSHIP_CONFIRMATION_LOCATOR, CLOSE_LOCATOR);
    }

    /**
     * Clicks the 'Close' CTA
     */
    public void clickCloseCTA() {
        waitForElementClickable(waitForElementVisible(CLOSE_BUTTON_LOCATOR)).click();
    }   
}