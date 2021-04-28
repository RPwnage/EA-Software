package com.ea.originx.automation.lib.pageobjects.gdp;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.resources.OriginClientData;
import org.openqa.selenium.By;
import org.openqa.selenium.NoSuchElementException;
import org.openqa.selenium.StaleElementReferenceException;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;

/**
 * A page representing the GDP Header CTAs. Encapsulates all of the CTAs in the
 * action section of the GDP Hero, as well as anything required to show or open
 * the CTAs, and the price (which is found on the CTA).
 *
 * @author nvarthakavi
 * @author cdeaconu
 */
public class GDPActionCTA extends GDPHeader {

    //Primary CTA
    protected final static String ORIGIN_STORE_PRIMARY_CTA_TMPL = "origin-store-gdp-primarycta";
    protected final static String ORIGIN_STORE_GDP_HEADER_BLOCK = "div.origin-store-gdp-header-block";
    protected final static By ORIGIN_STORE_PRIMARY_CTA_LOCATOR = By.cssSelector(ORIGIN_STORE_PRIMARY_CTA_TMPL + " .otkbtn-primary");
    protected final static By GET_THE_GAME_CTA_LOCATOR = By.cssSelector(ORIGIN_STORE_GDP_HEADER_BLOCK + " " + ORIGIN_STORE_PRIMARY_CTA_TMPL + " .origin-telemetry-cta-buy-aui-primary button.otkbtn-primary.otkbtn-primary-btn");
    protected final static By BUY_CTA_OSP_LOCATOR = By.cssSelector(ORIGIN_STORE_PRIMARY_CTA_TMPL + " .origin-telemetry-cta-buy-osp-primary button.otkbtn-primary");
    protected final static By BUY_CTA_LOCATOR = By.cssSelector(ORIGIN_STORE_PRIMARY_CTA_TMPL + " .origin-telemetry-cta-buy-primary button.otkbtn-primary");
    protected final static By PREORDER_CTA_LOCATOR = By.cssSelector(ORIGIN_STORE_PRIMARY_CTA_TMPL + " .origin-telemetry-cta-preorder-primary button.otkbtn-primary");
    protected final static By PREORDER_CTA_OSP_LOCATOR = By.cssSelector(ORIGIN_STORE_PRIMARY_CTA_TMPL + " .origin-telemetry-cta-preorder-osp-primary button.otkbtn-primary");
    protected final static By ADD_TO_LIBRARY_VAULT_GAME_CTA_LOCATOR = By.cssSelector(ORIGIN_STORE_PRIMARY_CTA_TMPL + " .origin-telemetry-cta-add-vaultgame-primary button.otkbtn-primary");
    protected final static By VIEW_IN_LIBRARY_CTA_LOCATOR = By.cssSelector(ORIGIN_STORE_PRIMARY_CTA_TMPL + " .origin-telemetry-cta-library-primary button.otkbtn-primary");
    protected final static By DROPDDOWN_CTA_LOCATOR = By.cssSelector(ORIGIN_STORE_PRIMARY_CTA_TMPL + " .otkbtn-dropdown-btn");
    protected final static By BUY_AS_GIFT_BUTTON_LOCATOR = By.cssSelector(ORIGIN_STORE_PRIMARY_CTA_TMPL + ".origin-telemetry-cta-gift button.otkbtn-primary");
    protected final static By PLAY_NOW_CTA_LOCATOR = By.cssSelector(ORIGIN_STORE_PRIMARY_CTA_TMPL + " .origin-telemetry-cta-play-now button.otkbtn-primary");
    
    //Secondary CTA
    protected final static String ORIGIN_STORE_SECONDARY_CTA_CSS = "origin-store-gdp-secondarycta";
    protected final static By TRY_IT_FIRST_CTA_LOCATOR = By.cssSelector(ORIGIN_STORE_SECONDARY_CTA_CSS + " .origin-telemetry-get-trial button.otkbtn");
    protected final static By VIEW_TRIAL_CTA_LOCATOR = By.cssSelector(ORIGIN_STORE_SECONDARY_CTA_CSS + " .origin-telemetry-try-pft .otkex-cta");
    protected final static By WATCH_TRAILER_CTA_LOCATOR = By.cssSelector(ORIGIN_STORE_SECONDARY_CTA_CSS + " .origin-telemetry-trailer .otkex-cta");
    protected final static By PLAY_FIRST_TRIAL_TEXT_LOCATOR = By.cssSelector(ORIGIN_STORE_SECONDARY_CTA_CSS + " p");
    
