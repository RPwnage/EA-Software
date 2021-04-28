package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * The dialog that appears when the system has some local data saved.
 *
 * @author nvarthakavi
 */
public class NewSaveDataFoundDialog extends Dialog{

    protected static final By DIALOG_LOCATOR = By.cssSelector("origin-dialog-content-prompt[header='New save data found']");
    protected static final By TITLE_LOCATOR = By.cssSelector(".otkmodal-title.otktitle-2.origin-dialog-header");
    protected static final By CANCEL_BUTTON_LOCATOR = By.cssSelector(".dialog-content-prompt-section-footer .otkbtn-primary");
    protected static final By DELETE_LOCAL_DATA_BUTTON_LOCATOR = By.cssSelector(".origin-cldialog-commandbuttons button.otkbtn-command:nth-of-type(1)");
    protected static final By SAVE_LOCAL_DATA_BUTTON_LOCATOR = By.cssSelector(".origin-cldialog-commandbuttons button.otkbtn-command:nth-of-type(2)");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public NewSaveDataFoundDialog(WebDriver driver) {
        super(driver,DIALOG_LOCATOR,TITLE_LOCATOR);
    }

    /**
     * Click the 'Delete Local Data' button to clear the local data
     * in the computer.
     */
    public void clickDeleteLocalDataButton() {
        waitForElementClickable(DELETE_LOCAL_DATA_BUTTON_LOCATOR).click();
    }

    /**
     * Click the 'Save Local Data' button.
     */
    public void clickSaveLocalDataButton() {
        waitForElementVisible(SAVE_LOCAL_DATA_BUTTON_LOCATOR).click();
    } 

    /**
     * Click the 'Cancel' button.
     */
    public void clickCancelButton() {
        waitForElementVisible(CANCEL_BUTTON_LOCATOR).click();
    }

    /**
     * Verify the 'Delete' and 'Save' buttons exist.
     *
     * @return true if both exist, false otherwise
     */
    public boolean verifyDeleteAndSaveButtonsExist() {
        return waitIsElementVisible(DELETE_LOCAL_DATA_BUTTON_LOCATOR) && waitIsElementVisible(SAVE_LOCAL_DATA_BUTTON_LOCATOR);
    }
}