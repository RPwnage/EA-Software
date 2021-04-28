package com.ea.originx.automation.lib.macroaction;

import com.ea.originx.automation.lib.pageobjects.dialog.GiftMessageDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.ShareYourWishlistDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.pageobjects.profile.WishlistTile;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.helpers.RobotKeyboard;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import java.awt.event.KeyEvent;
import java.io.IOException;
import java.lang.invoke.MethodHandles;

/**
 * Macro class containing static methods for multi-step actions related to the
 * wishlist.
 *
 * @author rchoi
 */
public final class MacroWishlist {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());
    private static final String CANNOT_OPEN_GDP = "Cannot open GDP page for: %s";
    private static final String FAIL_TO_OPEN_WISHLIST_PAGE = "Failed to open the wishlist page";
    private static final String WISHLIST = "wishlist";
    
    /**
     * Constructor
     */
    private MacroWishlist() {
    }

    /**
     * Verify if a game added to wishlist through its GDP page
     *
     * @param driver Selenium WebDriver
     * @param entitlementInfo Entitlement added to wishlist
     * @param isOnGDP true is on GDP page, false otherwise;
     * if false given, this function will navigate to GDP page before verification
     *
     * @return true if wishlist heart icon, 'Added to your wishlist' message and
     * 'View full wishlist' link text are all visible, false otherwise
     */
    public static boolean verifyAddedToWishlist(WebDriver driver, EntitlementInfo entitlementInfo, boolean isOnGDP) {
        if (!isOnGDP && !MacroGDP.loadGdpPage(driver, entitlementInfo)) {
            _log.error(String.format(CANNOT_OPEN_GDP, entitlementInfo.getName()));
            return false;
        }
        return new GDPActionCTA(driver).verifyOnWishlist();
    }

    /**
     * Verify if a game can be added to the wishlist through its GDP page
     *
     * @param driver Selenium WebDriver
     * @param entitlementInfo Entitlement added to wishlist
     *
     * @return true if the buy button drop-down allows entitlement to be added,
     * false otherwise
     */
    public static boolean verifyAddingGameToWishlist(WebDriver driver, EntitlementInfo entitlementInfo) {
        if (!MacroGDP.loadGdpPage(driver, entitlementInfo)) {
            _log.error(String.format(CANNOT_OPEN_GDP, entitlementInfo.getName()));
            return false;
        }
        return !new GDPActionCTA(driver).verifyOnWishlist();
    }

    /**
     * Adds a given entitlement to the user's wishlist.
     *
     * @param driver Selenium WebDriver
     * @param entitlementInfo Entitlement added to wishlist
     * opening a PDP page on the browser
     * @return true if entitlement is successfully added to the wishlist, false otherwise
     */
    public static boolean addToWishlist(WebDriver driver, EntitlementInfo entitlementInfo) {
        return addToWishlist(driver, entitlementInfo.getParentName(), entitlementInfo.getOcdPath(), entitlementInfo.getPartialPdpUrl());
    }

    /**
     * Adds a given entitlement to the user's wishlist.
     * 
     * @param driver Selenium WebDriver
     * @param entitlementName Entitlement name
     * @param ocdPath OCD path for entitlement
     * @param entitlementPartialGDPURL Entitlement partial PDP URL
     * @return true if entitlement is successfully added to the wishlist, false otherwise
     */
    public static boolean addToWishlist(WebDriver driver, String entitlementName, String ocdPath, String entitlementPartialGDPURL) {
        if (!MacroGDP.loadGdpPage(driver, entitlementName, entitlementPartialGDPURL)) {
            _log.error(String.format(CANNOT_OPEN_GDP, entitlementName));
            return false;
        }
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        if (Waits.pollingWait(() -> gdpActionCTA.verifyGDPHeaderReached())) {
            gdpActionCTA.selectDropdownAddToWishlist();
            OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
            if(offerSelectionPage.verifyOfferSelectionPageReached()) {
                offerSelectionPage.clickPrimaryButton(ocdPath);
            }
            return true;
        }
        _log.error(String.format(CANNOT_OPEN_GDP, entitlementName));
        return false;
    }

    /**
     * Get the 'Share Your Wishlist' address starting from the wishlist tab,
     * which must have already been open.
     *
     * @param driver Selenium WebDriver
     * @return 'Share Your Wishlist' address if found, or throw exception
     */
    public static String getWishlistShareAddress(WebDriver driver) {
        ProfileWishlistTab wishlistTab = new ProfileWishlistTab(driver);
        try {
            wishlistTab.clickShareYourWishlistButton();
        } catch (TimeoutException e) {
            _log.debug("Timed out clicking 'Share your wishlist' button");
            return null;
        }

        ShareYourWishlistDialog shareYourWishlistDialog = new ShareYourWishlistDialog(driver);
        // wait on shareYourWishlistDialog to open, or return right away if it fails
        if (!Waits.pollingWait(() -> shareYourWishlistDialog.isOpen())) {
            _log.debug("Failed to open 'Share your wishlist' dialog");
            return null;
        }
        String wishlistAddr = shareYourWishlistDialog.getShareWishlistLink().orElse(null);
        shareYourWishlistDialog.closeAndWait();
        return wishlistAddr;
    }

    /**
     * Navigates to a friend's wishlist by navigating to the current user's profile,
     * going to the friend's tab, finding the friend, then going to their wishlist.
     *
     * @param driver Selenium WebDriver
     * @param friendUsername The name of the friend whose wishlist should be navigated to
     * @return true if navigating to friend's wishlist was successful, false otherwise
     */
    public static boolean navigateToFriendWishlist(WebDriver driver, String friendUsername) {
        MacroProfile.navigateToFriendProfile(driver, friendUsername);
        ProfileHeader profileHeader = new ProfileHeader(driver);
        profileHeader.waitForAchievementsTabToLoad();
        ProfileHeader wishlistProfile = new ProfileHeader(driver);
        wishlistProfile.openWishlistTab();
        if (!profileHeader.verifyWishlistActive()) {
            _log.debug(FAIL_TO_OPEN_WISHLIST_PAGE);
            return false;
        }
        return true;
    }

    /**
     * Navigates to a user's wishlist page by clicking the Shortcut CTRL-SHIFT-W.
     *
     * @param driver Selenium WebDriver
     * @return true if navigating to the wishlist page was successful, false otherwise
     * @throws java.lang.Exception
     */
    public static boolean navigateToWishlistUsingShortcut(WebDriver driver) throws Exception {
        new RobotKeyboard().pressAndReleaseKeys(OriginClient.getInstance(driver), KeyEvent.VK_CONTROL, KeyEvent.VK_SHIFT, KeyEvent.VK_W);

        ProfileHeader profileHeader = new ProfileHeader(driver);
        if (!profileHeader.verifyWishlistActive()) {
            _log.debug(FAIL_TO_OPEN_WISHLIST_PAGE);
            return false;
        }

        profileHeader.waitForWishlistTabToLoad(); // added this wait as to make sure the wishlist page is stablized
        return true;
    }

    /**
     * Navigates to a user's wishlist page by navigating to profile page then
     * click the wishlist tab.
     *
     * @param driver Selenium WebDriver
     * @return true if navigating to the wishlist page was successful, false otherwise
     */
    public static boolean navigateToWishlist(WebDriver driver) {
        new MiniProfile(driver).selectViewMyProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        profileHeader.waitForAchievementsTabToLoad();
        if (!profileHeader.verifyAchievementsTabActive()) {
            _log.debug(FAIL_TO_OPEN_WISHLIST_PAGE);
            return false;
        }

        profileHeader.openWishlistTab();
        for(int i = 0; i < 3; ++i){//There's an issue with the profile reloading on an empty profile page via automation, so multiple tries are needed
            profileHeader.waitForWishlistTabToLoad();
            if(profileHeader.verifyWishlistActive()){
                return true;
            }
            driver.navigate().refresh();
        }
        _log.debug(FAIL_TO_OPEN_WISHLIST_PAGE);
        return false;
    }

    /**
     * Purchase an entitlement through wishlist tab in profile page as a gift
     * for a friend or user self.
     *
     * @param driver Selenium WebDriver
     * @param entitlement Entitlement to purchase (note: for browser test,
     * partialURL for PDP must have been defined)
     * @param friendNameOrEmail Name or email of friend to send gift to
     * @param message Gift message sender sends
     * @param isUserSelf true if purchase from own wishlist and false for other user's wishlist
     * @param isProductionEnvironment true if is on production environment
     * @return true if the entitlement is successfully purchased, false otherwise
     * @throws java.io.IOException
     */
    public static boolean purchaseGiftFromWishListTab(WebDriver driver, EntitlementInfo entitlement, String friendNameOrEmail, String message, boolean isUserSelf, boolean isProductionEnvironment) throws IOException {
        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(driver);
        new ProfileHeader(driver).waitForWishlistTabToLoad();
        WishlistTile wishlistTile = profileWishlistTab.getWishlistTile(entitlement);
        if (isUserSelf) {
            wishlistTile.clickBuyButton();
        } else {
            // handle gift message dialog
            wishlistTile.clickPurchaseAsGiftButton();
            GiftMessageDialog giftMessageDialog = new GiftMessageDialog(driver);
            giftMessageDialog.enterMessage(message);
            giftMessageDialog.clickNext();
        }
        final String entitlementName = entitlement.getName();
        // Handle 'Purchase Info', 'Review Order', and 'Thank You' pages
        boolean completePurchaseCloseThankYouSuccessful = false;
        if (isProductionEnvironment) {
            completePurchaseCloseThankYouSuccessful = MacroPurchase.purchaseThroughPaypalOffCodeFromPaymentInformationPage(driver, entitlement, friendNameOrEmail);
        } else {
            completePurchaseCloseThankYouSuccessful = MacroPurchase.completePurchaseAndCloseThankYouPage(driver);
        }
        if (!completePurchaseCloseThankYouSuccessful) {
            _log.error(String.format("Error: Cannot complete purchase of %s as a gift for %s", entitlementName, friendNameOrEmail));
            return false;
        }
        return true;
    }

    /**
     * Checks to see if Buy and Origin Access CTAs appear for a tile on wishlist
     *
     * @param driver the current webdriver
     * @param entitlementInfo the specific offer to check for CTAs
     * @return true if both Buy and Origin Access CTAs appear. False if at least
     * one is missing
     */
    public static boolean verifyBuyAndOriginAccessCTAs(WebDriver driver, EntitlementInfo entitlementInfo) {
        //Check if already on Wishlist page. If not, navigate to.
        if (!driver.getCurrentUrl().contains(WISHLIST) && !navigateToWishlist(driver)) {
            _log.error(FAIL_TO_OPEN_WISHLIST_PAGE);
            return false;
        }

        boolean buyButton = false;
        boolean accessButton = false;
        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(driver);
        for (int i = 0; i < 3; ++i) {//There's an issue with the profile reloading on an empty profile page via automation, so multiple tries are needed
            driver.navigate().refresh(); //Refresh the pag to reload wishlist
            profileWishlistTab.waitForPageToLoad();
            buyButton = profileWishlistTab.verifyBuyButtonsExist(entitlementInfo);
            accessButton = profileWishlistTab.verifyPlayAccessButtonsExist(entitlementInfo);
            if (buyButton && accessButton) {
                break;
            }
        }

        return buyButton && accessButton;
    }

    /**
     * Checks to see if Tile exists in Wishlist
     *
     * @param driver the current webdriver
     * @param offerIds list of offer IDs to check existence
     * @return true if tiles exist, false if not
     */
    public static boolean verifyTilesExist(WebDriver driver, String... offerIds) {
        //Check if already on Wishlist page. If not, navigate to.
        if (!driver.getCurrentUrl().contains(WISHLIST) && !navigateToWishlist(driver)) {
            _log.error(FAIL_TO_OPEN_WISHLIST_PAGE);
            return false;
        }

        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(driver);
        for (int i = 0; i < 3; ++i) {//There's an issue with the profile reloading on an empty profile page via automation, so multiple tries are needed
            profileWishlistTab.waitForPageToLoad();
            if (profileWishlistTab.verifyTilesExist(offerIds)) {
                return true;
            }
            driver.navigate().refresh(); //Refresh the pag to reload wishlist
        }

        return false;
    }

    /**
     * Checks to see if Tile do not exists in Wishlist
     *
     * @param driver the current webdriver
     * @param offerIds list of offer IDs to check existence
     * @return true if tiles do not exist, false if they appear
     */
    public static boolean verifyTilesRemoved(WebDriver driver, String... offerIds) {
        //Check if already on Wishlist page. If not, navigate to.
        if (!driver.getCurrentUrl().contains(WISHLIST) && !navigateToWishlist(driver)) {
            _log.error(FAIL_TO_OPEN_WISHLIST_PAGE);
            return false;
        }
        
        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(driver);
        for (int i = 0; i < 3; ++i) { //There's an issue with the profile reloading on an empty profile page via automation, so multiple tries are needed
            profileWishlistTab.waitForPageToLoad();
            if (!profileWishlistTab.verifyTilesExist(offerIds)) {
                return true;
            }
            driver.navigate().refresh(); //Refresh the pag to reload wishlist
        }

        return false;
    }
}