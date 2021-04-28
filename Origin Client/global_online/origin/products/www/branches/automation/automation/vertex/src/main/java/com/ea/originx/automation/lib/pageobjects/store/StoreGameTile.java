package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.lang.invoke.MethodHandles;
import java.util.EnumSet;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object that represents store game tiles in store pages including
 * 'Deals', 'Browse Games', and 'Origin Access Vault'.
 *
 * @author mkalaivanan
 * @author palui
 */
public class StoreGameTile extends EAXVxSiteTemplate {

    // Use XPATH selector in order to access ancestor web elements
    protected static final String TILES_XPATH = "//div[contains(@class,'origin-storegametile')]";
    protected static final String TILES_DETAILS_XPATH = TILES_XPATH + "/section[contains(@class,'origin-storegametile-details')]";
    protected static final String TILE_OFFER_ID_XPATH_TMPL = TILES_DETAILS_XPATH
            + "//div[contains(@class,'origin-storeprice') and @offer-id='%s']";
    protected static final String TILE_STANDARD_PRICE_XPATH_TMPL = TILE_OFFER_ID_XPATH_TMPL + "/div[contains(@class, 'origin-storeprice-standard')]/span[contains(@class, 'otkprice')]";
    protected static final String TILE_SAVE_PERCENTAGE_XPATH_TMPL = TILE_OFFER_ID_XPATH_TMPL + "/div[contains(@class, 'origin-storeprice-standard')]/span[contains(@class, 'otkprice-sale')]";
    protected static final String TILE_FREE_PRICE_XPATH_TMPL = TILE_OFFER_ID_XPATH_TMPL + "/span[contains(@ng-if, 'showFreeText')]/span[contains(@class, 'origin-storeprice-free')]";
    protected static final String TILE_ACCESS_PRICE_XPATH_TMPL = TILE_OFFER_ID_XPATH_TMPL
            + "/div[contains(@class, 'origin-storeprice-access')]/div[contains(@class, 'origin-storeprice-originalprice')]/span[contains(@ng-bind, 'model.formattedPrice')]";
    protected static final String TILE_OWNED_PRICE_XPATH_TMPL = TILE_OFFER_ID_XPATH_TMPL + "/div[contains(@class, 'origin-storeprice-owned')]";
    protected static final String TILE_DETAILS_XPATH_TMPL = TILE_OFFER_ID_XPATH_TMPL + "/..";
    protected static final String TILE_XPATH_TMPL = TILE_DETAILS_XPATH_TMPL + "/..";
    protected static final String TILE_OCDPATH_XPATH_TMPL = TILE_XPATH_TMPL + "/../..";
    protected static final String TILE_TITLE_XPATH_TMPL = TILE_DETAILS_XPATH_TMPL + "/h2[contains(@class, 'origin-storegametile-title')]/span";

    protected static final String TILE_VIOLATOR_CSS_TMPL = "origin-store-game-tile .origin-storegametile[data-telemetry-label-offerid='%s'] .origin-storegametile-violator span";

    protected static final String TILE_OVERLAY_WRAPPER_XPATH_TMPL = TILE_DETAILS_XPATH_TMPL
            + "/../../a/div[contains(@class,'origin-storegametile-posterart-wrapper')]";
    protected static final String TILE_IMG_XPATH_TMPL = TILE_OVERLAY_WRAPPER_XPATH_TMPL
            + "//div[contains(@class,'origin-storegametile-posterart-container')]/img";

