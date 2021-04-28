package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 *  A page object that represents the 'Installation Settings Changed' dialog.
 *
 *  @author mkalaivanan
 */
public class InstallationSettingsChangedDialog extends Dialog {

    protected static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(".otkmodal-btn-no");
    protected static final By INSTALLATION_SETTINGS_CHANGED_DIALOG_LOCATOR = By.cssSelector("origin-dialog-content-prompt");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public InstallationSettingsChangedDialog(WebDriver driver) {
        super(driver, INSTALLATION_SETTINGS_CHANGED_DIALOG_LOCATOR, null, CLOSE_BUTTON_LOCATOR);
    }
}