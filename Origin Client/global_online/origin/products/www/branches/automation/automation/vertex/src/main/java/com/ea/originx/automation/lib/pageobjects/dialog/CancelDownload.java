package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Confirmation' dialog after attempting to cancel a download.
 *
 * @author glivingstone
 * @author palui
 */
public class CancelDownload extends Dialog {

    protected static final By YES_BUTTON = By.cssSelector(".otkmodal-content .otkmodal-footer .otkmodal-btn-yes");
    protected static final String EXPECTED_TITLE = "CANCEL DOWNLOAD";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public CancelDownload(WebDriver driver) {
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
    public void clickYes() {
        clickButtonWithRetries(YES_BUTTON, YES_BUTTON);
    }
}