package com.ea.originx.automation.lib.pageobjects.profile;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the 'Wishlist' tab on the 'Profile' page.
 *
 * @author nvarthakavi
 * @author palui
 */
public class ProfileWishlistTab extends EAXVxSiteTemplate {

    private static final String WISHLIST_TILES_CSS = ".origin-profile-wishlist-container .origin-profile-wishlist .origin-profile-wishlist-gametile";
    private static final By WISHLIST_TILES_LOCATOR = By.cssSelector(WISHLIST_TILES_CSS);
    protected static final String WISHLIST_TILE_CSS_TMPL = WISHLIST_TILES_CSS + "[data-telemetry-offer-id='%s']";

    private static final By EMPTY_WISHLIST_HEADING_LOCATOR = By.cssSelector(".origin-profile-wishlist-empty .origin-profile-wishlist-empty-heading");
    private static final By PRIVATE_PROFILE_NOTIFICATION_LOCATOR = By.cssSelector(".origin-profile-wishlist-private-stripe");
    private static final By SHARE_YOUR_WISHLIST_BUTTON_LOCATOR = By.cssSelector(".origin-profile-wishlist-share-textcontainer button");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ProfileWishlistTab(WebDriver driver) {
        super(driver);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Wishlist tile getters and verifiers
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Get a list of all the Wishlist tile WebElements.
     *
     * @return List of all WishList tile WebElements on this 'Wishlist' tab
     */
    public List<WebElement> getAllWishlistTileElements() {
        waitForWishlistTilesToLoad();
        return driver.findElements(WISHLIST_TILES_LOCATOR);
    }

    /**
     * Wait for the 'Wishlist' tiles to load.
     */
    public void waitForWishlistTilesToLoad() {
        // Wait for an initial tile to load with its offer id
        try {
            Waits.pollingWaitEx(() -> waitForElementVisible(WISHLIST_TILES_LOCATOR).getAttribute("data-telemetry-offer-id") != null, 5000, 1000, 0);
        } catch (TimeoutException e) {
            _log.debug("No tiles loaded");
            return;
        }

        // Wait for the number of tiles returned to be consistent
        int nTries = 5;
        int currentWishlistTileListSize = driver.findElements(WISHLIST_TILES_LOCATOR).size();
        int nextWishlistTileListSize;
        for (int i = 0; i < nTries; i++) {
            Waits.sleep(3000); // Wait for more tiles to load
            nextWishlistTileListSize = driver.findElements(WISHLIST_TILES_LOCATOR).size();
            if (currentWishlistTileListSize == nextWishlistTileListSize) {
                _log.debug("currentWishlistTileListSize: " + currentWishlistTileListSize);
                return;
            }
            currentWishlistTileListSize = driver.findElements(WISHLIST_TILES_LOCATOR).size();
        }
        throw new RuntimeException("Failed to load wishlist tiles");
    }

    /**
     * Get a list of all the WishList tile WebElements given an offer ID.
     *
     * @param offerId Entitlement offer ID
     * @return Wishlist tile WebElement for the given offerId on this 'Wishlist'
     * tab, or return null if not found
     */
    private WebElement getWishlistTileElement(String offerId) {
        try {
            scrollToElement(waitForElementVisible(By.cssSelector(String.format(WISHLIST_TILE_CSS_TMPL, offerId)))); //added this scroll as the element is hidden
            return driver.findElement(By.cssSelector(String.format(WISHLIST_TILE_CSS_TMPL, offerId)));
        } catch (TimeoutException e) {
            _log.error("Unable to find tile based on offerid: " + offerId);
            return null;
        }

    }

    /**
     * Get a wishlist tile given an offer ID.
     *
     * @param offerId Entitlement offer ID
     * @return WishlistTile for the given offer ID on this 'Wishlist' tab, or
     * null if not found
     */
    public WishlistTile getWishlistTile(String offerId) {
        try {
            return new WishlistTile(driver, getWishlistTileElement(offerId));
        } catch (NullPointerException e) {
            _log.error("Unable to find Wishlist tile based on offerID: " + offerId);
            return null;
        }
    }

    /**
     * Get a wishlist tile given an entitlement.
     *
     * @param entitlement EntitlementInfo of the entitlement
     * @return WishlistTile for the given entitlement on this 'Wishlist' tab, or
     * null if not found
     */
    public WishlistTile getWishlistTile(EntitlementInfo entitlement) {
        return getWishlistTile(entitlement.getOfferId());
    }

    /**
     * Get a list of wishlist tiles given a list of offer IDs.
     *
     * @param offerids Offer IDs of the entitlements to get from the 'Wishlist' tiles on the
     * 'Wishlist' tab
     * @return List of 'Wishlist' tiles for the given entitlements
     */
    private List<WishlistTile> getWishlistTiles(List<String> offerids) {
        List<WishlistTile> wishlistTiles = new ArrayList<>();
        for (String offerid : offerids) {
            wishlistTiles.add(getWishlistTile(offerid));
        }
        return wishlistTiles;
    }

    /**
     * Get a list of all the wishlist tiles.
     *
     * @return List of all 'Wishlist' tiles on this 'Wishlist' tab
     */
    public List<WishlistTile> getAllWishlistTiles() {
        List<WishlistTile> wishlistTiles = getAllWishlistTileElements().
                stream().map(webElement -> new WishlistTile(driver, webElement)).collect(Collectors.toList());
        return wishlistTiles;
    }

    /**
     * Get a list of all the offer IDs of all the 'Wishlist' tiles.
     *
     * @return List of all the 'Wishlist' tile offer IDs on this 'Wishlist' tab
     */
    private List<String> getAllOfferIds() {
        scrollToBottom();
        List<WebElement> wishlistTileElements = getAllWishlistTileElements();
        List<String> offerids = new ArrayList<>();
        for (WebElement wishlistTile : wishlistTileElements) {
            offerids.add(wishlistTile.getAttribute("data-telemetry-offer-id"));
        }
        return offerids;
    }

    /**
     * Checks if the 'Wishlist' is empty.
     *
     * @return true if the 'Wishlist' is empty, false otherwise
     */
    public boolean isEmptyWishlist(WebDriver driver) {
        boolean isEmptyWishlistHeadingVisible = driver.findElement(EMPTY_WISHLIST_HEADING_LOCATOR).isDisplayed();
        boolean isEmptyWishlistDescriptionVisible = driver.findElement(EMPTY_WISHLIST_HEADING_LOCATOR).isDisplayed();
        List<WishlistTile> wishlistTiles = getAllWishlistTiles();
        boolean isTilesExist = wishlistTiles.isEmpty();
        return isEmptyWishlistHeadingVisible && isEmptyWishlistDescriptionVisible && isTilesExist;
    }

    /**
     * Verify if a list of entitlement(s) all exists in this 'Wishlist' tab.
     *
     * @param offerIds List of Offer IDs to look for
     * @return true if ALL offer IDs appear in the list of 'Wishlist' tiles, false
     * otherwise
     */
    public boolean verifyTilesExist(String... offerIds) {
        if (offerIds == null) {
            throw new RuntimeException("List of offerIds must not be null");
        }
        List<String> allOfferIds = getAllOfferIds();
        for (String offerid : offerIds) {
            if (!allOfferIds.contains(offerid)) {
                return false;
            }
        }
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Buy Button
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Verify if 'Wishlist' tiles for all the given entitlement offer IDs have a
     * 'Buy' button.
     *
     * @param entitlementOfferIds List of entitlement Offer IDs to look for the
     * button
     * @return true if all entitlement offerIds have corresponding 'Wishlist'
     * tiles with the button, false otherwise
     */
    public boolean verifyBuyButtonsExist(List<String> entitlementOfferIds) {
        List<WishlistTile> allWishlistItems = getWishlistTiles(entitlementOfferIds);
        if (allWishlistItems.isEmpty()) {
            _log.error("No Wishlist tiles were found");
            return false;
        }

        return allWishlistItems.stream().noneMatch((wishlistTile) -> (wishlistTile == null || !wishlistTile.verifyBuyButtonVisible()));
    }

    /**
     * Verify if 'Wishlist' tiles for all the given entitlements have a 'Buy'
     * button.
     *
     * @param entitlementInfos Entitlements to look for the button
     * @return true if all entitlement offerIds have corresponding 'Wishlist'
     * tiles with the button, false otherwise
     */
    public boolean verifyBuyButtonsExist(EntitlementInfo... entitlementInfos) {
        if (entitlementInfos == null) {
            throw new RuntimeException("List of entitlementInfos must not be null");
        }
        return verifyBuyButtonsExist(EntitlementInfoHelper.getEntitlementOfferIds(entitlementInfos));
    }

    /**
     * Verify if tiles in this 'Wishlist' tab corresponding to the given
     * entitlement have a price in the 'Buy' button.
     *
     * @param entitlementOfferIds Entitlements to look for a price in the button
     * @return true if all tiles corresponding to the entitlements have price in
     * the button, false otherwise
     */
    public boolean verifyPriceForBuyButtonsExist(List<String> entitlementOfferIds) {
        List<WishlistTile> allWishlistItems = getAllWishlistTiles();

        if (allWishlistItems.isEmpty()) {
            throw new RuntimeException("Could not get the wishlist items");
        }
        for (WishlistTile wishlistTile : allWishlistItems) {
            for (String offerid : entitlementOfferIds) {
                if (wishlistTile.verifyOfferId(offerid)) {
                    if (!wishlistTile.verifyPriceInBuyButtonText()) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    /**
     * Verifies that the all wishlist items have a 'Buy' button.
     *
     * @return true if the all wishlist items have a 'Buy' button.
     */
    public boolean verifyBuyButtonsExistForAllItems() {
        List<WishlistTile> allWishlistItems = getAllWishlistTiles();
        for (WishlistTile wishlistTile : allWishlistItems) {
            if (!wishlistTile.verifyBuyButtonTextMatches()) {
                return false;
            }
        }
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////
    // 'Purchase As Gift' Button
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Verify if 'Wishlist' tiles for all the given entitlement offer IDs have a
     * 'Purchase As Gift' button.
     *
     * @param entitlementOfferIds List of entitlement Offer IDs to look for
     * @return true if all entitlement offer IDs have corresponding 'Wishlist'
     * tiles with the button, false otherwise
     */
    public boolean verifyPurchaseAsAGiftButtonsExist(List<String> entitlementOfferIds) {
        List<WishlistTile> allWishlistItems = getWishlistTiles(entitlementOfferIds);

        if (allWishlistItems.isEmpty()) {
            throw new RuntimeException("Could not get the wishlist items");
        }
        for (WishlistTile wishlistTile : allWishlistItems) {
            if (!wishlistTile.verifyPurchaseAsAGiftButtonTextMatches()) {
                return false;
            }
        }
        return true;
    }

    /**
     * Verify if 'Wishlist' tiles for all the given entitlements have a 'Purchase
     * As Gift' button.
     *
     * @param entitlementInfos Entitlements to look for the button
     * @return true if all entitlement offer IDs have corresponding 'Wishlist'
     * tiles with the button, false otherwise
     */
    public boolean verifyPurchaseAsAGiftButtonsExist(EntitlementInfo... entitlementInfos) {
        if (entitlementInfos == null) {
            throw new RuntimeException("List of entitlementInfos must not be null");
        }
        return verifyPurchaseAsAGiftButtonsExist(EntitlementInfoHelper.getEntitlementOfferIds(entitlementInfos));
    }

    /**
     * Verify if tiles in this 'Wishlist' tab corresponding to the given
     * entitlements have a price in the 'Purchase As Gift' button.
     *
     * @param offerIds Entitlements to look for a price in the button
     * @return true if all tiles corresponding to the entitlements have price in
     * the button, false otherwise
     */
    public boolean verifyPriceForPurchaseAsGiftButtonsExist(List<String> offerIds) {
        List<WishlistTile> allWishlistItems = getWishlistTiles(offerIds);
        if (allWishlistItems.isEmpty()) {
            throw new RuntimeException("Could not get the wishlist items");
        }

        for (WishlistTile wishlistTile : allWishlistItems) {
            if (!wishlistTile.verifyPricePurchaseAsGiftTextMatches()) {
                return false;
            }
        }
        return true;
    }

    /**
     * Verify if 'Wishlist' tiles for all the given entitlements have a 'Purchase
     * As Gift' button.
     *
     * @param entitlementInfos Entitlements to look the button
     * @return true if all entitlements have corresponding 'Wishlist' tiles with
     * the 'Purchase As Gift' button, false otherwise
     */
    public boolean verifyPricePurchaseAsGiftExist(EntitlementInfo... entitlementInfos) {
        if (entitlementInfos == null) {
            throw new RuntimeException("List of entitlementInfos must not be null");
        }
        return verifyPriceForPurchaseAsGiftButtonsExist(EntitlementInfoHelper.getEntitlementOfferIds(entitlementInfos));
    }

    ////////////////////////////////////////////////////////////////////////////
    // 'Play Access' Button
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Verify if 'Wishlist' tiles for all the given entitlement offer IDs have a
     * 'Play Access' button.
     *
     * @param entitlementOfferIds List of entitlement Offer IDs to look for
     * @return true if all entitlement offer IDs have corresponding 'Wishlist'
     * tiles with the button, false otherwise
     */
    public boolean verifyPlayAccessButtonsExist(List<String> entitlementOfferIds) {
        List<WishlistTile> allWishlistItems = getWishlistTiles(entitlementOfferIds);

        if (allWishlistItems.isEmpty()) {
            _log.error("Could not find any Wishlist tiles");
            return false;
        }

        return allWishlistItems.stream().noneMatch((wishlistTile) -> (wishlistTile == null || !wishlistTile.verifyPlayWithAccessButtonExists()));
    }

    /**
     * Verify if 'Wishlist' tiles for all the given entitlements have a 'Play
     * Access' button.
     *
     * @param entitlementInfos Entitlements to look for the button
     * @return true if all entitlements have corresponding 'Wishlist' tiles with
     * the button, false otherwise
     */
    public boolean verifyPlayAccessButtonsExist(EntitlementInfo... entitlementInfos) {
        if (entitlementInfos == null) {
            throw new RuntimeException("List of entitlementInfos must not be null");
        }
        return verifyPlayAccessButtonsExist(EntitlementInfoHelper.getEntitlementOfferIds(entitlementInfos));
    }

    ////////////////////////////////////////////////////////////////////////////
    // Pack Art
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Verifies that the given entitlements has the correct pack art in the
     * 'Wishlist' tab.
     *
     * @param entitlementOfferId Offer ID of the DLC/entitlement
     * @param entitlementName Name of the DLC/entitlement
     * @return true if the 'Wishlist' tile for the entitlement was found with the
     * correct pack art
     */
    public boolean verifyPackArtExistsEntitlement(String entitlementOfferId, String entitlementName) {
        Waits.sleep(5000); // wait for packart to load
        return getWishlistTile(entitlementOfferId).getPackArtName().equals(entitlementName);
    }

    /**
     * Verify if 'Wishlist' tiles for all the given entitlements have pack arts.
     *
     * @param entitlementInfos Entitlements to look for pack arts
     * @return true if all entitlements have corresponding 'Wishlist' tiles with
     * pack arts, false otherwise
     */
    public boolean verifyPackArtsExist(EntitlementInfo... entitlementInfos) {
        if (entitlementInfos == null) {
            throw new RuntimeException("List of entitlementInfos must not be null");
        }
        List<WishlistTile> allWishlistItems = getAllWishlistTiles();
        if (allWishlistItems.isEmpty()) {
            throw new RuntimeException("Could not get the wishlist items");
        }
        for (WishlistTile wishlistTile : allWishlistItems) {
            for (EntitlementInfo entitlement : entitlementInfos) {
                if (wishlistTile.verifyOfferId(entitlement.getOfferId())) {
                    if (!wishlistTile.getPackArtName().equals(entitlement.getName())) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Product title
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Verify if 'Wishlist' tiles for all the given entitlement offer IDs have
     * correct product title.
     *
     * @param entitlementOfferId Entitlement Offer IDs to look for
     * @param entitlementName Name of the entitlement
     * @return true if 'Wishlist' tile for the entitlement offer ID was found with
     * the correct product title, false otherwise
     */
    public boolean verifyProductTitleEntitlement(String entitlementOfferId, String entitlementName) {
        return getWishlistTile(entitlementOfferId).getProductTitle().equals(entitlementName);
    }

    /**
     * Verifies that the given entitlements has the correct product title in the
     * 'Wishlist' tab.
     *
     * @param entitlementInfos Entitlements to look for product titles
     * @return true if the entitlement was found with the correct product title,
     * false otherwise
     */
    public boolean verifyProductTitlesExist(EntitlementInfo... entitlementInfos) {
        if (entitlementInfos == null) {
            throw new RuntimeException("List of entitlementInfos must not be null");
        }
        List<WishlistTile> allWishlistItems = getAllWishlistTiles();
        if (allWishlistItems.isEmpty()) {
            throw new RuntimeException("Could not get the wishlist items");
        }

        for (WishlistTile wishlistTile : allWishlistItems) {
            for (EntitlementInfo entitlement : entitlementInfos) {
                if (wishlistTile.verifyOfferId(entitlement.getOfferId())) {
                    if (!wishlistTile.getProductTitle().equals(entitlement.getName())) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Date Added
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Verifies that the all entitlements has the correct added date.
     *
     * @return true if the entitlement was found with the added date,
     * false otherwise
     */
    public boolean verifyDateAddedKeywordForAllItems() {
        List<WishlistTile> allWishlistItems = getAllWishlistTiles();

        if (allWishlistItems.isEmpty()) {
            throw new RuntimeException("Could not get the wishlist items");
        }

        for (WishlistTile wishlistTile : allWishlistItems) {
            if (!wishlistTile.verifyDateAddedKeyword()) {
                return false;
            }
        }
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Ineligible message
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Verify given entitlements have an ineligible message in their 'Wishlist'
     * tile.
     *
     * @param entitlementInfos The entitlements to look for an 'Ineligible'
     * message
     * @return true if the entitlement was found with the 'Ineligible' message,
     * false otherwise
     */
    public boolean verifyIneligibleMessageExist(EntitlementInfo... entitlementInfos) {
        if (entitlementInfos == null) {
            throw new RuntimeException("List of entitlementInfos must not be null");
        }
        return verifyIneligibleMessageExist(EntitlementInfoHelper.getEntitlementOfferIds(entitlementInfos));
    }

    /**
     * Verifies that the given entitlements has the 'Ineligible' message in
     * the 'Wishlist' tab.
     *
     * @param entitlementOfferIds The entitlements to look for an 'Ineligible'
     * message
     * @return true if the entitlement was found with the 'Ineligible' message,
     * false otherwise
     */
    public boolean verifyIneligibleMessageExist(List<String> entitlementOfferIds) {
        List<WishlistTile> allWishlistItems = getWishlistTiles(entitlementOfferIds);

        if (allWishlistItems.isEmpty()) {
            throw new RuntimeException("Could not get the wishlist items");
        }

        for (WishlistTile wishlistTile : allWishlistItems) {
            if (!wishlistTile.verifyIneligibleMessage()) {
                return false;
            }
        }
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Remove from Wishlist link
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Verifies that the all 'Wishlist' tiles have a 'Remove From Wishlist' link.
     *
     * @return true if the all 'Wishlist' tiles have a 'Remove From Wishlist' link,
     * false otherwise
     */
    public boolean verifyRemoveFromWishlistLinkExistForAllItems() {
        List<WishlistTile> allWishlistItems = getAllWishlistTiles();
        if (allWishlistItems.isEmpty()) {
            _log.error("Unable to find Wishlist tiles");
            return false;
        }
        return allWishlistItems.stream().noneMatch((wishlistTile) -> (wishlistTile == null || !wishlistTile.verifyRemoveFromWishlistLinkVisible()));
    }
    
    /**
     * Removes all the products in the user's wishlist.
     */
    public void removeAllWishlistItems(){
        getAllWishlistTiles().stream().forEach(wishlistTile -> {wishlistTile.clickRemoveFromWishlistLink();});
    }

    ////////////////////////////////////////////////////////////////////////////
    // Share your Wishlist Button
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Click on the 'Share Your Wishlist' button (to open a dialog containing the
     * shared link).
     */
    public void clickShareYourWishlistButton() {
        waitForElementClickable(SHARE_YOUR_WISHLIST_BUTTON_LOCATOR).click();
    }

    ////////////////////////////////////////////////////////////////////////////
    // Private Profile Notification
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Verify 'Profile is Private' notification is visible.
     *
     * @return true if the notification indicating that your profile is private
     * is shown
     */
    public boolean verifyProfilePrivateNotification() {
        return waitIsElementVisible(PRIVATE_PROFILE_NOTIFICATION_LOCATOR);
    }
}