    // Dropdown
    protected static final String ORIGIN_STORE_GDP_INFOBLOCKS = "origin-store-gdp-infoblocks";
    protected static final By PRIMARY_BUTTON_DROPDOWN_ARROW_LOCATOR = By.cssSelector(ORIGIN_STORE_PRIMARY_CTA_TMPL + " .otkbtn-dropdown-btn");
    protected static final By WISHLIST_HEART_ICON_LOCATOR = By.cssSelector(ORIGIN_STORE_GDP_INFOBLOCKS + " .otkicon-heart");
    protected static final By WISHLIST_MESSAGE_TEXT_LOCATOR = By.cssSelector(ORIGIN_STORE_GDP_INFOBLOCKS + " otkex-infoblock[type=heart] .otkex-infoblock-text");
    protected static final By VIEW_WISHLIST_LINK_LOCATOR = By.cssSelector(ORIGIN_STORE_GDP_INFOBLOCKS + " otkex-infoblock[type=heart] .otkex-infoblock-text > a");
    private final String EXPECTED_ON_WISHLIST_TEXT = "Added to your wishlist";
    private final String EXPECTED_ADD_TO_WISHLIST_TEXT = "Add to wishlist";
    private final String EXPECTED_VIEW_WISHLIST_LINK_TEXT = "View full wishlist";
    private final String EXPECTED_PLAY_FIRST_TRIAL_TEXT = "Play First Trial now available";
    private final String EXPECTED_PURCHASE_AS_A_GIFT_TEXT = "Purchase as a gift";
    private final String EXPECTED_VIEW_ALL_EDITIONS_TEXT = "View all editions";
    private final String EXPECTED_UPGRADE_MY_EDITION_TEXT = "Purchase different edition";
    private final String EXPECTED_BUY_NOW_TEXT = "Buy Now";
    private final String EXPECTED_GET_THE_GAME_TEXT = "Get the Game";
    
    private final GDPDropdownMenu dropdownMenu;

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public GDPActionCTA(WebDriver driver) {
        super(driver);
        dropdownMenu = new GDPDropdownMenu(driver, PRIMARY_BUTTON_DROPDOWN_ARROW_LOCATOR, true); // left click
    }

    /**
     * Verify the primary CTA is visible
     *
     * @return true if the button is visible, false otherwise
     */
    public boolean verifyPrimaryCTAVisible() {
        return waitIsElementVisible(ORIGIN_STORE_PRIMARY_CTA_LOCATOR);
    }

    /**
     * Verify an entitlement has 'Get the Game' as its primary CTA The following
     * CTA appears when non-subscriber user navigates to 1.a single edition
     * vault entitlement 2.multiple edition with one of the entitlement in vault
     *
     * @return true if the button is visible, false otherwise
     */
    public boolean verifyGetTheGameCTAVisible() {
        return waitIsElementVisible(GET_THE_GAME_CTA_LOCATOR);
    }

    /**
     * Verify an entitlement has 'Get the Game' as its primary CTA The following
     * CTA appears when non-subscriber user navigates to 1.a single edition
     * vault entitlement 2.multiple edition with one of the entitlement in vault
     * (with no waits)
     *
     * @return true if the button is visible, false otherwise
     */
    public boolean isGetTheGameCTAVisible() {
        return isElementVisible(GET_THE_GAME_CTA_LOCATOR);
    }

    /**
     * Verify an entitlement has 'Buy' as its primary CTA when the entitlement
     * is has multiple editions non vault entitlement
     *
     * @return true if the button is visible, false otherwise
     */
    public boolean verifyBuyOSPCTAVisible() {
        return waitIsElementVisible(BUY_CTA_OSP_LOCATOR);
    }

    /**
     * Verify an entitlement has 'Buy' as its primary CTA when the entitlement
     * is has multiple editions non vault entitlement (with no waits)
     *
     * @return true if the button is visible, false otherwise
     */
    public boolean isBuyOSPCTAVisible() {
        return isElementVisible(BUY_CTA_OSP_LOCATOR);
    }

    /**
     * Verify an entitlement has 'Buy' as its primary CTA when the entitlement
     * is has single edition entitlement
     *
     * @return true if the button is visible, false otherwise
     */
    public boolean verifyBuyCTAVisible() {
        return waitIsElementVisible(BUY_CTA_LOCATOR);
    }

