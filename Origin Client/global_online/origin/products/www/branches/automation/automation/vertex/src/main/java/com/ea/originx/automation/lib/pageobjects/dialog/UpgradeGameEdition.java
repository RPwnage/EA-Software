package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Upgrade Game Edition Warning' dialog
 *
 * @author cdeaconu
 */
public class UpgradeGameEdition extends Dialog{
    
    private static final String UPGRADE_GAME_EDITION_DIALOG_TMPL = ".origin-store-dialog-content-vaultdlcupgradewarning";
    private static final By UPGRADE_GAME_EDITION_DIALOG_LOCATOR = By.cssSelector(UPGRADE_GAME_EDITION_DIALOG_TMPL);
    private static final By CLOSE_LOCATOR = By.cssSelector(".otkmodal-close");
    private static final By UPGRADE_GAME_EDITION_CTA_LOCATOR = By.cssSelector(UPGRADE_GAME_EDITION_DIALOG_TMPL + " .otkmodal-footer .otkbtn-primary");
    private static final By CANCEL_CTA_LOCATOR = By.cssSelector(UPGRADE_GAME_EDITION_DIALOG_TMPL + " .otkmodal-footer .otkbtn-light");
    
    public UpgradeGameEdition(WebDriver driver) {
        super(driver, UPGRADE_GAME_EDITION_DIALOG_LOCATOR, CLOSE_LOCATOR);
    }
    
    /**
     * Clicks the 'Upgrade Game Edition' CTA
     */
    public void clickUpgradeGameEditionCTA() {
        waitForElementClickable(UPGRADE_GAME_EDITION_CTA_LOCATOR).click();
    }
    
    /**
     * Clicks the 'Cancel' CTA
     */
    public void clickCancelCTA() {
        waitForElementClickable(CANCEL_CTA_LOCATOR).click();
    }
    
}
