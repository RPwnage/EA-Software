package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * The dialog that appears when a user wants to start a trial.
 *
 * @author nvarthakavi
 */
public class TrialPlayNowDialog extends Dialog {

    public static final String DIALOG_LOCATOR = "origin-dialog-content-prompt[header='%s']";
    public static final By TITLE_LOCATOR = By.cssSelector(".otkmodal-title.otktitle-2.origin-dialog-header");

    public static final By PLAY_NOW_BUTTON_LOCATOR = By.cssSelector(".otkmodal-footer button.otkmodal-btn-yes");

    /**
     * Constructor
     *
     * @param driver          Selenium Web Driver
     * @param entitlementName The entitlement name of the game to be played
     */
    public TrialPlayNowDialog(WebDriver driver, String entitlementName) {
        super(driver, By.cssSelector(String.format(DIALOG_LOCATOR, entitlementName.toUpperCase())), TITLE_LOCATOR);
    }

    /**
     * Click the 'Play Now' button to play the trial game.
     */
    public void clickPlayNowButton() {
        waitForElementClickable(PLAY_NOW_BUTTON_LOCATOR).click();
    }
}