    /**
     * Verify an entitlement has 'Buy' as its primary CTA when the entitlement
     * is has single edition entitlement (with no waits)
     *
     * @return true if the button is visible, false otherwise
     */
    public boolean isBuyCTAVisible() {
        return isElementVisible(BUY_CTA_LOCATOR);
    }

    /**
     * Gets the text for Buy Button
     * Verify an entitlement has 'Add to Library' as its primary CTA The
     * following CTA appears when subscriber user navigates to 1.a single
     * edition vault entitlement 2.multiple edition with one of the entitlement
     * in vault
     *
     * @return Buy button text
     */
    public String getBuyButtonText() {
        return waitForElementPresent(BUY_CTA_LOCATOR).getText();
    }

    /**
     * Get the Price part of Buy Button text
     *
     * @return Price of the entitlement
     */
    public String getPriceFromBuyButton() {
        return StringHelper.normalizeNumberString(getBuyButtonText());

    }

    /**
     *
     * @return true if the button is visible, false otherwise
     */
    public boolean verifyAddToLibraryVaultGameCTAVisible() {
        return waitIsElementVisible(ADD_TO_LIBRARY_VAULT_GAME_CTA_LOCATOR);
    }

    /**
     * Verify an entitlement has 'View in Library' as its primary CTA when the
     * entitlement is a single edition vault entitlement owned by the user
     *
     * @return true if the button is visible, false otherwise
     */
    public boolean verifyViewInLibraryCTAVisible() {
        return waitIsElementVisible(VIEW_IN_LIBRARY_CTA_LOCATOR);
    }

    /**
     * Verify an entitlement has 'View in Library' as its primary CTA when the
     * entitlement is a single edition vault entitlement owned by the user
     *
     * @return true if the button is present, false otherwise
     */
    public boolean verifyViewInLibraryCTAPresent() {
        return waitIsElementPresent(VIEW_IN_LIBRARY_CTA_LOCATOR);
    }
    
    /**
     * Verify an entitlement has 'View Trial' as its secondary CTA when the
     * entitlement is a 'Play First Trial' edition vault entitlement
     *
     * @return true if the button is visible, false otherwise
     */
    public boolean verifyViewTrialCTAVisible() {
        return waitIsElementVisible(VIEW_TRIAL_CTA_LOCATOR);
    }
    
    /**
     * Verify an entitlement has 'Watch Trailer' as its secondary CTA placed
     * below the primary CTA
     *
     * @return true if the secondary CTA is visible and place below the primary
     * CTA, false otherwise
     */
    public boolean verifyWatchTrailerCTAIsVisibleBelowPrimaryCTA() {
        return getYCoordinate(waitForElementVisible(WATCH_TRAILER_CTA_LOCATOR)) > getYCoordinate(waitForElementVisible(ORIGIN_STORE_PRIMARY_CTA_LOCATOR));
    }
    
    /**
     * Verify a 'View Trial' entitlement has 'Play First Trial' text placed
     * below the primary CTA
     *
     * @return true if the text is visible and place below the primary CTA,
     * false otherwise
     */
    public boolean verifyPlayFirstTrialTextIsVisibleBelowPrimaryCTA() {
        return getYCoordinate(waitForElementVisible(PLAY_FIRST_TRIAL_TEXT_LOCATOR)) > getYCoordinate(waitForElementVisible(ORIGIN_STORE_PRIMARY_CTA_LOCATOR));
    }
    
    /**
     * Verify a 'View Trial' entitlement has 'Play First Trial' text placed
     * above the secondary CTA
     *
     * @return true if the text is visible and place below the primary CTA,
     * false otherwise
     */
    public boolean verifyPlayFirstTrialTextIsVisibleAboveSecondaryCTA() {
        return getYCoordinate(waitForElementVisible(VIEW_TRIAL_CTA_LOCATOR)) > getYCoordinate(waitForElementVisible(PLAY_FIRST_TRIAL_TEXT_LOCATOR));
    }
    
    /**
     * Verify the 'Primary' dropdown 'Purchase as Gift' menu item is visible.
     * 
     * @return true if the menu item is visible, false otherwise
     */
    public boolean verifyDropdownPurchaseAsGiftVisible() {
        return waitIsElementVisible(dropdownMenu.getItemContainingIgnoreCase(EXPECTED_PURCHASE_AS_A_GIFT_TEXT));
    }

