package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object that represents the 'Deals' page.
 *
 * @author rocky
 */
public class DealsPage extends EAXVxSiteTemplate {

    protected static final String DEALS_PAGE_TITLE_CSS = "div[class='origin-store-paragraph-pagetitle'][description*='Deals']";
    protected static final String GAME_TILE_CSS = "div[class*='origin-store-carousel-product-carousel-container origin-store-carousel-carousel-container'] > ul > li > origin-store-game-tile";
    protected static final String STORE_GAME_TILE_CTA_CSS = ".origin-storegametile-cta";
    protected static final String STORE_GAME_TILE_TITLE_CSS = ".origin-storegametile-title";

    protected static final By DEAL_PAGE_TITLE_LOCATOR = By.cssSelector(DEALS_PAGE_TITLE_CSS);
    protected static final By GAME_TILE_LOCATOR = By.cssSelector(GAME_TILE_CSS);
    protected static final By STORE_GAME_TILE_CTA_LOCATOR = By.cssSelector(STORE_GAME_TILE_CTA_CSS);
    protected static final By STORE_GAME_TILE_TITLE_LOCATOR = By.cssSelector(STORE_GAME_TILE_TITLE_CSS);
    protected static final By GAME_TILE_OFFER_ID_LOCATOR = By.cssSelector("div[class*='origin-storeprice']");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public DealsPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'Deals' page is displayed
     *
     * @return true if page is displayed, false otherwise
     */
    public boolean verifyDealsPageReached() {
        return waitIsElementVisible(DEAL_PAGE_TITLE_LOCATOR);
    }

    /** 
     * Wait for first game tile to load
     */    
    public void waitForFirstGameTileToLoad() {
        waitForElementPresent(GAME_TILE_LOCATOR);
        waitForElementPresent(STORE_GAME_TILE_CTA_LOCATOR);
        waitForElementPresent(STORE_GAME_TILE_TITLE_LOCATOR);
    }
    
    /**
     * Get offer ID from a store game tile WebElement.
     *
     * @return String offer ID of the store game title
     * @throws RuntimeException if the offer ID is an empty String
     */
    private String getOfferId(WebElement storeGameTileElement) {
        String offerId = storeGameTileElement.findElement(GAME_TILE_OFFER_ID_LOCATOR).getAttribute("offer-id");
        if (!offerId.isEmpty()) {
            _log.debug("Get store game tile offerId: " + offerId);
        } else {
            String errorMessage = "Cannot determine offerid for store game tile WebElement: " + storeGameTileElement.toString();
            _log.error(errorMessage);
            throw new RuntimeException(errorMessage);
        }
        return offerId;
    }

    /**
     * Get First store game tile as a StoreGameTile object.
     *
     * @return List of (loaded) StoreGameTile objects
     */
    public StoreGameTile getFirstStoreGameTile() {
        waitForFirstGameTileToLoad();
        WebElement firstGameTile = waitForElementVisible(GAME_TILE_LOCATOR);
        return new StoreGameTile(driver, getOfferId(firstGameTile));
    }
}