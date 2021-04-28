package com.ea.originx.automation.lib.pageobjects.gdp;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.resources.CountryInfo;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import java.util.List;
import org.openqa.selenium.StaleElementReferenceException;

/**
 * Represents 'Offer Selection Page'
 * <p>
 * This page is loaded on clicking the 'Buy' button from either a GDP Page/ Access Interstitial Page
 * if the entitlement is multiple edition to represent the comparison table for different editions.
 *
 * @author nvarthakavi
 */
public class OfferSelectionPage extends EAXVxSiteTemplate {

    protected static final String PRIMARY_CTA_TMPL = "origin-store-osp-comparison-table-cta[data-telemetry-ocdpath='%s'] button.otkbtn-primary";
    protected static final String PRIMARY_CTA_BUTTON = "origin-store-osp-comparison-table-cta[data-telemetry-ocdpath='%s'] button.otkbtn-dropdown-btn";
    protected static final String PURCHASE_AS_A_GIFT_CTA_TMPL = "origin-store-osp-comparison-table-cta[data-telemetry-ocdpath='%s'] .origin-telemetry-gift-cta-primary button.otkbtn-primary";
    protected static final String BUY_NOW_CTA_TMPL = "origin-store-osp-comparison-table-cta[data-telemetry-ocdpath='%s'] .origin-telemetry-buy-cta-primary button.otkbtn-primary";
    protected static final String REMOVE_FROM_WISHLIST_BUTTON = "otkex-dropdown-item:nth-child(2) > span";
    protected static final By OSP_PAGETITLE_LOCATOR = By.cssSelector("div.origin-store-osp-hero h4.origin-store-osp-hero-pagetitle.otktitle-page");
    protected static final By ACCESS_DISCOUNT_LOCATOR = By.cssSelector(".otkex-priceblock-originaccess-discount");
    protected static final String[] ACCESS_DISCOUNT_KEYWORDS = {"origin", "access", "discount", "applied"};
    protected static final By EDITION_LOCATORS_ALL = By.cssSelector(".origin-store-pdp-comparison-table-editions");
    private static final String[] EXPECTED_OSP_PAGE_TITLE_KEYWORDS = {"choose", "edition"};

    // Wishlist
    protected static final String ADD_TO_WISHLIST_CSS = "otkex-addtowishlist";
    protected static final By WISHLIST_HEART_ICON_LOCATOR = By.cssSelector(ADD_TO_WISHLIST_CSS + " .otkicon-heart");
    protected static final By WISHLIST_MESSAGE_TEXT_LOCATOR = By.cssSelector(ADD_TO_WISHLIST_CSS + " .otkex-addtowishlist-text");
    protected static final By VIEW_WISHLIST_LINK_LOCATOR = By.cssSelector(ADD_TO_WISHLIST_CSS + " .otkex-addtowishlist-text > a");
    private final String EXPECTED_ON_WISHLIST_TEXT = "Added to wishlist";
    private final String EXPECTED_VIEW_WISHLIST_LINK_TEXT = "View wishlist";

    //Game Rating
    protected static final By PRODUCT_HERO_ENTITLEMENT_RATING_LOCATOR = By.cssSelector(".otkex-rating img");
    
    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OfferSelectionPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the OSP Page was reached
     *
     * @return true if the user is on the OSP Page, false otherwise
     */
    public boolean verifyOfferSelectionPageReached() {
        waitForPageToLoad();
        try {
            return StringHelper.containsAnyIgnoreCase(waitForElementVisible(OSP_PAGETITLE_LOCATOR).getText(), EXPECTED_OSP_PAGE_TITLE_KEYWORDS);
        } catch (TimeoutException te) {
            return false;
        }
    }
    
    /**
     * Verify the 'Purchase as a gift' CTA is visible
     * 
     * @param ocdPath OCD path for entitlement
     * @return true if the button is visible, false otherwise
     */
    public boolean verifyPurchaseAsAGiftCTAVisible(String ocdPath) {
        return waitIsElementVisible(By.cssSelector(String.format(PURCHASE_AS_A_GIFT_CTA_TMPL, ocdPath)));
    }
    
    /**
     * Verify the 'Buy now' CTA is visible
     * 
     * @param ocdPath OCD path for entitlement
     * @return true if the button is visible, false otherwise
     */
    public boolean verifyBuyNowCTAVisible(String ocdPath) {
        return waitIsElementVisible(By.cssSelector(String.format(BUY_NOW_CTA_TMPL, ocdPath)));
    }

    /**
     * Click primary CTA with a given offer-id to
     * select a particular edition in the comparison table
     *
     * @param ocdPath OCD path for entitlement
     */
    public void clickPrimaryButton(String ocdPath) {
        WebElement primaryButton = waitForElementClickable(By.cssSelector(String.format(PRIMARY_CTA_TMPL, ocdPath)));

        // for games that have 3 editions
        scrollElementToCentre(primaryButton);
        primaryButton.click();

    }

    /**
     * Click primary CTA dropdown with a given offer-id to
     * add a particular edition to the wishlist
     *
     * @param ocdPath OCD path for entitlement
     */
    public void clickPrimaryDropdownAndRemoveFromWishlist(String ocdPath) {
        WebElement primaryDropdownButton = waitForElementClickable(By.cssSelector(String.format(PRIMARY_CTA_BUTTON, ocdPath)));

        // for games that have 3 editions
        scrollElementToCentre(primaryDropdownButton);
        primaryDropdownButton.click();

        waitForElementClickable(By.cssSelector(String.format(REMOVE_FROM_WISHLIST_BUTTON))).click();
    }


