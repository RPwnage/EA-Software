package com.ea.originx.automation.lib.pageobjects.login;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Reset Password' page.
 *
 * @author mkalaivanan
 */
public class ResetPasswordPage extends EAXVxSiteTemplate {

    protected static final String RESET_PASSWORD_CSS = "#panel-reset-password";
    protected static final String RESET_PASSWORD_CONTENT_CSS = RESET_PASSWORD_CSS + " .panel-contents";
    protected static final By PASSWORD_LOCATOR = By.cssSelector(RESET_PASSWORD_CSS + " #origin-reset-password-password-container #password");
    protected static final By CONFIRM_PASSWORD_LOCATOR = By.cssSelector(RESET_PASSWORD_CSS + " #origin-reset-password-confirm-password-container #confirmPassword");
    protected static final By SUBMIT_BUTTON_LOCATOR = By.cssSelector(RESET_PASSWORD_CSS + " .panel-action-area .origin-ux-button-primary");
    protected static final By PASSWORD_CHANGED_MESSAGE_LOCATOR = By.cssSelector("#panel-reset-password-success .panel-content > p");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ResetPasswordPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'Reset Password' page has been reached.
     *
     * @return true if the 'Password' field is visible, false otherwise
     */
    public boolean verifyResetPasswordPageReached() {
        return waitIsElementVisible(PASSWORD_LOCATOR);
    }

    /**
     * Reset password.
     *
     * @param newPassword New password to be reset to
     */
    public void resetPassword(String newPassword) {
        enterPassword(newPassword);
        enterConfirmPassword(newPassword);
    }

    /**
     * Enter the new password.
     *
     * @param newPassword The new password to be reset to
     */
    public void enterPassword(String newPassword) {
        waitForElementVisible(PASSWORD_LOCATOR).sendKeys(newPassword);
    }

    /**
     * Confirm the new password.
     *
     * @param newPassword The new pass word to be reset to
     */
    public void enterConfirmPassword(String newPassword) {
        waitForElementVisible(CONFIRM_PASSWORD_LOCATOR).sendKeys(newPassword);
    }

    /**
     * Click the 'Submit' button.
     */
    public void clickSubmitButton() {
        waitForElementClickable(SUBMIT_BUTTON_LOCATOR).click();
    }


    /**
     * Verify if the 'Submit' button is enabled.
     *
     * @return true if enabled, false otherwise
     */
    public boolean isSubmitButtonEnabled() {
        return isElementEnabled(SUBMIT_BUTTON_LOCATOR);
    }

    /**
     * Verify a 'Password changed' message is visible.
     *
     * @return true if password changed message is visible, false otherwise
     */
    public boolean verifyPasswordChangedMessageVisible() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(PASSWORD_CHANGED_MESSAGE_LOCATOR).getText(), "password", "changed");
    }
}