package com.ea.originx.automation.lib.pageobjects.account;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represent the Two-Factor Authentication 'Verify Code' dialog
 *
 * @author palui
 */
public final class TwoFactorVerifyCodeDialog extends Dialog {

    protected static final String DIALOG_CSS = "#twofactor_verifycode #formTwofactorVerify";
    protected static final By DIALOG_LOCATOR = By.cssSelector(DIALOG_CSS);

    protected static final By TITLE_LOCATOR = By.cssSelector(DIALOG_CSS + " .dialog_info .tfa-button-label");

    protected static final By CLOSE_CIRCLE_BUTTON_LOCATOR = By.cssSelector(DIALOG_CSS + " #closebtn_twofactor_verifycode");    // Non-standard close circle button

    protected static final By MESSAGE_LOCATOR = By.cssSelector(DIALOG_CSS + " #sendMsgEmail");
    protected static final By CODE_INPUT_LOCATOR = By.cssSelector(DIALOG_CSS + " #twofactorCode");
    protected static final By SEND_CODE_LINK_LOCATOR = By.cssSelector(DIALOG_CSS + " #sendCodeLink");
    protected static final By TURN_ON_BUTTON_LOCATOR = By.cssSelector(DIALOG_CSS + " #savebtn_twofactor_verifycode");
    protected static final By CANCEL_BUTTON_LOCATOR = By.cssSelector(DIALOG_CSS + " #cancelbtn_twofactor_verifycode");

    protected static final String EXPECTED_TITLE = "Turn On Login Verification";

    protected static final String[] EXPECTED_MESSAGE_KEYWORDS = {"sent", "security code", "your email"};

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public TwoFactorVerifyCodeDialog(WebDriver driver) {
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
     * Get dialog message displayed after the dialog title
     *
     * @return dialog message
     */
    public String getMessage() {
        return waitForElementVisible(MESSAGE_LOCATOR).getText();
    }

    /**
     * Verify if message that email has been sent to user's email matches what
     * is expected
     *
     * @param userEmail user's email that forms part of the dialog message
     *
     * @return true if matches, false otherwise
     */
    public boolean VerifyMessageForUserEmail(String userEmail) {
        String message = getMessage();
        boolean messageKeywordsMatch = StringHelper.containsIgnoreCase(message, EXPECTED_MESSAGE_KEYWORDS);
        boolean messageUserEmailMatch = StringHelper.containsIgnoreCase(message, userEmail);
        return messageKeywordsMatch && messageUserEmailMatch;
    }

    /**
     * Enter security code to the input text box
     *
     * @param securityCode string to enter
     */
    public void enterSecurityCode(String securityCode) {
        waitForElementVisible(CODE_INPUT_LOCATOR).sendKeys(securityCode);
    }

    /**
     * Click 'Resend My Security Code' name
     */
    public void clickResendLink() {
        waitForElementClickable(SEND_CODE_LINK_LOCATOR).click();
    }

    /**
     * Click 'Turn On Login Verification' button
     */
    public void clickTurnOnButton() {
        waitForElementClickable(TURN_ON_BUTTON_LOCATOR).click();
    }

    /**
     * Click 'Cancel' button
     */
    public void clickCancelButton() {
        close();
    }
}
