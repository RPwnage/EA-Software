package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Purchase Prevention' dialog
 *
 * @author cdeaconu
 */
public class PurchasePreventionDialog extends Dialog{
    
    private static final String PURCHASE_PREVENTION_DIALOG_CSS = "origin-dialog-purchaseprevention";
    private static final By PURCHASE_PREVENTION_DIALOG_LOCATOR = By.cssSelector(PURCHASE_PREVENTION_DIALOG_CSS);
    private static final By TITLE_LOCATOR = By.cssSelector(PURCHASE_PREVENTION_DIALOG_CSS + " .origin-dialog-content-pp-title");
    private static final By PURCHASE_ANYWAY_BUTTON_LOCATOR = By.cssSelector(PURCHASE_PREVENTION_DIALOG_CSS + " .otkbtn-primary");
    private static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(PURCHASE_PREVENTION_DIALOG_CSS + " .otkbtn-light");
    private static final String[] TITLE_KEYWORDS = {"already", "have", "game"};
    
    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public PurchasePreventionDialog(WebDriver driver) {
        super(driver, PURCHASE_PREVENTION_DIALOG_LOCATOR, TITLE_LOCATOR, CLOSE_BUTTON_LOCATOR);
    }
    
    /**
     * Verify the title specifies the user owns already a bundle item
     * 
     * @return true if the title contains keywords, false otherwise
     */
    public boolean verifyTitleContainBundleItem(){
        return StringHelper.containsIgnoreCase(waitForElementVisible(TITLE_LOCATOR).getText(), TITLE_KEYWORDS);
    }
    
    /**
     * Verify the 'Purchase Anyway' button is visible
     * 
     * @return true if the button is visible, false otherwise
     */
    public boolean verifyPurchaseAnywayButtonVisible(){
        return waitIsElementVisible(PURCHASE_ANYWAY_BUTTON_LOCATOR);
    }
    
    /**
     * Clicks the 'Purchase Anyway' CTA
     */
    public void clickPurchaseAnywayCTA(){
        waitForElementClickable(PURCHASE_ANYWAY_BUTTON_LOCATOR).click();
    }
}
