package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Dialog indicating that the age requirement is not met when an underage user wants to buy games.
 *
 * @author nvarthakavi
 */
public class UnderAgeWarningDialog extends Dialog {

    protected static final By TITLE_LOCATOR = By.cssSelector(".otkmodal-content .origin-dialog-content .otkmodal-content .origin-dialog-content-errormodal .clearfix .otktitle-2");
    protected static final By DIALOG_BODY_LOCATOR = By.cssSelector(".otkmodal-content .origin-dialog-content .otkmodal-content .origin-dialog-content-errormodal .clearfix .otktitle-3");
    protected static final String EXPECTED_TITLE = "Age requirement not met";
    protected static final String DIALOG_BODY_KEYWORDS[] = {"sorry", "age", "requirement", "purchase"};

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public UnderAgeWarningDialog(WebDriver driver) {
        super(driver,TITLE_LOCATOR,TITLE_LOCATOR);
    }

    /**
     * Override isOpen to also check for matching title
     *
     * @return true if dialog is open with matching title, false otherwise
     */
    @Override
    public boolean isOpen() {
        return super.isOpen() && verifyTitleEqualsIgnoreCase(EXPECTED_TITLE);
    }

    /**
     * Verifies the dialog body text matches the expected keywords
     *
     * @return true if the keywords are a match, false otherwise
     */
    public boolean verifyDialogBodyText() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(DIALOG_BODY_LOCATOR).getText(), DIALOG_BODY_KEYWORDS);
    }
}