package com.ea.originx.automation.lib.pageobjects.profile;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.*;

/**
 * Represents a Wishlist tile on the profile Wishlist Tab
 *
 * @author nvarthakavi
 * @author palui
 */
public class WishlistTile extends EAXVxSiteTemplate {

    private final WebElement rootElement;

    // The following CSS selectors are for child elements within the rootElement
    private static final String TILE_PRIMARY_COL_CSS = ".origin-gamecard-primarycol";
    private static final By PACK_ART_LOCATOR = By.cssSelector(TILE_PRIMARY_COL_CSS + " .origin-profile-wishlist-gametile-packart");

    private static final String TILE_SECONDARY_COL_CSS = ".origin-profile-wishlist-secondarycol";
    private static final By TITLE_LOCATOR = By.cssSelector(TILE_SECONDARY_COL_CSS + " .origin-profile-wishlist-gametile-title");
    private static final By DATE_ADDED_LOCATOR = By.cssSelector(TILE_SECONDARY_COL_CSS + " .origin-profile-wishlist-date-added");

    private static final String WISHLIST_CTA_CONTAINER_CSS = TILE_SECONDARY_COL_CSS + " .origin-profile-wishlist-gametile-cta-container";
    private static final String BUY_BUTTON_CSS = WISHLIST_CTA_CONTAINER_CSS + " origin-cta-purchase button";
    private static final By BUY_BUTTON_LOCATOR = By.cssSelector(BUY_BUTTON_CSS);
    private static final By PLAY_ACCESS_BUTTON_TEXT_LOCATOR = By.cssSelector(WISHLIST_CTA_CONTAINER_CSS + " origin-cta-directacquisition div a div span");
    private static final By PLAY_ACCESS_BUTTON_LOCATOR = By.cssSelector(WISHLIST_CTA_CONTAINER_CSS + " origin-cta-directacquisition origin-cta-primary");
    private static final By REMOVE_FROM_WISHLIST_LINK_LOCATOR = By.cssSelector(WISHLIST_CTA_CONTAINER_CSS + " .origin-profile-wishlist-gametile-remove-link");
    private static final By PURCHASE_AS_GIFT_BUTTON_LOCATOR = By.cssSelector(WISHLIST_CTA_CONTAINER_CSS + " .origin-gifting-cta button");

    private static final By INELIGIBLE_MESSAGE_LOCATOR = By.cssSelector(WISHLIST_CTA_CONTAINER_CSS + " .origin-profile-wishlist-ineligibletoreceive-msg");
    private static final By ORIGIN_ACCESS_DISCOUNT_MESSAGE_LOCATOR = By.cssSelector(WISHLIST_CTA_CONTAINER_CSS + " .origin-gamecard-accessdiscount");
    protected static final String[] ORIGIN_ACCESS_DISCOUNT_KEYWORDS = {"origin", "access", "discount"};

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     * @param rootElement Root element of this 'Wishlist' tile
     */
    protected WishlistTile(WebDriver driver, WebElement rootElement) {
        super(driver);
        this.rootElement = rootElement;
    }

    /**
     * Verify 'Wishlist' tile is visible.
     *
     * @return true if this 'Wishlist' tile is visible, false otherwise
     */
    public boolean verifyWishlistTileVisible() {
        return waitIsElementVisible(rootElement, 2);
    }

    /**
     * Get the offer ID of the current 'Wishlist' tile.
     *
     * @return Offer ID of the current 'Wishlist' tile
     */
    public String getOfferId() {
        return rootElement.getAttribute("data-telemetry-offer-id");
    }

    /**
     * Verify the offer ID is as expected.
     *
     * @param offerId Expected offer ID
     * @return true if this 'Wishlist' tile has matching offer ID, false otherwise
     */
    public boolean verifyOfferId(String offerId) {
        return getOfferId().equalsIgnoreCase(offerId);
    }

    /**
     * Get the child element given its locator.
     *
     * @param childLocator Child locator within rootElement of this 'Wishlist'
     * tile
     * @return Child WebElement if found, return null if not found
     */
    private WebElement getChildElement(By childLocator) {
        try {
            return rootElement.findElement(childLocator);
        } catch (NoSuchElementException e) {
            _log.error("Unable to find child element");
            return null;
        }
    }

    /**
     * Get the child element's text given it's locator.
     *
     * @param childLocator Child locator within rootElement of this 'Wishlist'
     * tile
     * @return Child WebElement text if found, or null if not found
     */
    private String getChildElementText(By childLocator) {
        WebElement childElement = getChildElement(childLocator);
        if (childElement == null) {
            return null;
        }

        scrollToElement(childElement);
        return childElement.getAttribute("textContent");
    }

    /**
     * Click the child WebElement as specified by the child locator.
     *
     * @param childLocator Child locator within rootElement of this 'Wishlist'
     * tile
     */
    private void clickChildElement(By childLocator) {
        waitForElementClickable(getChildElement(childLocator)).click();
    }

