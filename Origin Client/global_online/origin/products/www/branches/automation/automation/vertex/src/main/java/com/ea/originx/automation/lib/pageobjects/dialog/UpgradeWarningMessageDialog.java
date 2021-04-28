package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import org.openqa.selenium.WebDriver;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;

/**
 * Dialog prompted when lauching a base entitlement when a Vault edition upgrade
 * is available
 *(prompt appears only once at first launch)
 *
 * @author cbalea
 */
public class UpgradeWarningMessageDialog extends Dialog {

    protected static final String UPGRADE_WARNING_DIALOG_CSS = "origin-dialog-content-vaultupgradeonplayaction";
    protected static final By UPGRADE_WARNING_DIALOG_LOCATOR = By.cssSelector(UPGRADE_WARNING_DIALOG_CSS);
    protected static final By UPGRADE_WARNING_DIALOG_TITLE_LOCATOR = By.cssSelector(UPGRADE_WARNING_DIALOG_CSS + " .otkmodal-header h4");
    protected static final By UPGRADE_WARNING_DIALOG_MESSAGE_BODY_LOCATOR = By.cssSelector(UPGRADE_WARNING_DIALOG_CSS + " .otkmodal-body");
    protected static final By UPGRADE_WARNING_DIALOG_UPGRADE_NOW_CTA_LOCATOR = By.cssSelector(UPGRADE_WARNING_DIALOG_CSS + " .otkbtn-primary");
    protected static final By UPGRADE_WARNING_DIALOG_NO_THANKS_CTA_LOCATOR = By.cssSelector(UPGRADE_WARNING_DIALOG_CSS + " .otkbtn-light");
    protected static final String[] UPGRADE_MESSAGE_KEYWORDS = {"included", "access"};
    protected static final String[] UPGRADE_NEW_SAVES_KEYWORDS = {"expire", "load", "new", "game", "saves"};
    protected static final String UPGRADE_WARNING_DIALOG_TITLE = "Upgrade your game edition";

    /**
     * Constructor
     *
     * @param driver WebDriver
     */
    public UpgradeWarningMessageDialog(WebDriver driver) {
        super(driver, UPGRADE_WARNING_DIALOG_LOCATOR, UPGRADE_WARNING_DIALOG_TITLE_LOCATOR, null);
    }

    /**
     * Verifies the title of the dialog matches the expected title
     *
     * @return true if the title is a match, false otherwise
     */
    public boolean verifyDialogTitle() {
        return verifyTitleContainsIgnoreCase(UPGRADE_WARNING_DIALOG_TITLE);
    }

    /**
     * Verifies the dialog contains a message to upgrade game to vault edition
     *
     * @return true if the dialog body contains matching keywords, false
     * otherwise
     */
    public boolean verifyUpgradeMessageWarning() {
        return StringHelper.containsIgnoreCase(getDialodBodyText(), UPGRADE_MESSAGE_KEYWORDS);
    }

    /**
     * Verifies the dialog contains a message regarding new game saves
     *
     * @return true if the dialog body contains matching keywords, false
     * otherwise
     */
    public boolean verifyNewSaveGameWarning() {
        return StringHelper.containsIgnoreCase(getDialodBodyText(), UPGRADE_NEW_SAVES_KEYWORDS);
    }

    /**
     * Verifies the 'Upgrade Now' CTA is displayed
     *
     * @return true if the CTA is displayed, false otherwise
     */
    public boolean verifyUpgradeNowCTA() {
        return waitIsElementPresent(UPGRADE_WARNING_DIALOG_UPGRADE_NOW_CTA_LOCATOR);
    }

    /**
     * Verifies the 'Upgrade Now' CTA is displayed
     *
     * @return true if the CTA is displayed, false otherwise
     */
    public boolean verifyNoThanksCTA() {
        return waitIsElementPresent(UPGRADE_WARNING_DIALOG_NO_THANKS_CTA_LOCATOR);
    }

    /**
     * Gets the text within the body of the dialog
     *
     * @return the text found in the dialog body
     */
    public String getDialodBodyText() {
        return waitForElementVisible(UPGRADE_WARNING_DIALOG_MESSAGE_BODY_LOCATOR).getText();
    }
}
