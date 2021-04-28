package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Component object class for the 'Unfriend User' dialog.
 *
 * @author palui
 */
public final class UnfriendUserDialog extends Dialog {

    protected static final By UNFRIEND_BUTTON_LOCATOR = By.xpath("//BUTTON[contains(@class,'otkbtn') and text()='Unfriend']");
    protected static final By CANCEL_BUTTON_LOCATOR = By.xpath("//BUTTON[contains(@class,'otkbtn') and text()='Cancel']");
    protected static final String DIALOG_TITLE = "Unfriend User";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public UnfriendUserDialog(WebDriver driver) {
        super(driver);
    }

    /**
     * Click the 'Unfriend' button.
     */
    public void clickUnfriend() {
        waitForElementClickable(UNFRIEND_BUTTON_LOCATOR).click();
    }
}