    /**
     * Verify the child element is visible given it's locator.
     *
     * @param childLocator Child locator within rootElement of this 'Wishlist'
     * tile
     * @return true if childElement is visible, false otherwise
     */
    private boolean verifyChildElementVisible(By childLocator) {
        WebElement childElement = getChildElement(childLocator);
        if (childElement == null) {
            return false;
        }
        return waitIsElementVisible(childElement);
    }

    /**
     * Scroll to child WebElement and hover on it.
     *
     * @param childLocator Child locator within rootElement of this 'Wishlist'
     * tile
     */
    private void scrollToAndHoverOnChildElement(By childLocator) {
        WebElement childElement = waitForElementVisible(getChildElement(childLocator));
        scrollToElement(childElement);
        sleep(500);
        hoverOnElement(childElement);
        sleep(500);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Pack Art
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Get the pack art name of this 'Wishlist' tile.
     *
     * @return Pack art name of this 'Wishlist' tile
     */
    public String getPackArtName() {
        return getChildElement(PACK_ART_LOCATOR).getAttribute("alt");
    }

    /**
     * Click the pack art of this 'Wishlist' tile
     */
    public void clickPackArt() {
        clickChildElement(PACK_ART_LOCATOR);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Product Title
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Get the product title of this 'Wishlist' tile.
     *
     * @return Product title text of this 'Wishlist' tile
     */
    public String getProductTitle() {
        return getChildElementText(TITLE_LOCATOR);
    }

    /**
     * Click product tile of this 'Wishlist' tile.
     */
    public void clickProductTitle() {
        clickChildElement(TITLE_LOCATOR);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Added Date text
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Get the 'Added Date' text of this 'Wishlist' tile.
     *
     * @return Added date text of this 'Wishlist' tile, or null if not found
     */
    public String getAddedDateText() {
        return getChildElementText(DATE_ADDED_LOCATOR);
    }

    /**
     * Verify the 'Date Added' keyword appears in this 'Wishlist' tile.
     *
     * @return true if the 'Date Added' keyword appears in the Wishlist tile, false
     * otherwise
     */
    public boolean verifyDateAddedKeyword() {
        return StringHelper.containsIgnoreCase(getAddedDateText(), "Added");
    }

    ////////////////////////////////////////////////////////////////////////////
    //Ineligible message
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Get 'Ineligible' message.
     *
     * @return Ineligible message text of this 'Wishlist' tile, or null if not
     * found
     */
    public String getIneligibleMessage() {
        return getChildElementText(INELIGIBLE_MESSAGE_LOCATOR);
    }

    /**
     * Verify the 'Ineligible' message keyword appears in this 'Wishlist' tile.
     *
     * @return true if the 'Ineligible' message keyword appears in the 'Wishlist'
     * tile, false otherwise
     */
    public boolean verifyIneligibleMessage() {
        return StringHelper.containsIgnoreCase(getIneligibleMessage(), "Ineligible");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Discounted price
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Checks if this 'Wishlist' tile is discounted.
     *
     * @return true if the tile is discounted (through Origin Access or because
     * of a sale), false otherwise
     */
    public boolean isDiscounted() {
        return rootElement.findElements(By.className("otkprice-sale")).stream().anyMatch(WebElement::isDisplayed);
    }

    /**
     * Verify that the 'Origin Access Discount Applied' is provided for an Origin
     * Access user
     *
     * @return true if the expected keywords are found in the message, false
     * otherwise
     */
    public boolean verifyOriginAccessDiscountIsVisible() {
        try {
            String header = waitForElementVisible(ORIGIN_ACCESS_DISCOUNT_MESSAGE_LOCATOR, 10).getText();
            return StringHelper.containsIgnoreCase(header, ORIGIN_ACCESS_DISCOUNT_KEYWORDS);
        } catch (TimeoutException e) {
            return false;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Buy Button
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Verify that the buy button is visible
     * 
     * @return true if 'Buy' button is visible, false otherwise
     */
    public boolean verifyBuyButtonVisible() {
        return waitIsElementVisible(BUY_BUTTON_LOCATOR);
    }
       
    /**
     * Get the 'Buy' button text of this 'Wishlist' tile.
     *
     * @return 'Buy' button text of this 'Wishlist' item
     */
    public String getBuyButtonText() {
        return getChildElementText(BUY_BUTTON_LOCATOR);
    }

    /**
     * Verify the 'Buy' button text is as expected.
     *
     * @return true if the 'Buy' button text in this 'Wishlist' tile contains the
     * keyword, false otherwise
     */
    public boolean verifyBuyButtonTextMatches() {
        return StringHelper.containsIgnoreCase(getBuyButtonText(), "Buy");
    }

    /**
     * Verify the price is shown in the 'Buy' button.
     *
     * @return true if the 'Buy' button text exists with a price in this
     * 'Wishlist' tile, false otherwise
     */
    public boolean verifyPriceInBuyButtonText() {
        String buyButtonText = getBuyButtonText();
        if (buyButtonText == null) {
            return false;
        } else {
            final Double price = StringHelper.extractNumberFromText(buyButtonText);
            return !price.isNaN();
        }
    }

    /**
     * Hover on the 'Buy' button.
     */
    public void hoverOnBuyButton() {
        scrollToAndHoverOnChildElement(BUY_BUTTON_LOCATOR);
    }

    /**
     * Click the 'Buy' button of this 'Wishlist' tile.
     */
    public void clickBuyButton() {
        clickChildElement(BUY_BUTTON_LOCATOR);
    }

    ////////////////////////////////////////////////////////////////////////////
    // 'Play Access' Button
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Get the 'Play Access' button text.
     *
     * @return 'Play Access' button text of this 'Wishlist' tile, or null if not
     * found
     */
    public String getPlayAccessButtonText() {
        return getChildElementText(PLAY_ACCESS_BUTTON_TEXT_LOCATOR);
    }

    /**
     * Verify the 'Play Access' button text is as expected.
     *
     * @return true if the 'Play Access' button text contains the keyword for
     * this 'Wishlist' tile, false otherwise
     */
    public boolean verifyPlayAccessButtonTextMatches() {
        return StringHelper.containsIgnoreCase(getPlayAccessButtonText(), "Play");
    }

    /**
     * Click the 'Play Access' button of this 'Wishlist' tile.
     */
    public void clickPlayAccessButton() {
        clickChildElement(PLAY_ACCESS_BUTTON_LOCATOR);
    }

    /**
     * Verify the 'Play Access' button exists.
     *
     * @return true if 'Play Access' button of this 'Wishlist' tile exists,
     * false otherwise
     */
    public boolean verifyPlayWithAccessButtonExists() {
        return waitIsElementVisible(PLAY_ACCESS_BUTTON_LOCATOR);
    }

    ////////////////////////////////////////////////////////////////////////////
    // 'Purchase As Gift' Button
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Get the 'Purchase As Gift' button text.
     *
     * @return 'Purchase As Gift' button text of this 'Wishlist' tile, or null if
     * not found
     */
    public String getPurchaseAsGiftButtonText() {
        return getChildElementText(PURCHASE_AS_GIFT_BUTTON_LOCATOR);
    }

    /**
     * Verify the 'Purchase As Gift' button text is as expected.
     *
     * @return true if the 'Purchase As Gift' button text contains the keyword
     * for this 'Wishlist' tile, false otherwise
     */
    public boolean verifyPurchaseAsAGiftButtonTextMatches() {
        final String[] EXPECTED_PURCHASE_AS_A_GIFT_BUTTON_KEYWORDS = {"Purchase", "Gift"};
        return StringHelper.containsIgnoreCase(getPurchaseAsGiftButtonText(), EXPECTED_PURCHASE_AS_A_GIFT_BUTTON_KEYWORDS);
    }

    /**
     * Verify the 'Purchase As Gift' button text has a price on it.
     *
     * @return true if the 'Purchase As Gift' button text exists with a price in
     * this 'Wishlist' tile, false otherwise
     */
    public boolean verifyPricePurchaseAsGiftTextMatches() {
        String asGiftButtonText = getPurchaseAsGiftButtonText();
        if (asGiftButtonText == null) {
            return false;
        } else {
            final Double price = StringHelper.extractNumberFromText(asGiftButtonText);
            return !price.isNaN();
        }
    }

    /**
     * Click the 'Purchase As Gift' button of this 'Wishlist' tile.
     */
    public void clickPurchaseAsGiftButton() {
        clickChildElement(PURCHASE_AS_GIFT_BUTTON_LOCATOR);
    }

    /**
     * Hover on the 'Purchase As Gift' button.
     */
    public void hoverOnPurchaseAsGiftButton() {
        scrollToAndHoverOnChildElement(PURCHASE_AS_GIFT_BUTTON_LOCATOR);
    }

    ////////////////////////////////////////////////////////////////////////////
    // 'Remove From Wishlist' link
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Verify the 'Remove From Wishlist' link is visible.
     *
     * @return true if this 'Wishlist' tile has a 'Remove from Wishlist' link,
     * false otherwise
     */
    public boolean verifyRemoveFromWishlistLinkVisible() {
        return verifyChildElementVisible(REMOVE_FROM_WISHLIST_LINK_LOCATOR);
    }

    /**
     * Click 'Remove From Wishlist' link for this 'Wishlist' tile.
     */
    public void clickRemoveFromWishlistLink() {
        clickChildElement(REMOVE_FROM_WISHLIST_LINK_LOCATOR);
        sleep(2000); // wait for the Wishlist tile to be removed
    }
}