package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the 'Edit Basic Information' dialog that appears on the account settings page when
 * managing your account.
 *
 * @author jmittertreiner
 */
public class EditBasicInfoDialog extends Dialog {
    private static final By USER_ID_FIELD = By.id("ebi_originId-input");
    private static final By SAVE_BUTTON = By.id("savebtn_basicinfo");

    public EditBasicInfoDialog(WebDriver webDriver) {
        super(webDriver);
    }

    /**
     * Enter a new ID to the ID field.
     */
    public void enterNewID(String newID) {
        final WebElement userIdField = waitForElementVisible(USER_ID_FIELD);
        userIdField.clear();
        userIdField.sendKeys(newID);
    }

    /**
     * Clicks the 'Save' button.
     */
    public void save() {
        waitForElementClickable(SAVE_BUTTON).click();
    }
}