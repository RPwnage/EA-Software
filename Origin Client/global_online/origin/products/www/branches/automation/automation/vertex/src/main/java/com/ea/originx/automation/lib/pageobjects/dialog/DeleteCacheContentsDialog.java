package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 *  A page object that represents the 'Delete Cache Contents Confirmation' dialog.
 *
 * @author mkalaivanan
 */
public class DeleteCacheContentsDialog extends Dialog {
    protected static final By YES_BUTTON_LOCATOR = By.cssSelector(".otkmodal-btn-yes");
   
    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */   
    public DeleteCacheContentsDialog(WebDriver driver) {
        super(driver);
    }

    /**
     * Click on 'Yes' button.
     */
    public void clickYes() {
        waitForElementClickable(YES_BUTTON_LOCATOR).click();
    }
}