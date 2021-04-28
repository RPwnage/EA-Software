package com.ea.originx.automation.lib.macroaction;

import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.GiftMessageDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import java.io.IOException;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;
import javax.annotation.Nullable;
import java.lang.invoke.MethodHandles;

/**
 * Macro class containing static methods for multi-step actions related to gifting.
 *
 * @author rocky
 */
public final class MacroGifting {

    public static final String DEFAULT_GIFT_MESSAGE = "An AWESOME Gift from EA, Just for You!";
    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     */
    private MacroGifting() {
    }

    /**
     * Prepare gift for a friend by opening the entitlement's PDP page and going
     * through the gifting dialogs until the 'Payment Info' page is reached.
     *
     * @param driver Selenium WebDriver
     * @param entitlementName Name of the entitlement to purchase
     * @param ocdPath OCD path for entitlement
     * @param friendName Name of friend to send the gift to
     * @param message Message for the gift
     * @return true if gifting preparation is successful, false otherwise
     */
    public static boolean prepareGiftPurchase(WebDriver driver, String entitlementName, String ocdPath, String friendName, String message) {
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        if (gdpActionCTA.verifyGDPHeaderReached()) {
            gdpActionCTA.selectDropdownPurchaseAsGift();
            OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
            if (offerSelectionPage.verifyOfferSelectionPageReached()) {
                offerSelectionPage.clickPrimaryButton(ocdPath);
            }
        }
        // 'Friends Selection' dialog
        FriendsSelectionDialog friendsSelectionDialog = new FriendsSelectionDialog(driver);
        if (!friendsSelectionDialog.waitIsVisible()) {
            _log.error(String.format("Error: Cannot open 'Friends Selection' dialog for gifting %s", entitlementName));
            return false;
        }
        Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(friendName));
        FriendsSelectionDialog.Recipient recipient = friendsSelectionDialog.selectRecipient(friendName);
        if (!Waits.pollingWait(recipient::isCheckCircleButtonVisible)) {
            _log.error(String.format("Error: Cannot select recipient %s in the 'Friends Selection' dialog", friendName));
            return false;
        }
        friendsSelectionDialog.clickNext();
        GiftMessageDialog giftMessageDialog = new GiftMessageDialog(driver);
        if (!giftMessageDialog.waitIsVisible()) {
            _log.error(String.format("Error: Cannot open 'Gift Message' dialog for gifting %s", entitlementName));
            return false;
        }
        boolean headerVerified = giftMessageDialog.verifyDialogMessageHeader(entitlementName, friendName);
        if (!headerVerified) {
            _log.error(String.format("Error: 'Gift Message' dialog does not show %s as a gift for %s", entitlementName, friendName));
            return false;
        }
        // 'Gift Message' dialog
        giftMessageDialog.enterMessage(message);
        giftMessageDialog.clickNext();
        return true;
    }

    /**
     * Purchase an entitlement through its PDP page as a gift for a friend.
     *
     * @param driver Selenium WebDriver
     * @param entName Entitlement name
     * @param parentName Entitlement name without ASCII characters, used for
     * loading GDP page
     * @param ocdPath OCD path for entitlement
     * @param entPartialPdpURL Entitlement partial PDP URL
     * @param friendName Name of friend to send the gift to
     * @param message Message for the gift
     * @return true if purchase successful, false otherwise
     */
    public static boolean purchaseGiftForFriend(WebDriver driver, String entName, String parentName, String ocdPath, String entPartialPdpURL, String friendName, String message) {
        // Load 'PDP' page
        if (!MacroGDP.loadGdpPage(driver, parentName, entPartialPdpURL)) {
            _log.error(String.format("Error: Cannot open 'PDP' page for %s", entName));
            return false;
        }
        // Handle 'PDP Page', 'Friends Selection', 'Gift Message' dialogs
        if (!prepareGiftPurchase(driver, entName, ocdPath, friendName, message)) {
            _log.error(String.format("Error: Cannot prepare purchase of %s as a gift for %s", entName, friendName));
            return false;
        }
        // Handle 'Purchase Info', 'Review Order', and 'Thank You' pages
        if (!MacroPurchase.completePurchaseAndCloseThankYouPage(driver)) {
            _log.error(String.format("Error: Cannot complete purchase of %s as a gift for %s", entName, friendName));
            return false;
        }
        return true;
    }
    
    /**
     * Purchase an entitlement through its GDP page as a gift for a friend.
     *
     * @param driver Selenium WebDriver
     * @param entitlement Entitlement 
     * @param friendName Name of friend to send the gift to
     * @param message Message for the gift
     * @param userEmail account email
     * @return true if purchase successful, false otherwise
     * @throws java.io.IOException
     */
    public static boolean purchaseGiftForFriendThroughPaypalOffCode(WebDriver driver, EntitlementInfo entitlement, String friendName, String message, String userEmail) throws IOException {
        // Load 'GDP' page
        if (!MacroGDP.loadGdpPage(driver, entitlement.getParentName(), entitlement.getPartialPdpUrl())) {
            _log.error(String.format("Error: Cannot open 'PDP' page for %s", entitlement.getName()));
            return false;
        }
        // Handle 'PDP Page', 'Friends Selection', 'Gift Message' dialogs
        if (!prepareGiftPurchase(driver, entitlement.getName(), entitlement.getOcdPath(), friendName, message)) {
            _log.error(String.format("Error: Cannot prepare purchase of %s as a gift for %s", entitlement.getName(), friendName));
            return false;
        }
        // Handle 'Purchase Info', 'Review Order', and 'Thank You' pages
        if (!MacroPurchase.purchaseThroughPaypalOffCodeFromPaymentInformationPage(driver, entitlement, userEmail)){
            _log.error(String.format("Error: Cannot complete purchase of %s as a gift for %s", entitlement.getName(), friendName));
            MacroPurchase.handleThankYouPage(driver);
            return false;
        }
        return true;
    }

    /**
     * Purchase an entitlement through its PDP page as a gift for a friend.
     *
     * @param driver Selenium WebDriver
     * @param entitlement Entitlement to purchase (note: for browser test,
     * partialURL for PDP must have been defined)
     * @param friendName Name of friend to send the gift to
     * @param message Message for the gift
     * @return true if purchase successful, false otherwise
     */
    public static boolean purchaseGiftForFriend(WebDriver driver, EntitlementInfo entitlement, String friendName, String message) throws InterruptedException {
        return purchaseGiftForFriend(driver, entitlement.getName(), entitlement.getParentName(), entitlement.getOcdPath(), entitlement.getPartialPdpUrl(), friendName, message);
    }

    /**
     * Purchase an entitlement through its PDP page as a gift for a friend, and
     * return the order number.
     *
     * @param driver Selenium WebDriver
     * @param entitlement Entitlement to purchase (note: for browser test,
     * partialURL for PDP must have been defined)
     * @param friendName Name of friend to send the gift to
     * @param message Message for the gift
     * @return The order number if purchase successful, null otherwise
     */
    public @Nullable
    static String purchaseGiftForFriendReturnOrderNumber(WebDriver driver, EntitlementInfo entitlement, String friendName, String message) {
        String entitlementName = entitlement.getName();
        // Load 'GDP' page
        if (!MacroGDP.loadGdpPage(driver, entitlement)) {
            _log.error(String.format("Error: Cannot open 'PDP' page for %s", entitlementName));
            return null;
        }
        // 'Friends Selection', 'Gift Message' dialogs
        if (!prepareGiftPurchase(driver, entitlementName, entitlement.getOfferId(), friendName, message)) {
            _log.error(String.format("Error: Cannot prepare purchase of %s as a gift for %s", entitlementName, friendName));
            return null;
        }
        // Handle 'Purchase Info', 'Review Order', and 'Thank You' pages,
        // extracting the order number from the 'Thank You' page
        return MacroPurchase.completePurchaseReturnOrderNumber(driver);
    }

    /**
     * Purchase an entitlement through its PDP page as a gift for a friend,
     * using a default message.
     *
     * @param driver Selenium WebDriver
     * @param entitlement Entitlement to purchase (note: for browser test,
     * partialURL for PDP must have been defined)
     * @param friendName Name of friend to send gift to
     * @return true if purchase successful, false otherwise
     */
    public static boolean purchaseGiftForFriend(WebDriver driver, EntitlementInfo entitlement, String friendName) throws InterruptedException {
        return purchaseGiftForFriend(driver, entitlement, friendName, DEFAULT_GIFT_MESSAGE);
    }

    /**
     * Purchase an entitlement through its PDP page as a gift for a friend,
     * using a default message, and return the order number.
     *
     * @param driver Selenium WebDriver
     * @param entitlement Entitlement to purchase (note: for browser test,
     * partialURL for PDP must have been defined)
     * @param friendName Name of friend to send the gift to
     * @return The order number if purchase successful, null otherwise
     */
    public @Nullable
    static String purchaseGiftForFriendReturnOrderNumber(WebDriver driver, EntitlementInfo entitlement, String friendName) {
        return purchaseGiftForFriendReturnOrderNumber(driver, entitlement, friendName, DEFAULT_GIFT_MESSAGE);
    }

    /**
     * Complete purchasing an entitlement as a gift from the 'From Gift Message Dialog'
     *
     * @param driver Selenium WebDriver
     * @return true if the entitlement is successfully purchased and the thank you
     * page is closed
     */
    public static boolean completePurchaseGiftFromGiftMessageDialog(WebDriver driver) {
        new GiftMessageDialog(driver).clickNext();
        if (!MacroPurchase.completePurchaseAndCloseThankYouPage(driver)) {
            _log.error(String.format("Error: Cannot complete purchasing gift"));
            return false;
        }
        return true;
    }
}