    protected static final String TILE_OVERLAY_XPATH_TMPL = TILE_DETAILS_XPATH_TMPL
            + "/../../a/div[contains(@class,'origin-storegametile-overlay')]/div[contains(@class,'origin-storegametile-overlaycontent')]";
    protected static final String TILE_CTA_XPATH_TMPL = TILE_OVERLAY_XPATH_TMPL
            + "/div[contains(@class,'origin-storegametile-cta')]";
    protected static final String TILE_LEARN_MORE_BUTTON_XPATH_TMPL = TILE_CTA_XPATH_TMPL
            + "//div[contains(@class,'otkbtn-primary-text') and text()='Learn More']";
    protected static final String TILE_WISHLIST_XPATH_TMPL = TILE_OVERLAY_XPATH_TMPL
            + "/origin-store-game-wishlist";
    protected static final String TILE_ADD_TO_WISHLIST_XPATH_TMPL = TILE_WISHLIST_XPATH_TMPL
            + "//p[contains(@class,'origin-store-game-wishlist-add')]";
    protected static final String TILE_HEART_OUTLINE_XPATH_TMPL = TILE_ADD_TO_WISHLIST_XPATH_TMPL
            + "//i[contains(@class,'otkicon-heart-outlined')]";
    protected static final String TILE_REMOVE_FROM_WISHLIST_XPATH_TMPL = TILE_WISHLIST_XPATH_TMPL
            + "//p[contains(@class,'origin-store-game-wishlist-remove')]";
    protected static final String TILE_HEART_XPATH_TMPL = TILE_REMOVE_FROM_WISHLIST_XPATH_TMPL
            + "//i[contains(@class,'otkicon-heart')]";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    protected final String offerId;
    protected final By tileLocator;
    protected final By tileTitleLocator;
    protected final By tileOcdPathLocator;
    protected final By tileSavePercentageLocator;
    protected final By tileStandardPriceLocator;
    protected final By tileFreePriceLocator;
    protected final By tileAccessPriceLocator;
    protected final By tileOwnedPriceLocator;
    protected final By tileViolatorLocator;
    protected final By tileImageLocator;
    protected final By tileLearnMoreButtonLocator;
    protected final By tileAddToWishlistLocator;
    protected final By tileHeartOutlineLocator;
    protected final By tileRemoveFromWishlistLocator;
    protected final By tileHeartLocator;

    /**
     * Enum for Store Game Tile types. Includes an "INVALID" type
     */
    public enum TileType {
        STANDARD, ACCESS, FREE, OWNED, INVALID
    }

    // EnumSet for valid Store Game Tile types only
    public static EnumSet<TileType> ValidTileType = EnumSet.of(
            TileType.STANDARD, TileType.ACCESS, TileType.FREE, TileType.OWNED);

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     * @param offerId Name of the entitlement on the game tile
     */
    public StoreGameTile(WebDriver driver, String offerId) {
        super(driver);

        this.offerId = offerId;
        this.tileLocator = By.xpath(String.format(TILE_XPATH_TMPL, offerId));
        this.tileOcdPathLocator = By.xpath(String.format(TILE_OCDPATH_XPATH_TMPL, offerId));
        this.tileTitleLocator = By.xpath(String.format(TILE_TITLE_XPATH_TMPL, offerId));
        this.tileSavePercentageLocator = By.xpath(String.format(TILE_SAVE_PERCENTAGE_XPATH_TMPL, offerId));
        this.tileStandardPriceLocator = By.xpath(String.format(TILE_STANDARD_PRICE_XPATH_TMPL, offerId));
        this.tileFreePriceLocator = By.xpath(String.format(TILE_FREE_PRICE_XPATH_TMPL, offerId));
        this.tileAccessPriceLocator = By.xpath(String.format(TILE_ACCESS_PRICE_XPATH_TMPL, offerId));
        this.tileOwnedPriceLocator = By.xpath(String.format(TILE_OWNED_PRICE_XPATH_TMPL, offerId));
        this.tileViolatorLocator = By.cssSelector(String.format(TILE_VIOLATOR_CSS_TMPL, offerId));
        this.tileImageLocator = By.xpath(String.format(TILE_IMG_XPATH_TMPL, offerId));
        this.tileLearnMoreButtonLocator = By.xpath(String.format(TILE_LEARN_MORE_BUTTON_XPATH_TMPL, offerId));
        this.tileAddToWishlistLocator = By.xpath(String.format(TILE_ADD_TO_WISHLIST_XPATH_TMPL, offerId));
        this.tileHeartOutlineLocator = By.xpath(String.format(TILE_HEART_OUTLINE_XPATH_TMPL, offerId));
        this.tileRemoveFromWishlistLocator = By.xpath(String.format(TILE_REMOVE_FROM_WISHLIST_XPATH_TMPL, offerId));
        this.tileHeartLocator = By.xpath(String.format(TILE_HEART_XPATH_TMPL, offerId));
    }

