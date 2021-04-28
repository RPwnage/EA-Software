package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Add To Download Queue' dialog.
 *
 * @author jmittertreiner
 * @author palui
 */
public class AddToDownloadQueueDialog extends Dialog {

    protected static final By NEXT_BUTTON_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-footer .otkmodal-btn-yes");
    protected static final String EXPECTED_TITLE = "Add to Download Queue";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public AddToDownloadQueueDialog(WebDriver driver) {
        super(driver);
    }

    /**
     * Override isOpen to also check for matching title.
     *
     * @return true if dialog is open with matching title, false otherwise
     */
    @Override
    public boolean isOpen() {
        return super.isOpen() && verifyTitleEquals(EXPECTED_TITLE);
    }

    /**
     * Click the 'Yes' button on the dialog.
     */
    public void clickNextButton() {
        waitForElementVisible(NEXT_BUTTON_LOCATOR).click();
    }
}