package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * A dialog that appears when attempting to set an invalid game library location.
 * 
 * @author lscholte
 */
public class InvalidDirectoryDialog extends Dialog {

    protected static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(".otkmodal-btn-no");
    protected static final By INVALID_DIRECTORY_DIALOG_LOCATOR = By.cssSelector("origin-dialog-content-prompt[header='Cannot save in forbidden directory']");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public InvalidDirectoryDialog(WebDriver driver) {
        super(driver, INVALID_DIRECTORY_DIALOG_LOCATOR, null, CLOSE_BUTTON_LOCATOR);
    }
}