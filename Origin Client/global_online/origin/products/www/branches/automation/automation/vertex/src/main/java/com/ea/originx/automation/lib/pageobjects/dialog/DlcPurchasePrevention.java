package com.ea.originx.automation.lib.pageobjects.dialog;

import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

public class DlcPurchasePrevention extends Dialog {

    public static final String DIALOG_LOCATOR = "origin-dialog origin-dialog-dlcpurchaseprevention";
    public static final By CLOSE_BUTTON = By.cssSelector(".otkmodal-close i.otkicon-closecircle");
    public static final By CONTINUE_PURCHASE_BUTTON = By.cssSelector(DIALOG_LOCATOR + " button.otkbtn-primary");

    /**
     * Constructor
     *
     * @param driver          Selenium Web Driver
     * @param offerId         The offer id of the entitlement
     */
    public DlcPurchasePrevention(WebDriver driver, String offerId){
        super(driver, By.cssSelector(String.format(DIALOG_LOCATOR, offerId.toUpperCase())), CLOSE_BUTTON);
    }

    /**
     * Click the 'Continue purchase' button at the bottom of the dialog
     */
    public void clickContinuePurchaseButton() {
        WebElement continuePurchaseButton = waitForElementPresent(CONTINUE_PURCHASE_BUTTON);
        continuePurchaseButton.click();
    }
}
