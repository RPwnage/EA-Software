package com.ea.originx.automation.lib.pageobjects.dialog;

import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import com.ea.originx.automation.lib.pageobjects.template.Dialog; 

/**
 * Represents the 'Soft Dependency Suggestion' Dialog
 * 
 * @author mdobre
 */
public class SoftDependencySuggestionDialog extends Dialog {

    private static final By TITLE_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-header .otkmodal-title");
    private static final By CONTINUE_WITHOUT_ADDING_DLC_BUTTON = By.cssSelector("button.origin-telemetry-upsellxsell-dlc-excluded");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public SoftDependencySuggestionDialog(WebDriver driver) {
        super(driver, TITLE_LOCATOR, TITLE_LOCATOR);
    }
    
    /**
     * Clicks the 'Continue without adding' CTA
     */
    public void clickContinueWithoutAddingDlcButton() {
        WebElement continueWithoutAddingDlcButton = waitForElementPresent(CONTINUE_WITHOUT_ADDING_DLC_BUTTON);
        scrollToElement(continueWithoutAddingDlcButton);
        continueWithoutAddingDlcButton.click();
    }
}