    /////////////////////////////////
    // Getters
    /////////////////////////////////
    /**
     * Get the store game tile as a WebElement.
     *
     * @return WebElement for the store game tile
     */
    protected WebElement getTileWebElement() {
        return waitForElementVisible(tileLocator);
    }

    /**
     * Verify that the pack art for a store tile is visible.
     *
     * @return true if the pack art is visible, false otherwise
     */
    public boolean verifyPackArtVisible() {
        return waitIsElementVisible(tileImageLocator);
    }

    /**
     * Get title of the store game tile.
     *
     * @return Title of the tile
     */
    public String getTitle() {
        return waitForElementVisible(tileTitleLocator).getText();
    }

    /**
     * Get the offer ID of the store game tile.
     *
     * @return offer ID of the tile
     */
    public String getOfferId() {
        return offerId;
    }

    /**
     * Verify that the title for a store tile is visible.
     *
     * @return true if the title is visible, false otherwise
     */
    public boolean verifyTitleVisible() {
        return waitIsElementVisible(tileTitleLocator);
    }

    /**
     * Get OCD path of the store game tile.
     *
     * @return OCD path of the tile
     */
    public String getOcdPath() {
        return waitForElementVisible(tileOcdPathLocator).getAttribute("ocd-path");
    }

    /**
     * Verify store game tile's actual type against the expected type.
     *
     * @param type The expected TileType
     * @return true if expected type matches and the type is not 'INVALID',
     * false otherwise
     */
    private boolean isTileType(TileType type) {
        List<WebElement> elementList;
        switch (type) {
            case STANDARD:
                elementList = driver.findElements(tileStandardPriceLocator);
                return !elementList.isEmpty() && isElementVisible(elementList.get(0));
            case ACCESS:
                elementList = driver.findElements(tileAccessPriceLocator);
                return !elementList.isEmpty() && isElementVisible(elementList.get(0));
            case FREE:
                elementList = driver.findElements(tileFreePriceLocator);
                return !elementList.isEmpty() && elementList.get(0).getAttribute("textContent").equals("FREE");
            case OWNED:
                elementList = driver.findElements(tileOwnedPriceLocator);
                return !elementList.isEmpty() && isElementVisible(elementList.get(0));
            default:
                _log.error("Cannot check store game tile against an invalid type: " + type);
                return false;
        }
    }

    /**
     * Get the type of the store game tile. This is done by looking at the price
     * portion which can be a standard price, an Origin Access "or" price, or a
     * FREE price.
     *
     * @return TileType, which can be STANDARD, ACCESS, FREE, or INVALID
     */
    public TileType getTileType() {
        for (TileType type : ValidTileType) {
            if (isTileType(type)) {
                return type;
            }
        }
        return TileType.INVALID;
    }

    /**
     * Get price of a 'STANDARD' store game tile.
     *
     * @return String price (e.g. $4.99) of a "STANDARD" store game tile
     */
    private String getStandardPrice() {
        return waitForElementVisible(tileStandardPriceLocator).getAttribute("textContent");
    }

    /**
     * Get price of an 'ACCESS' store game tile.
     *
     * @return String 'or' price (e.g. $4.99 from "Included with access or
     * $4.99" ) of an "ACCESS" store game tile.
     */
    private String getAccessPrice() {
        return waitForElementVisible(tileAccessPriceLocator).getAttribute("textContent");
    }

    /**
     * Private utility to convert from dollar String (e.g. $4.99) to a float
     * (e.g. 4.99f).
     *
     * @param dollarString String to convert
     * @return Float value of the dollarString
     */
    private float dollarStringToFloat(String dollarString) {
        return Float.parseFloat(dollarString.trim().substring(1));
    }

