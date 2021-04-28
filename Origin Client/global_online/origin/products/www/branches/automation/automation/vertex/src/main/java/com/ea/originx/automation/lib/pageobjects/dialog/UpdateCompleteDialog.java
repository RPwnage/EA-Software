package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the dialog that appears when an update is complete.
 *
 * @author caleung
 */
public class UpdateCompleteDialog extends Dialog {

    protected static final By CANCEL_BUTTON_LOCATOR = By.cssSelector(".otkmodal-title");
    protected static final String EXPECTED_TITLE = "UPDATE COMPLETE";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public UpdateCompleteDialog(WebDriver driver) {
        super(driver, null, null, CANCEL_BUTTON_LOCATOR);
    }

    /**
     * Override isOpen to also check for matching title.
     *
     * @return true if dialog is open with matching title, false otherwise
     */
    @Override
    public boolean isOpen() {
        return super.isOpen() && verifyTitleEqualsIgnoreCase(EXPECTED_TITLE);
    }
}