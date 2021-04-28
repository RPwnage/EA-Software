package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.vx.core.helpers.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the 'Activation Required' dialog, which pops up launching an
 * un-entitled game outside of Origin.
 *
 * @author palui
 */
public final class ActivationRequiredDialog extends Dialog {

    protected static final By MESSAGE_LOCATOR = By.cssSelector("#dialog #otkmodal .otkmodal-content .otkmodal-body div.otkc[ng-show='description']");

    protected static final String COMMAND_BUTTONS_LOCATOR = "origin-dialog-content-commandbuttons";
    protected static final By NOT_USER_BUTTON_LOCATOR = By.cssSelector(COMMAND_BUTTONS_LOCATOR + "> div > .otkbtn-command:nth-of-type(2)");
    protected static final By ACTIVATE_GAME_BUTTON_LOCATOR = By.cssSelector(COMMAND_BUTTONS_LOCATOR + "> div > .otkbtn-command:nth-of-type(1)");

    protected static final By CANCEL_BUTTON_LOCATOR = By.xpath("//BUTTON[contains(@class,'otkbtn') and text()='Cancel']");

    protected static final String EXPECTED_TITLE = "ACTIVATION REQUIRED";
    protected static final String AR_DIALOG_MESSAGE_TMPL = "To play %s, you must first activate it on your EA account.";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ActivationRequiredDialog(WebDriver driver) {
        super(driver, null, null, CANCEL_BUTTON_LOCATOR);
    }

    /**
     * Override isOpen to also check for matching title.
     *
     * @return true if dialog is open with matching title, false otherwise
     */
    @Override
    public boolean isOpen() {
        return super.isOpen() && verifyTitleEqualsIgnoreCase(EXPECTED_TITLE);
    }

    /**
     * Verify dialog title matches the expected 'Activation Required' dialog title.
     *
     * @return true if title matches, false otherwise
     */
    public boolean verifyActivationRequiredDialogTitle() {
        return verifyTitleEquals(EXPECTED_TITLE);
    }

    /**
     * Verify dialog message matches the expected 'Activation Required' dialog title.
     *
     * @return true if title matches, false otherwise
     */
    public boolean verifyActivationRequiredDialogMessage(String entitlementName) {
        WebElement message;
        try {
            message = waitForElementVisible(MESSAGE_LOCATOR);
        } catch (Exception e) {
            Logger.console(String.format("Exception: Cannot locate 'Activation Required' dialog message - %s", e.getMessage()), Logger.Types.WARN);
            return false;
        }
        return message.getText().contains(String.format(AR_DIALOG_MESSAGE_TMPL, entitlementName));
    }

    /**
     * Click 'Activate Game' button to open the code redemption dialog.
     */
    public void clickActivateGameButton() {
        waitForElementClickable(ACTIVATE_GAME_BUTTON_LOCATOR).click();
    }

    /**
     * Click 'Not ..?' button to logout from the current user.
     */
    public void clickNotUserButton() {
        waitForElementClickable(NOT_USER_BUTTON_LOCATOR).click();
    }
}