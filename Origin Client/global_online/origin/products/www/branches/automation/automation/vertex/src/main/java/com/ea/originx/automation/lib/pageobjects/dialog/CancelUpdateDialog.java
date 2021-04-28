package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Confirmation' dialog after attempting to cancel an update.
 *
 * @author lscholte
 * @author palui
 */
public class CancelUpdateDialog extends Dialog {

    protected static final By YES_BUTTON_LOCATOR = By.cssSelector(".otkmodal-footer .otkmodal-btn-yes");
    protected static final By NO_BUTTON_LOCATOR = By.cssSelector(".otkmodal-footer .otkmodal-btn-no");
    protected static final String EXPECTED_TITLE = "Are you sure you want to cancel your update?";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public CancelUpdateDialog(WebDriver driver) {
        super(driver);
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
     * Clicks the 'Yes' button on the dialog.
     */
    public void clickYesButton() {
        waitForElementClickable(YES_BUTTON_LOCATOR).click();
    }

    /**
     * Clicks the 'No' button on the dialog.
     */
    public void clickNoButton() {
        waitForElementClickable(NO_BUTTON_LOCATOR).click();
    }
}