    /**
     * Get price of a store game tile as a float.
     *
     * @return Price for a standard tile, 'or' price for an access tile, or 0.0f
     * for 'FREE' games
     */
    public float getPrice() {
        TileType type = getTileType();
        switch (type) {
            case STANDARD:
                return dollarStringToFloat(getStandardPrice());
            case ACCESS:
                return dollarStringToFloat(getAccessPrice());
            case FREE:
            case INVALID:
                // Placeholder tiles on QA will be invalid, treat them as free
                return 0.0f;
            default:
                throw new RuntimeException("TileType: " + type + " of " + offerId
                        + " does not have an associated price method");
        }
    }

    /**
     * Verify that the price for a store tile is visible.
     *
     * @return true if the price is visible, false otherwise
     */
    public boolean verifyPriceVisible() {
        switch (getTileType()) {
            case STANDARD:
                return waitIsElementVisible(tileStandardPriceLocator);
            case ACCESS:
                return waitIsElementVisible(tileAccessPriceLocator);
            case FREE:
                return waitIsElementVisible(tileFreePriceLocator);
            case OWNED:
                return waitIsElementVisible(tileOwnedPriceLocator);
            default:
                String errorMessage = "Cannot determine tile type (Standard, Access, Free, or Owned) to get price for the store game tile: " + offerId;
                _log.error(errorMessage);
                throw new RuntimeException(errorMessage);
        }
    }

    /////////////////////////////////
    // Actions on the store game tile
    /////////////////////////////////
    /**
     * Scroll to the store game tile.
     */
    public void scrollToTile() {
        scrollToElement(waitForElementVisible(tileImageLocator));
    }

    /**
     * Hover on image of the store game tile.
     *
     */
    public void hoverOnTileImage() {
        scrollToTile();
        hoverOnElement(waitForElementVisible(tileImageLocator));
    }

    /**
     * Click on tile of the store game tile.<br>
     * NOTE: the entire tile is clickable, but the image and title are not
     * clickable.
     */
    public void clickOnTile() {
        scrollToTile();
        waitForElementClickable(tileLocator).click();
    }

    /**
     * Hover on image of the store game tile and click 'Learn More' button.
     */
    public void clickLearnMoreButton() {
        hoverOnTileImage();
        waitForElementClickable(tileLearnMoreButtonLocator).click();
    }

    /**
     * Hover on the image of the store game tile and click the 'Add to Wishlist'
     * option.
     */
    public void clickAddToWishlist() {
        hoverOnTileImage();
        waitForElementClickable(tileAddToWishlistLocator).click();
    }

    /**
     * Hover on the image of the store game tile and click the 'Added to
     * Wishlist' option.
     */
    public void clickRemoveFromWishlist() {
        hoverOnTileImage();
        waitForElementClickable(tileRemoveFromWishlistLocator).click();
    }

    /**
     * Hover on the image of the store game tile and verify if the offer can be
     * added to the user's wishlist.
     *
     * @return true if the add to wishlist option is displayed, false otherwise
     */
    public boolean verifyAddToWishlistVisible() {
        hoverOnTileImage();
        return waitIsElementVisible(tileAddToWishlistLocator) && waitIsElementVisible(tileHeartOutlineLocator);
    }

    /**
     * Hover on the image of the store game tile and verify if the offer is on
     * the user's wishlist.
     *
     * @return true if the remove from wishlist option is displayed, false
     * otherwise
     */
    public boolean verifyRemoveFromWishlistVisible() {
        hoverOnTileImage();
        return waitIsElementVisible(tileRemoveFromWishlistLocator) && waitIsElementVisible(tileHeartLocator);
    }

    /**
     * Get Origin Access discount percentage.
     *
     * @return Percentage if exists on the store game tile. RuntimeException, if
     * not found
     */
    public int getSavePercentage() {
        String text = StringHelper.removeNonDigits(waitForElementVisible(tileSavePercentageLocator).getText());
        if (text.isEmpty()) {
            String errorMessage = "unable to get save percentage for this tile for : " + offerId;
            _log.error(errorMessage);
            throw new RuntimeException(errorMessage);
        }
        return Integer.parseInt(text);
    }

    /**
     * Verify discount percentage exists on the store game tile.
     *
     * @return true if the discount percentage exists false, if not exist
     */
    public boolean verifyDiscountPercentageExists() {
        return waitIsElementVisible(tileSavePercentageLocator);
    }
}
