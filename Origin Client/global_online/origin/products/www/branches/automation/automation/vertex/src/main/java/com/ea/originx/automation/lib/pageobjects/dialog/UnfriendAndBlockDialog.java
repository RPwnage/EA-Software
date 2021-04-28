package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * A page object representing the dialog for unfriending and blocking a friend.
 *
 * @author lscholte
 */
public class UnfriendAndBlockDialog extends Dialog {

    protected static final By UNFRIEND_AND_BLOCK_BUTTON_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-footer .otkmodal-btn-yes");
    protected static final By CANCEL_BUTTON_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-footer .otkmodal-btn-no");
    protected static final String DIALOG_TITLE = "Unfriend and Block";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public UnfriendAndBlockDialog(WebDriver driver) {
        super(driver);
    }

    /**
     * Click the 'Unfriend and Block' button.
     */
    public void clickUnfriendAndBlock() {
        waitForElementClickable(UNFRIEND_AND_BLOCK_BUTTON_LOCATOR).click();
    }
}