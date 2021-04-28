package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the warning dialog.
 * (e.g. warning while downloading a game on skipping installation of PunkBuster)
 * todo: dialog -> rename to warningdialog
 * todo: check files using this
 * todo: need to test
 *
 * @author jmittertreiner
 * @author palui
 */
public class WarningDialog extends Dialog {

    private static final String WARNING_CONTENT_CSS = "origin-dialog-content-prompt[header='Warning'] ";
    private static final By WARNING_CONTENT_LOCATOR = By.cssSelector(WARNING_CONTENT_CSS);

    private static final String WARNING_DESCRIPTION_CSS = WARNING_CONTENT_CSS + ".otkmodal-body .otkc";
    private static final By WARNING_DESCRIPTION_LOCATOR = By.cssSelector(WARNING_DESCRIPTION_CSS);

    private static final By YES_BTN_LOCATOR = By.cssSelector(WARNING_CONTENT_CSS + " .otkmodal-footer .otkmodal-btn-yes");
    private static final By NO_BTN_LOCATOR = By.cssSelector(WARNING_CONTENT_CSS + " .otkmodal-footer .otkmodal-btn-no");

    /**
     * Constructor.
     * Note this warning dialog does not have a close circle button.
     *
     * @param driver Selenium WebDriver
     */
    public WarningDialog(WebDriver driver) {
        super(driver, WARNING_CONTENT_LOCATOR, null, NO_BTN_LOCATOR);
    }

    public String getDescription() {
        return waitForElementVisible(WARNING_DESCRIPTION_LOCATOR).getAttribute("textContent");
    }

    /**
     * Checks if the 'Skip Punk Buster' warning dialog is open.
     */
    public boolean isSkipPunkBusterWarningDialog() {
        return super.isOpen() && verifyTitleEqualsIgnoreCase("Warning")
                && StringHelper.containsIgnoreCase(getDescription(), "PunkBuster");
    }

    /**
     * Clicks 'Yes' on the dialog.
     */
    public void clickYes() {
        waitForElementVisible(YES_BTN_LOCATOR).click();
    }
}