package com.ea.originx.automation.lib.pageobjects.dialog;

import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * This dialog pops up when trying to buy an entitlement that offers
 * additional Dlc add-ons to add to the overall purchase (example: Sims 4 standard)
 *
 * @author jdickens
 */
public class DlcUpSellSuggestionDialog extends Dialog {

    public static final String DIALOG_LOCATOR = "origin-dialog origin-dialog-content-dlcupsellxsellsuggestion[target-offerid='%s']";
    public static final By CLOSE_BUTTON = By.cssSelector(".otkmodal-close i.otkicon-closecircle");
    public static final By CONTINUE_WITHOUT_ADDING_DLC_BUTTON = By.cssSelector("button.origin-telemetry-upsellxsell-dlc-excluded");

    /**
     * Constructor
     *
     * @param driver          Selenium Web Driver
     * @param offerId         The offer id of the entitlement
     */
    public DlcUpSellSuggestionDialog(WebDriver driver, String offerId){
        super(driver, By.cssSelector(String.format(DIALOG_LOCATOR, offerId.toUpperCase())), CLOSE_BUTTON);
    }

    /**
     * Click the 'Continue without Adding' button at the bottom of the dialog
     */
    public void clickContinueWithoutAddingDlcButton() {
        WebElement continueWithoutAddingDlcButton = waitForElementPresent(CONTINUE_WITHOUT_ADDING_DLC_BUTTON);
        scrollToElement(continueWithoutAddingDlcButton);
        continueWithoutAddingDlcButton.click();
    }
}