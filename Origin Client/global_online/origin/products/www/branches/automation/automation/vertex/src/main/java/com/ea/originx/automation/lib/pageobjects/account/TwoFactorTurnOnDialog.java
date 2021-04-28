package com.ea.originx.automation.lib.pageobjects.account;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represent the Two-Factor Authentication 'Turn on Login Verification' dialog
 * that appears by clicking Login Verification 'Turn On' button at the
 * 'Security' section of the 'Account Settings' page
 *
 * @author palui
 */
public final class TwoFactorTurnOnDialog extends Dialog {

    protected static final String DIALOG_CSS = "#twofactor_turnon #formTwofactor";
    protected static final By DIALOG_LOCATOR = By.cssSelector(DIALOG_CSS);

    protected static final By TITLE_LOCATOR = By.cssSelector(DIALOG_CSS + " .dialog_info .tfa-button-label");

    // Click on the labels to select radio buttons
    protected static final By EMAIL_RADIO_LOCATOR = By.cssSelector(DIALOG_CSS + " #emailradiolabel");
    protected static final By APP_AUTHENTICATOR_RADIO_LOCATOR = By.cssSelector(DIALOG_CSS + " #appradiolabel");
    protected static final By TEXT_MESSAGE_RADIO_LOCATOR = By.cssSelector(DIALOG_CSS + " #smsradiolabel");

    protected static final By CLOSE_CIRCLE_BUTTON_LOCATOR = By.cssSelector(DIALOG_CSS + " #closebtn_twofactor_turnon");    // Non-standard close circle button

    protected static final By CONTINUE_BUTTON_LOCATOR = By.cssSelector(DIALOG_CSS + " #savebtn_twofactor_turnon");
    protected static final By CANCEL_BUTTON_LOCATOR = By.cssSelector(DIALOG_CSS + " #cancelbtn_twofactor_turnon");

    protected static final String EXPECTED_TITLE = "Turn On Login Verification";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public TwoFactorTurnOnDialog(WebDriver driver) {
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
     * Click "Email" radio button
     */
    public void clickEmailRadioButton() {
        waitForElementClickable(EMAIL_RADIO_LOCATOR).click();
    }

    /**
     * Click "App Authenticator" radio button
     */
    public void clickAppAuthenticatorRadioButton() {
        waitForElementClickable(APP_AUTHENTICATOR_RADIO_LOCATOR).click();
    }

    /**
     * Click "Text Message" radio button
     */
    public void clickTextMessageRadioButton() {
        waitForElementClickable(TEXT_MESSAGE_RADIO_LOCATOR).click();
    }

    /**
     * Check if a radio button is selected. If selected, its label will be
     * something like "radiostyle_1_checked"
     *
     * @param buttonLabelLocator label of the radio button
     *
     * @return true if selected, false otherwise
     */
    public boolean isRadioButtonSelected(By buttonLabelLocator) {
        String labelClass = waitForElementVisible(buttonLabelLocator).getAttribute("class");
        return labelClass.contains("checked");
    }

    /**
     * Check if the "Email" radio button is selected
     *
     * @return true if selected, false otherwise
     */
    public boolean isEmailRadioButtonSelected() {
        return isRadioButtonSelected(EMAIL_RADIO_LOCATOR);
    }

    /**
     * Check if the "App Authenticator" radio button is selected
     *
     * @return true if selected, false otherwise
     */
    public boolean isAppAuthenticatorRadioButtonSelected() {
        return isRadioButtonSelected(APP_AUTHENTICATOR_RADIO_LOCATOR);
    }

    /**
     * Check if the "Text Message" radio button is selected
     *
     * @return true if selected, false otherwise
     */
    public boolean isTextMessageRadioButtonSelected() {
        return isRadioButtonSelected(TEXT_MESSAGE_RADIO_LOCATOR);
    }

    /**
     * Click 'Continue' button
     */
    public void clickContinueButton() {
        waitForElementClickable(CONTINUE_BUTTON_LOCATOR).click();
    }

    /**
     * Click 'Cancel' button
     */
    public void clickCancelButton() {
        close();
    }
}
