package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.vx.originclient.client.OriginClient;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;

/**
 * Represents the dialog which appears to confirm changing the client
 * language.
 */
public class ConfirmLanguageDialog extends Dialog {
    private static final By RESTART_LOCATOR = By.cssSelector(".otkmodal-btn-yes");
    private static final By CANCEL_BUTTON_LOCATOR = By.cssSelector(".otkmodal-btn-no");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ConfirmLanguageDialog(WebDriver driver) {
        super(driver, RESTART_LOCATOR);
    }

    /**
     * Clicks the 'Confirm' button on the dialog.
     *
     * Also updates the Origin launch state to launch new instances of Origin
     * in the new language.
     */
    public void restart() {
        waitForElementClickable(RESTART_LOCATOR).click();
        // Commit the language change
        OriginClient.getInstance(driver).commitStagedLanguage();
    }

    /**
     * Clicks the 'Cancel' button on the dialog.
     */
    public void cancel() {
        waitForElementClickable(CANCEL_BUTTON_LOCATOR).click();
    }
}