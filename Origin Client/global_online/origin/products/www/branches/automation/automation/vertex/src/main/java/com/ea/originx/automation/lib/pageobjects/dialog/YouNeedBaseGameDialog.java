package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * This dialog pops up when a DLC/Preorder is purchased when the user doesn't own a base game.
 *
 * @author nvarthakavi
 */
public class YouNeedBaseGameDialog extends Dialog{

    public static final String DIALOG_LOCATOR = "origin-basegame-purchasesuggestion[dlcgame-name='%s']";
    public static final By CONTINUE_PURCHASE_BUTTON_LOCATOR = By.cssSelector(".otkmodal-footer button.otkbtn.otkbtn-primary");

    /**
     * Constructor
     *
     * @param driver          Selenium Web Driver
     * @param entitlementName The entitlement name of the game to be played
     */
    public YouNeedBaseGameDialog(WebDriver driver,String entitlementName){
        super(driver, By.cssSelector(String.format(DIALOG_LOCATOR, entitlementName.toUpperCase())));
    }

    /**
     * Click the 'Continue Purchase' button to play the trial game.
     */
    public void clickContinuePurchaseButton() {
        waitForElementClickable(CONTINUE_PURCHASE_BUTTON_LOCATOR).click();
    }
}