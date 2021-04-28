package com.ea.originx.automation.lib.pageobjects.dialog;

import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Cancel Membership' dialog
 *
 * @author cdeaconu
 */
public class CancelMembership extends Dialog{
    
    private static final String CANCEL_MEMBERSHIP_DIALOG_CSS = "origin-dialog-content-access-cancel";
    private static final By CANCEL_MEMBERSHIP_DIALOG_LOCATOR = By.cssSelector(CANCEL_MEMBERSHIP_DIALOG_CSS);
    private static final By CLOSE_LOCATOR = By.cssSelector(".otkmodal-close");
    private static final By CANCEL_MEMBERSHIP_BODY_TEXT_LOCATOR = By.cssSelector(CANCEL_MEMBERSHIP_DIALOG_CSS + " .otkmodal-body");
    private static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(CANCEL_MEMBERSHIP_DIALOG_CSS + " .otkbtn-light");
    private static final By CONTINUE_BUTTON_LOCATOR = By.cssSelector(CANCEL_MEMBERSHIP_DIALOG_CSS + " .otkbtn-primary");
    
    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public CancelMembership(WebDriver driver) {
        super(driver, CANCEL_MEMBERSHIP_DIALOG_LOCATOR, CLOSE_LOCATOR);
    }
    
    public static By getCancelMembershipBodyText() {
        return CANCEL_MEMBERSHIP_BODY_TEXT_LOCATOR;
    }
    
    /**
     * Verify the 'Cancel Membership' dialog exists.
     *
     * @return true if the dialog exists and visible, false
     * otherwise
     */
    public boolean isDialogVisible() {
        return waitIsElementVisible(CANCEL_MEMBERSHIP_DIALOG_LOCATOR);
    }

    /**
     * Clicks the 'Download' CTA
     */
    public void clickContinueCTA() {
        waitForElementClickable(waitForElementVisible(CONTINUE_BUTTON_LOCATOR)).click();
    }
    
    /**
     * Clicks the 'Close' CTA
     */
    public void clickCloseCTA() {
        waitForElementClickable(waitForElementVisible(CLOSE_BUTTON_LOCATOR)).click();
    }
    
}