    /**
     * Verify that the 'Origin Access Discount Applied' is provided for an origin access user
     * on all entitlements listed.
     *
     * @return true if the expected keywords are found in the message, false otherwise
     */
    public boolean verifyOriginAccessDiscountIsVisible() {
        try {
            List<WebElement> discountElements = waitForAllElementsVisible(ACCESS_DISCOUNT_LOCATOR, 10);
            List<WebElement> editions = waitForAllElementsVisible(EDITION_LOCATORS_ALL, 10);
            if (discountElements.size() != editions.size()) {
                _log.debug("The number of discount message does not equal to the number of editions listed!");
                return false;
            }

            for (WebElement el : discountElements) {
                // return false right way if one of editions has no discount message
                if (!StringHelper.containsIgnoreCase(el.getText(), ACCESS_DISCOUNT_KEYWORDS)) {
                    return false;
                }
            }

            return true;
        } catch (TimeoutException e) {
            return false;
        }
    }
    
    //////////////////////////////////////////////////////////////////////////
    // WISHLIST
    // Wishlist information related to the product, like if it is on the
    // user's wishlist.
    //////////////////////////////////////////////////////////////////////////
    
    /**
     * Verify the wishlist heart icon is visible.
     *
     * @return true if wishlist heart icon is visible, false otherwise
     */
    public boolean verifyWishlistHeartIconVisible() {
        return waitIsElementVisible(WISHLIST_HEART_ICON_LOCATOR, 5); // increase to 5 seconds since 1 is failing
    }
    
    /**
     * Verify the wishlist heart icon is red.
     *
     * @return true if wishlist heart icon is red, false otherwise
     */
    public boolean verifyWishlistHeartIconRed() {
        return getColorOfElement(waitForElementVisible(WISHLIST_HEART_ICON_LOCATOR)).equals(OriginClientData.WISHLIST_HEART_ICON_COLOUR);
    }

    /**
     * Verify 'On Your Wishlist' message is as expected.
     *
     * @return true if 'On Your Wishlist' message text matches, false otherwise
     */
    public boolean verifyOnWishlistMessageText() {
        return verifyOnWishlistMessageText(30);
    }

    /**
     * Verify 'View Wishlist' link text is as expected.
     *
     * @return true if 'View Wishlist' link text matches, false otherwise
     */
    public boolean verifyViewWishlistLinkText() {
        return verifyViewWishlistLinkText(30);
    }

    /**
     * Verify 'On Your Wishlist' message.
     *
     * @param waitSeconds Number of seconds to wait for 'Wishlist message'
     * element to be visible
     * @return true if 'On Your Wishlist' message text matches, false otherwise
     */
    public boolean verifyOnWishlistMessageText(int waitSeconds) {
        try {
            String message = waitForElementVisible(WISHLIST_MESSAGE_TEXT_LOCATOR, waitSeconds).getText();
            boolean result = StringHelper.containsIgnoreCase(message, EXPECTED_ON_WISHLIST_TEXT);
            if (!result) {
                throw new RuntimeException(String.format("Wishlist message '%s' does not contain expected text '%s'",
                        message, EXPECTED_ON_WISHLIST_TEXT));
            }
            return result;
        } catch (TimeoutException | StaleElementReferenceException e) {
            return false;
        }
    }

    /**
     * Verify 'View Wishlist' link text is as expected.
     *
     * @param waitSeconds The number of seconds to wait for the 'Wishlist
     * message'
     * @return true if the 'View Wishlist' link text matches, false otherwise
     */
    public boolean verifyViewWishlistLinkText(int waitSeconds) {
        try {
            String text = waitForElementVisible(VIEW_WISHLIST_LINK_LOCATOR, waitSeconds).getText();
            boolean result = StringHelper.containsIgnoreCase(text, EXPECTED_VIEW_WISHLIST_LINK_TEXT);
            if (!result) {
                throw new RuntimeException(String.format("Wishlist link label '%s' does not contain expected text '%s'",
                        text, EXPECTED_VIEW_WISHLIST_LINK_TEXT));
            }
            return result;
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify wishlist text is to the right of the heart icon.
     *
     * @return true if text is to the right, false otherwise
     */
    public boolean verifyWishlistTextIsRightOfIcon() {
        return getXCoordinate(waitForElementVisible(WISHLIST_MESSAGE_TEXT_LOCATOR)) > getXCoordinate(waitForElementVisible(WISHLIST_HEART_ICON_LOCATOR));
    }
    
    /**
     * Verify a given entitlement is on the wishlist.
     *
     * @return true if wishlist heart icon, 'Added to your wishlist' message and
     * 'View full wishlist' link text are all visible, false otherwise
     */
    public boolean verifyOnWishlist() {
        return verifyWishlistHeartIconVisible()
                && verifyOnWishlistMessageText()
                && verifyViewWishlistLinkText();
    }

    /**
     * Verify a 'Play First Trial now available' text is showing between the
     * primary and secondary button
     *
     * Click the 'View Full Wishlist' link
     */
    public void clickViewWishlistLink(){
        waitForElementClickable(VIEW_WISHLIST_LINK_LOCATOR).click();
    }

    /**
     * Get the 'OSP' entitlement rating
     *
     * @return the entitlement rating name
     */
    public String getOSPEntitlementRating() {
        return waitForElementPresent(PRODUCT_HERO_ENTITLEMENT_RATING_LOCATOR).getAttribute("alt");
    }

    /**
     * Verifies the Rating is correctly localized based on the country given
     *
     * @param country the country localization for a specific rating
     * @return true if correctly localized, false otherwise
     */
    public boolean verifyOSPEntitlementRating(CountryInfo.CountryEnum country) {
        return StringHelper.containsIgnoreCase(getOSPEntitlementRating(), country.getCountryGameRating());
    }
}