    /**
     * Verify the 'Purchase as Gift' button is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyBuyAsGiftButtonVisible() {
        return waitIsElementVisible(BUY_AS_GIFT_BUTTON_LOCATOR, 2);
    }

    /**
     * Verify the 'Buy' dropdown 'Purchase as a Gift' menu item is available.
     *
     * @return true if 'Purchase as a Gift' menu item is available, false
     * otherwise
     */
    public boolean verifyBuyDropdownPurchaseAsGiftItemAvailable() {
        return dropdownMenu.verifyContextMenuLocatorVisible() && dropdownMenu.getAllItemsText().contains("Purchase as a gift");
    }

    /**
     * Verify the 'Primary' dropdown 'View all editions' menu item is visible.
     * 
     * @return true if the menu item is visible, false otherwise
     */
    public boolean verifyViewAllEditionsVisible() {
        return waitIsElementVisible(dropdownMenu.getItemContainingIgnoreCase(EXPECTED_VIEW_ALL_EDITIONS_TEXT));
    }
    
    /**
     * Verify the 'Primary' dropdown 'Buy now' menu item is visible.
     * 
     * @return true if the menu item is visible, false otherwise
     */
    public boolean verifyDropdownBuyNowVisible() {
        return waitIsElementVisible(dropdownMenu.getItemContainingIgnoreCase(EXPECTED_BUY_NOW_TEXT));
    }
    
    /**
     * Verify the 'Primary' dropdown 'Upgrade my edition' menu item is visible.
     * 
     * @return true if the menu item is visible, false otherwise
     */
    public boolean verifyDropdownUpgradeMyEditionVisible() {
        return waitIsElementVisible(dropdownMenu.getItemContainingIgnoreCase(EXPECTED_UPGRADE_MY_EDITION_TEXT));
    }
    
    /**
     * Verify a browser only entitlement has 'Play now' as its primary CTA
     *
     * @return true if the button is visible, false otherwise
     */
    public boolean verifyPlayNowCTAVisible() {
        return waitIsElementVisible(PLAY_NOW_CTA_LOCATOR);
    }
    
    /**
     * Verify 'Play now' primary CTA has a pop-out icon visible
     *
     * @return true if the pop-out icon is visible, false otherwise
     */
    public boolean verifyPlayNowCtaPopOutIconVisible() {
        return !getElementPseudoCssValue(waitForElementVisible(PLAY_NOW_CTA_LOCATOR), "::after", "content").isEmpty();
    }
    
    /**
     * Click the 'Buy' (with price) CTA
     */
    public void clickBuyCTA() {
        waitForElementClickable(BUY_CTA_LOCATOR).click();
    }

    /**
     * Click the 'Pre-order' (with price) CTA
     */
    public void clickPreorderCTA() {
        waitForElementClickable(ORIGIN_STORE_PRIMARY_CTA_LOCATOR).click();
    }
    /**
     * Click the 'Try it First' CTA
     */
    public void clickTryItFirstCTA() {
        waitForElementClickable(TRY_IT_FIRST_CTA_LOCATOR).click();
    }

    /**
     * Click the 'Get the Game' CTA
     */
    public void clickGetTheGameCTA() {
        waitForElementClickable(GET_THE_GAME_CTA_LOCATOR).click();
    }

    /**
     * Click the 'Buy' (OSP) CTA
     */
    public void clickBuyOSPCTA() {
        waitForElementClickable(BUY_CTA_OSP_LOCATOR).click();
    }

    /**
     * Click the 'View in Library' CTA
     */
    public void clickViewInLibraryCTA() {
        scrollElementToCentre(waitForElementVisible(VIEW_IN_LIBRARY_CTA_LOCATOR));
        waitForElementClickable(VIEW_IN_LIBRARY_CTA_LOCATOR).click();
    }

    /**
     * Click the 'Add to Library' (Vault game) CTA
     */
    public void clickAddToLibraryVaultGameCTA() {
        waitForElementClickable(ADD_TO_LIBRARY_VAULT_GAME_CTA_LOCATOR).click();
    }

    /**
     * Click the 'View Trial' secondary CTA for a 'Play First Trial' edition
     * vault entitlement
     */
    public void clickViewTrialCTA() {
        waitForElementClickable(VIEW_TRIAL_CTA_LOCATOR).click();
    }
    
