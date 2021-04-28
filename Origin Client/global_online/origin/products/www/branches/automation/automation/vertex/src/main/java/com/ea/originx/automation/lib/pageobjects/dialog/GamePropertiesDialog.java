package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * A page object representing the 'Game Properties' dialog.
 *
 * @author lscholte
 */
public class GamePropertiesDialog extends Dialog {

    protected static final By TITLE_LOCATOR = By.cssSelector("origin-dialog-content-gameproperties .otkmodal-header .otkmodal-title");
    protected static final By DIALOG_LOCATOR = By.cssSelector("origin-dialog-content-gameproperties");
    protected static final By SAVE_BUTTON_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-footer .otkbtn-primary");
    protected static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-footer .otkbtn-light");
    protected static final By ENABLE_OIG_CHECKBOX_LOCATOR = By.cssSelector("#enable-OIG-input");

    public GamePropertiesDialog(WebDriver driver) {
        super(driver, DIALOG_LOCATOR, TITLE_LOCATOR);
    }

    /**
     * Click on the 'Save' button, which closes the dialog.
     */
    public void save() {
        waitForElementClickable(SAVE_BUTTON_LOCATOR).click();
    }

    /**
     * Sets the 'Enable OIG' checkbox to the specified state.
     *
     * @param enable If true, set the check the checkbox. If false, uncheck the
     * checkbox
     */
    public void setEnableOIG(boolean enable) {
        if (verifyOIGEnabled() != enable) {
            // The checkbox is invisible but is used to track the state of the parent
            // the parent element, so we click that instead.
            WebElement element = waitForElementPresent(ENABLE_OIG_CHECKBOX_LOCATOR).findElement(By.xpath("./.."));
            waitForElementClickable(element).click();
        }
    }

    /**
     * Verify that the 'Enable OIG' checkbox is checked.
     *
     * @return true if the Enable OIG checkbox is checked, false otherwise
     */
    public boolean verifyOIGEnabled() {
        return waitForElementPresent(ENABLE_OIG_CHECKBOX_LOCATOR).isSelected();
    }
}