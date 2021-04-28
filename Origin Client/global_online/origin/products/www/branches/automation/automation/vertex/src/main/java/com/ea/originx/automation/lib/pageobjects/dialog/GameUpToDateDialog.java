package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * A dialog that appears when attempting to update a game that is already
 * up to date.
 *
 * @author lscholte
 */
public class GameUpToDateDialog extends Dialog {

    protected static final By DIALOG_LOCATOR = By.cssSelector("origin-dialog-content-prompt");
    protected static final By DIALOG_BODY_MESSAGE_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-body .otkc");
    protected static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(".otkmodal-footer .otkmodal-btn-no");
    protected static final String DIALOG_MESSAGE = "is up to date";

    public GameUpToDateDialog(WebDriver driver) {
        super(driver, DIALOG_LOCATOR, null, CLOSE_BUTTON_LOCATOR);
    }
}