    /**
     * Click the 'Watch Trailer' secondary CTA
     */
    public void clickWatchTrailerCTA() {
        waitForElementClickable(WATCH_TRAILER_CTA_LOCATOR).click();
    }
    
    /**
     * Click the 'Play now' primary CTA
     */
    public void clickPlayNowCTA() {
        waitForElementClickable(PLAY_NOW_CTA_LOCATOR).click();
    }

    /**
     * Select the 'Primary' dropdown 'Add to Wishlist' menu item.
     */
    public void selectDropdownAddToWishlist() {
        dropdownMenu.selectItemContainingIgnoreCase(EXPECTED_ADD_TO_WISHLIST_TEXT);
    }

    /**
     * Select the 'Primary' dropdown 'Purchase as Gift' menu item.
     */
    public void selectDropdownPurchaseAsGift() {
        dropdownMenu.selectItemContainingIgnoreCase(EXPECTED_PURCHASE_AS_A_GIFT_TEXT);
    }

    /**
     * Select the 'Primary' dropdown 'Buy' menu item.
     */
    public void selectDropdownBuy() {
        if (dropdownMenu.getItemContainingIgnoreCase(EXPECTED_BUY_NOW_TEXT) != null) {
            dropdownMenu.selectItemContainingIgnoreCase(EXPECTED_BUY_NOW_TEXT);
        } else {
            dropdownMenu.selectItemContainingIgnoreCase(EXPECTED_GET_THE_GAME_TEXT);
        }
    }

    /**
     * Select the 'Primary' dropdown 'Get the Game' menu item.
     */
    public void selectDropdownGetTheGame() {
        dropdownMenu.selectItemContainingIgnoreCase(EXPECTED_GET_THE_GAME_TEXT);
    }

    /**
     * Select the 'Primary' dropdown 'Upgrade my edition' menu item.
     */
    public void selectDropdownUpgradeMyEdition() {
        dropdownMenu.selectItemContainingIgnoreCase(EXPECTED_UPGRADE_MY_EDITION_TEXT);
    }
    
    /**
     * Select the 'Primary' dropdown 'View all editions' menu item.
     */
    public void selectDropdownViewAllEditions() {
        dropdownMenu.selectItemContainingIgnoreCase(EXPECTED_VIEW_ALL_EDITIONS_TEXT);
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
        return convertRGBAtoHexColor(waitForElementVisible(WISHLIST_HEART_ICON_LOCATOR).getCssValue("color")).equals(OriginClientData.WISHLIST_HEART_ICON_COLOUR);
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
                throw new RuntimeException(String.format("PDP wishlist message '%s' does not contain expected text '%s'",
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
                throw new RuntimeException(String.format("PDP view wishlist link label '%s' does not contain expected text '%s'",
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
        return waitForElementVisible(WISHLIST_MESSAGE_TEXT_LOCATOR).getLocation().getX() > waitForElementVisible(WISHLIST_HEART_ICON_LOCATOR).getLocation().getX();
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
    public void clickViewFullWishlistLink(){
        waitForElementClickable(VIEW_WISHLIST_LINK_LOCATOR).click();
    }
    
    /**
    * Verify a 'Play First Trial now available' text is showing between the
    * primary and secondary button
    *
    * @return true if the text is displayed and placed between the buttons,
    * false otherwise
    */
    public boolean verifyPlayFirstTrialText() {
        try {
            String text = waitForElementVisible(PLAY_FIRST_TRIAL_TEXT_LOCATOR).getText();
            boolean isMessageShowing = StringHelper.containsIgnoreCase(text, EXPECTED_PLAY_FIRST_TRIAL_TEXT);
            if (isMessageShowing) {
                int primaryCTALocation = waitForElementVisible(ORIGIN_STORE_PRIMARY_CTA_LOCATOR).getLocation().getY();
                int secondaryCTALocation = waitForElementVisible(VIEW_TRIAL_CTA_LOCATOR).getLocation().getY();
                int playFirstTrialTextLocation = waitForElementVisible(PLAY_FIRST_TRIAL_TEXT_LOCATOR).getLocation().getY();
                if ((primaryCTALocation < playFirstTrialTextLocation) && (playFirstTrialTextLocation < secondaryCTALocation)) {
                    return true;
                }
            }
        } catch (NoSuchElementException e) {
            _log.error("Unable to verify the text is displayed.");
        }
        return false;
    }
}
