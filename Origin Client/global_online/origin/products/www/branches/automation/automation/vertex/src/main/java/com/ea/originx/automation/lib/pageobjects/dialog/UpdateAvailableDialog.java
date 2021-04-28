package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the dialog that appears when attempting to play a game that has an
 * update available for download.
 *
 * @author lscholte
 * @author palui
 */
public class UpdateAvailableDialog extends Dialog {

    protected static final By CANCEL_BUTTON_LOCATOR = By.cssSelector(".otkmodal-footer .otkmodal-btn-no.otkbtn-primary");
    protected static final String EXPECTED_TITLE = "UPDATE AVAILABLE";
    protected static final String BUTTON_INFO_XPATH = "//BUTTON[contains(@class,'otkbtn')]//DIV[contains(@class,'otkbtn-command-info')]";

    protected static final By UPDATE_BUTTON_LOCATOR = By.xpath(BUTTON_INFO_XPATH + "/P[contains(@class,'otkc')]/STRONG[contains(text(),'Update')]");
    protected static final By PLAY_WITHOUT_UPDATE_BUTTON_LOCATOR
            = By.xpath(BUTTON_INFO_XPATH + "/P[contains(@class,'otkc')]/STRONG[contains(text(),'Play Without Updating')]");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public UpdateAvailableDialog(WebDriver driver) {
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
     * Click the 'Update' button.
     */
    public void clickUpdateButton() {
        waitForElementClickable(UPDATE_BUTTON_LOCATOR).click();
    }

    /**
     * Click the 'Play Without Updating' button.
     */
    public void clickPlayWithoutUpdatingButton() {
        waitForElementClickable(PLAY_WITHOUT_UPDATE_BUTTON_LOCATOR).click();
    }
}