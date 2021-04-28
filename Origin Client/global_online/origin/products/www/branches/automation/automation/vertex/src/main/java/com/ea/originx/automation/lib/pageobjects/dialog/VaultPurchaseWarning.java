package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Origin Vault Purchase Warning' dialog
 *
 * @author cdeaconu
 */
public class VaultPurchaseWarning extends Dialog{
    
    private static final String VAULT_PURCHASE_WARNING_DIALOG_TMPL = "origin-dialog-content-vault-purchasewarning";
    private static final By VAULT_PURCHASE_WARNING_DIALOG_LOCATOR = By.cssSelector(VAULT_PURCHASE_WARNING_DIALOG_TMPL);
    private static final By CLOSE_LOCATOR = By.cssSelector(".otkmodal-close");
    private static final By PURCHASE_ANYWAYS_CTA_LOCATOR = By.cssSelector(VAULT_PURCHASE_WARNING_DIALOG_TMPL + " .otkmodal-footer .otkbtn-primary");
    
    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public VaultPurchaseWarning(WebDriver driver) {
        super(driver, VAULT_PURCHASE_WARNING_DIALOG_LOCATOR, CLOSE_LOCATOR);
    }
    
    /**
     * Clicks the 'Purchase anyways' CTA
     */
    public void clickPurchaseAnywaysCTA() {
        waitForElementClickable(PURCHASE_ANYWAYS_CTA_LOCATOR).click();
    }
}
