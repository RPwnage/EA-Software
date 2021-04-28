package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Origin Vault Purchase Suggestion' dialog
 *
 * @author cdeaconu
 */
public class VaultPurchaseSuggestion extends Dialog{
    
    private static final String VAULT_PURCHASE_SUGGESTION_DIALOG_TMPL = "origin-dialog-content-vault-purchasesuggestion";
    private static final By VAULT_PURCHASE_SUGGESTION_DIALOG_LOCATOR = By.cssSelector(VAULT_PURCHASE_SUGGESTION_DIALOG_TMPL);
    private static final By CONTINUE_WITHOUT_CTA_LOCATOR = By.cssSelector(VAULT_PURCHASE_SUGGESTION_DIALOG_TMPL + " .otkmodal-footer .otkbtn-primary");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public VaultPurchaseSuggestion(WebDriver driver) {
        super(driver, VAULT_PURCHASE_SUGGESTION_DIALOG_LOCATOR);
    }
    
    /**
     * Clicks the 'Continue Without' CTA
     */
    public void clickContinueWithoutCTA() {
        waitForElementClickable(CONTINUE_WITHOUT_CTA_LOCATOR).click();
    }
}
