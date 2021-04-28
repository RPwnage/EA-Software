package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * A page object representing the dialog for ignoring friend requests and block.
 *
 * @author rchoi
 */
public class IgnoreFriendRequestAndBlockDialog extends Dialog {

    protected static final By TITLE_LOCATOR = By.cssSelector("origin-dialog-content-prompt[header='Ignore friend request and block']");
    protected static final By IGNORE_FRIEND_REQUEST_AND_BLOCK_BUTTON_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-footer .otkmodal-btn-yes");
    protected static final By CANCEL_BUTTON_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-footer .otkmodal-btn-no");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public IgnoreFriendRequestAndBlockDialog(WebDriver driver) {
        super(driver, TITLE_LOCATOR);
    }

    /**
     * Click the 'Unfriend and Block' button.
     */
    public void clickIgnoreFriendRequestAndBlock() {
        waitForElementClickable(IGNORE_FRIEND_REQUEST_AND_BLOCK_BUTTON_LOCATOR).click();
    }
}