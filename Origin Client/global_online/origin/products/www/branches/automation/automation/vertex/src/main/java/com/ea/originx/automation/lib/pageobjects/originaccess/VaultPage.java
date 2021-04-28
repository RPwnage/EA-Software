package com.ea.originx.automation.lib.pageobjects.originaccess;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * A page object that represents the 'Origin Access Vault' page. Note that this
 * is the default Origin Access page if the user already owns Origin Access.
 *
 * @author glivingstone
 */
public class VaultPage extends EAXVxSiteTemplate {

    protected static final By ORIGIN_ACCESS_VAULT_PAGE_LOCATOR = By.cssSelector("#storecontent");
    protected static final String VAULT_TILES_CSS = ".store-access-vault-wrapper-games li.origin-store-access-vault-slider-tile";
    protected static final String VAULT_TILES_CTA_CSS = VAULT_TILES_CSS + "[origin-automation-id='game-tile']";
    protected static final String VAULT_TILES_HOMETILE_CSS = "otkex-hometile";
    protected static final By VAULT_TILES_CTA_TEXT_LOCATOR = By.cssSelector(VAULT_TILES_CTA_CSS);
    protected static final String VAULT_TILE_INDEX_CSS_TMPL = VAULT_TILES_CSS + " otkex-hometile[index='%s'] a";
    protected static final String VAULT_TILE_NAME_TMPL = VAULT_TILES_CSS + " otkex-hometile[tile-header='%s']";
    protected static final By VAULT_TILES_LOCATOR = By.cssSelector(VAULT_TILES_CSS);
    protected static final String VAULT_TILE_NOT_FOUND = "Vault tile could not be found for ";
    
    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public VaultPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Click on entitlement to navigate to the GDP page given the offer ID of the entitlement.
     *
     * @param offerID The offer ID of the entitlement to navigate to
     */
    public void clickEntitlementByOfferID(String offerID) {
        int tries = 0;
        while (!waitIsElementPresent(By.cssSelector(String.format(VAULT_TILE_INDEX_CSS_TMPL, offerID)))) {
            scrollToBottom();
            sleep(1000); // Wait for more tiles to load
            if (tries++ > 10) {
                _log.error(VAULT_TILE_NOT_FOUND + offerID);
                return;
            }
        }
        WebElement entitlementTile = waitForElementPresent(By.cssSelector(String.format(VAULT_TILE_INDEX_CSS_TMPL, offerID)));
        entitlementTile.click();
    }

    /**
     * Click on entitlement to the navigate to the GDP page.
     *
     * @param eName The name of the entitlement to navigate to
     */
    public void clickEntitlementByName(String eName) {
        int tries = 0;
        while (!waitIsElementPresent(By.cssSelector(String.format(VAULT_TILE_NAME_TMPL, eName)))) {
            scrollToBottom();
            sleep(1000); // Wait for more tiles to load
            if (tries++ > 10) {
                _log.error(VAULT_TILE_NOT_FOUND + eName);
                return;
            }
        }
        WebElement entitlementTile = waitForElementPresent(By.cssSelector(String.format(VAULT_TILE_NAME_TMPL, eName)));
        if (entitlementTile != null) {
            //scroll to centre in order to avoid errors when 'A new version of Origin is ready' message appears
            scrollElementToCentre(entitlementTile); 
            waitForElementClickable(entitlementTile).click();
        } else {
            _log.error(VAULT_TILE_NOT_FOUND + eName);
        }
    }

    /**
     * Verify if the 'Origin Access Vault' page is displayed.
     *
     * @return true if page displayed, false otherwise
     */
    public boolean verifyPageReached() {
        return waitIsElementVisible(ORIGIN_ACCESS_VAULT_PAGE_LOCATOR);
    }

    /**
     * Wait for the 'Vault' Tiles to load
     */
    public void waitForGamesToLoad() {
        waitForElementPresent(VAULT_TILES_CTA_TEXT_LOCATOR, 30); // wait added for slow loading of tiles
        waitForElementPresent(VAULT_TILES_LOCATOR);
    }

    /**
     * Get the offerId of the first 'Vault Tile' displayed
     *
     * @return the offerId of the first 'Vault Tile'
     */
    public String getFirstVaultTileOfferId() {
        return driver.findElements(By.cssSelector(VAULT_TILES_CSS)).get(0).findElement(By.cssSelector(VAULT_TILES_HOMETILE_CSS)).getAttribute("index");
    }
}