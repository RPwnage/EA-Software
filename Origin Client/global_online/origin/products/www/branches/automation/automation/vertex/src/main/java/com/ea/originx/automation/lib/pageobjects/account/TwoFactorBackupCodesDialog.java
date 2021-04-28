package com.ea.originx.automation.lib.pageobjects.account;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represent the Two-Factor Authentication 'Backup Codes' dialog
 *
 * @author palui
 */
public final class TwoFactorBackupCodesDialog extends Dialog {

    protected static final String DIALOG_CSS = "#twofactor_backupcodes";
    protected static final By DIALOG_LOCATOR = By.cssSelector(DIALOG_CSS);

    protected static final By TITLE_LOCATOR = By.cssSelector(DIALOG_CSS + " .dialog_info .tfa-button-label");

    protected static final By CLOSE_CIRCLE_BUTTON_LOCATOR = By.cssSelector(DIALOG_CSS + " #closebtn_twofactor_backupcodes");    // Non-standard close circle button

    protected static final By CREATE_NEW_CODES_BUTTON_LOCATOR = By.cssSelector(DIALOG_CSS + " #generatebpkbtn");
    protected static final By CANCEL_BUTTON_LOCATOR = By.cssSelector(DIALOG_CSS + " #cancelbtn_twofactor_backupcodes");

    protected static final String EXPECTED_TITLE = "Backup Codes";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public TwoFactorBackupCodesDialog(WebDriver driver) {
        super(driver, DIALOG_LOCATOR, TITLE_LOCATOR, CANCEL_BUTTON_LOCATOR);
    }

    /**
     * Override isOpen to also check for matching title
     *
     * @return true if dialog is open with matching title, false otherwise
     */
    @Override
    public boolean isOpen() {
        return super.isOpen() && verifyTitleEqualsIgnoreCase(EXPECTED_TITLE);
    }

    /**
     * This dialog does not use the standard close circle button. Use the
     * dialog's close circle button instead
     */
    @Override
    public final void clickCloseCircle() {
        waitForElementClickable(CLOSE_CIRCLE_BUTTON_LOCATOR).click();
    }

    /**
     * Click 'Create New Codes' button
     */
    public void clickCreateNewCodesButton() {
        waitForElementClickable(CREATE_NEW_CODES_BUTTON_LOCATOR).click();
    }

    /**
     * Click 'Close' button
     */
    public void clickCloseButton() {
        close();
    }
}
