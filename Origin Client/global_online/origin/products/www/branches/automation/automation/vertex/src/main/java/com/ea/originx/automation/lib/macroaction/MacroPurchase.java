package com.ea.originx.automation.lib.macroaction;

import com.ea.originx.automation.lib.helpers.I18NUtil;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.dialog.*;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPCurrenySelector;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.pageobjects.store.ReviewOrderPage;
import com.ea.originx.automation.lib.pageobjects.store.ThankYouPage;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.math.BigDecimal;
import java.math.RoundingMode;
import java.util.ArrayList;
import javax.annotation.Nullable;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.json.JSONArray;
import org.json.JSONObject;
import org.openqa.selenium.StaleElementReferenceException;
import org.openqa.selenium.WebDriver;

/**
 * Macro class containing static methods for multi-step actions related to
 * purchasing.
 *
 * @author glivingstone
 * @author palui
 */
public final class MacroPurchase {

    public static final String DEFAULT_GIFT_MESSAGE = "An AWESOME Gift from EA, Just for You!";
    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor - make private as class should not be instantiated
     */
    private MacroPurchase() {
        // Do nothing
    }

    /**
     * Handle credit card input for the 'Payment Info' page.
     *
     * @param driver             Selenium WebDriver
     * @param saveCreditCardInfo Set to true if credit card info is to be saved
     * @return true if 'Payment Info' page opens for entering credit card info,
     * false otherwise
     */
    public static boolean handlePaymentInfoPage(WebDriver driver, boolean saveCreditCardInfo) {
        VaultPurchaseWarning vaultPurchaseWarning = new VaultPurchaseWarning(driver);
        if (vaultPurchaseWarning.waitIsVisible()) {
            vaultPurchaseWarning.clickPurchaseAnywaysCTA();
        }
        PaymentInformationPage paymentInfoPage = new PaymentInformationPage(driver);
        paymentInfoPage.waitForPaymentInfoPageToLoad();
        if (!paymentInfoPage.verifyPaymentInformationReached()) {
            _log.error("Error: 'Payment info' page is not open");
            return false;
        }
        if (new I18NUtil().getLocale().getCountry().equals("KR")) { // if korea, select card
            paymentInfoPage.selectCreditCardForKoreaCheckout();
            paymentInfoPage.selectInternationalCard();
        }
        paymentInfoPage.enterCreditCardDetails();
        if (saveCreditCardInfo) {
            paymentInfoPage.clickSaveCreditCardInfoCheckbox();
        }
        Waits.sleep(2000); // Button is immediately enabled, but there is an animation
        paymentInfoPage.clickOnProceedToReviewOrderButton();
        return true;
    }

    public static boolean handlePaymentInfoPageForProdPurchases(WebDriver driver, String entitlementName, String userEmail) throws IOException {
        VaultPurchaseWarning vaultPurchaseWarning = new VaultPurchaseWarning(driver);
        if (vaultPurchaseWarning.waitIsVisible()) {
            vaultPurchaseWarning.clickPurchaseAnywaysCTA();
        }
        PaymentInformationPage paymentInfoPage = new PaymentInformationPage(driver);
        paymentInfoPage.waitForPaymentInfoPageToLoad();
        paymentInfoPage.switchToPaymentByPaypalAndSkipToReviewOrderPage();
        return completePurchaseByPaypalOffCode(driver, entitlementName, userEmail);
    }

    /**
     * Handle credit card input for the Yandex 'Payment Info' page.
     *
     * @param driver Selenium WebDriver
     * @return true if payment information successfully entered
     */
    public static boolean handleYandexPaymentInfoPage(WebDriver driver) {
        YandexDialog yandexDialog = new YandexDialog(driver);
        Waits.pollingWait(() -> yandexDialog.verifyYandexDialogVisible());
        yandexDialog.completePurchase();
        if (new ThankYouPage(driver).verifyThankYouPageReached()) {
            return true;
        }
        return false;
    }

    /**
     * Handle credit card input for the 'Payment Info' page, default to not
     * saving credit card info.
     *
     * @param driver Selenium WebDriver
     * @return true if 'Payment Info' page opens for entering credit card info,
     * false otherwise
     */
    public static boolean handlePaymentInfoPage(WebDriver driver) {
        return handlePaymentInfoPage(driver, false);
    }

    /**
     * Handle credit card input for the 'Payment Info' page. This is for the
     * Subscribe and Save checkout.
     *
     * @param driver             Selenium WebDriver
     * @param saveCreditCardInfo Set to true if credit card info is to be saved
     * @return true if 'Payment Info' page opens for entering credit card info,
     * false otherwise
     */
    public static boolean handlePaymentInfoPageSubAndSave(WebDriver driver, boolean saveCreditCardInfo) {
        PaymentInformationPage paymentInfoPage = new PaymentInformationPage(driver);
        paymentInfoPage.waitForPaymentInfoPageToLoad();
        if (!paymentInfoPage.verifyPaymentInformationReached()) {
            _log.error("Error: 'Payment info' page is not open");
            return false;
        }
        paymentInfoPage.enterCreditCardDetails();
        if (saveCreditCardInfo) {
            paymentInfoPage.clickSaveCreditCardInfoCheckbox();
        }
        Waits.sleep(2000); // Button is immediately enabled, but there is an animation
        paymentInfoPage.clickOnNextButton();
        return true;
    }

    /**
     * Handle credit card input for 'Payment Info' page for Subscribe and Save,
     * default to not saving credit card info.
     *
     * @param driver Selenium WebDriver
     * @return true if 'Payment Info' page opens for entering credit card info,
     * false otherwise
     */
    public static boolean handlePaymentInfoPageSubAndSave(WebDriver driver) {
        return handlePaymentInfoPageSubAndSave(driver, false);
    }

    /**
     * Clicks the 'Pay Now' button on the 'Review Order' page. The case where
     * the 'Thank You' page also has a 'Pay Now' button will not retry to click
     * the button (Ex.: purchasing dlc for 'Mass Effect 3') '
     *
     * @param driver        Selenium WebDriver
     * @param isWithRetries if there should be retries for clicking the 'Pay
     * Now' button
     * @return true if 'Review Order' page opens for clicking the button, false
     * otherwise
     */
    public static boolean handleReviewOrderPage(WebDriver driver, boolean isWithRetries) {
        ReviewOrderPage reviewOrderPage = new ReviewOrderPage(driver);
        reviewOrderPage.waitForPageToLoad();
        if (!Waits.pollingWait(reviewOrderPage::verifyReviewOrderPageReached)) {
            _log.error("Error: 'Review Order' page is not open");
            return false;
        }
        reviewOrderPage.clickPayNow(isWithRetries);
        return Waits.pollingWait(() -> !reviewOrderPage.isTitleVisibleAndCorrect(), 20000, 1000, 3000); // makes sure we aren't on the review order page
    }

    /**
     * Clicks the 'Pay Now' button on the 'Review Order' page with retries.
     *
     * @param driver Selenium WebDriver
     * @return true if 'Review Order' page opens for clicking the button, false
     * otherwise
     */
    public static boolean handleReviewOrderPage(WebDriver driver) {
        return handleReviewOrderPage(driver, true);
    }

    /**
     * Clicks the 'Close' button on the 'Thank You' page.
     *
     * @param driver Selenium WebDriver
     * @return true if 'Thank You' page opens for clicking the button, false
     * otherwise
     */
    public static boolean handleThankYouPage(WebDriver driver) {
        ThankYouPage thankYouPage = new ThankYouPage(driver);
        thankYouPage.waitForPageToLoad();
        try {
            thankYouPage.waitForThankYouPageToLoad();
        } catch (StaleElementReferenceException e) {
            return false;
        }
        thankYouPage.clickCloseButton();
        return true;
    }

    /**
     * Complete a purchase once you are already on the 'Payment info' page. This
     * will fill in all the credit card information, then handle the 'Review
     * Order' page.
     *
     * @param driver             Selenium WebDriver
     * @param saveCreditCardInfo Set to true if credit card info is to be saved
     * @return true if successfully completed purchase, false otherwise
     */
    public static boolean completePurchase(WebDriver driver, boolean saveCreditCardInfo) {
        return handlePaymentInfoPage(driver, saveCreditCardInfo)
                && handleReviewOrderPage(driver);
    }

    /**
     * Complete a purchase once you are already on the 'Payment info' page. This
     * will fill in all the credit card information, then handles the 'Review
     * Order' page.
     *
     * @param driver Selenium WebDriver
     * @return true if successfully completed purchase, false otherwise
     */
    public static boolean completePurchase(WebDriver driver) {
        return completePurchase(driver, false);
    }

    /**
     * Clicks the 'Primary' CTA in the GDP
     *
     * @param driver WebDriver driver
     * @param entitlement Entitlement to purchase
     * @return true if the 'Payment info page' is reached, false otherwise
     */
    public static boolean clickPurchaseEntitlementAndWaitForPurchaseDialog(WebDriver driver, EntitlementInfo entitlement) {
        return clickPurchaseEntitlementAndWaitForPurchaseDialog(driver, entitlement.getParentName(), entitlement.getOfferId(), entitlement.getOcdPath(), entitlement.getPartialPdpUrl());
    }

    /**
     * Verify 'Payment Information' page reached, by clicking GDP 'Get the game'
     * CTA and the going through 'Access Interstitial' page and 'Offer
     * Selection' page for a specific entitlement edition
     *
     * @param driver Selenium WebDriver
     * @param ocdPath the ocdPath of the entitlement
     * @return true if the 'Payment Information' is visible, false otherwise
     */
    public static boolean clickPurchaseEntitlementAndWaitForPurchaseDialog(WebDriver driver, String entitlementName, String offerId, String ocdPath, String partialPdpUrl) {
        if (!MacroGDP.loadGdpPage(driver, entitlementName, partialPdpUrl)) {
            _log.error(String.format("Cannot open GDP page for: %s", entitlementName));
            return false;
        }

        // For currency entitlements, first select the currency
        GDPCurrenySelector gdpCurrenySelector = new GDPCurrenySelector(driver);
        if (gdpCurrenySelector.isCurrencySelectorVisible()) {
            gdpCurrenySelector.selectByValue(offerId);
        }

        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        if (gdpActionCTA.isBuyCTAVisible()) { // Purchase a single edition non vault entitlement with 'Buy with <price>'
            gdpActionCTA.clickBuyCTA();
        } else if (gdpActionCTA.isGetTheGameCTAVisible()) { // Purchase a vault entitlement with 'Get The Game'
            gdpActionCTA.clickGetTheGameCTA();
            AccessInterstitialPage accessInterstitialPage = new AccessInterstitialPage(driver);
            accessInterstitialPage.verifyOSPInterstitialPageReached(); // Navigates to Access Interstitial Page
            if (accessInterstitialPage.isBuyGameCTAVisible()) { // For a single edition with 'Buy with <price>'
                accessInterstitialPage.clickBuyGameCTA();
            } else {
                accessInterstitialPage.clickBuyGameOSPCTA(); // For a multi edition with 'Buy the Game'
                OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver); // Navigates to the Offer Selection Page
                offerSelectionPage.verifyOfferSelectionPageReached();
                offerSelectionPage.clickPrimaryButton(ocdPath);
            }
        } else if (gdpActionCTA.isBuyOSPCTAVisible()) { // Purchase a non vault multi edition with 'Buy'
            gdpActionCTA.clickBuyOSPCTA();
            OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver); // Navigates to the OSP Page
            offerSelectionPage.verifyOfferSelectionPageReached();
            offerSelectionPage.clickPrimaryButton(ocdPath);
        } else if (gdpActionCTA.verifyAddToLibraryVaultGameCTAVisible()) { // Purchase a vault single edition as a subscriber with dropdown 'Buy'
            gdpActionCTA.selectDropdownBuy();
            VaultPurchaseWarning vaultPurchaseWarning = new VaultPurchaseWarning(driver);
            if (vaultPurchaseWarning.waitIsVisible()) {
                vaultPurchaseWarning.clickPurchaseAnywaysCTA();
            }

            AccessInterstitialPage accessInterstitialPage = new AccessInterstitialPage(driver);
            if (accessInterstitialPage.verifyOSPInterstitialPageReached()) {
                accessInterstitialPage.clickBuyGameOSPCTA();
            }

            OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);

            // Navigates to the Offer Selection Page if more than 1 version
            // Goes to payment info dialog if only version
            if (offerSelectionPage.verifyOfferSelectionPageReached()) {
                offerSelectionPage.clickPrimaryButton(ocdPath);

                if (vaultPurchaseWarning.waitIsVisible()) {
                    vaultPurchaseWarning.clickPurchaseAnywaysCTA();
                }
            }
        }
        // For some entitlements a dialog appears to warn the user they are about to buy extra content
        DlcPurchasePrevention dlcPurchasePrevention = new DlcPurchasePrevention(driver, offerId);
        if (dlcPurchasePrevention.waitForVisible()) {
            dlcPurchasePrevention.clickContinuePurchaseButton();
        }

        // For some entitlements a dialog appears that asks if the user wants to add additional dlc to the overall purchase
        DlcUpSellSuggestionDialog dlcUpSellSuggestionDialog = new DlcUpSellSuggestionDialog(driver, offerId);
        if (dlcUpSellSuggestionDialog.waitForVisible()) {
            dlcUpSellSuggestionDialog.clickContinueWithoutAddingDlcButton();
        }
        
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        return paymentInformationPage.verifyPaymentInformationReached();
    }
    
    /**
     * Complete a purchase once you are already on the 'Payment info' page. This
     * will fill in all the credit card information, then handle the 'Review
     * Order' page and click the 'Pay Now' button without retries for the case
     * where the 'Thank you' page also has a 'Pay Now' button (Ex.: purchasing
     * dlc for 'Mass Effect 3')
     *
     * @param driver Selenium WebDriver
     * @return true if successfully completed purchase, false otherwise
     */
    public static boolean completePurchaseWithPurchasingPoints(WebDriver driver) {
        return handlePaymentInfoPage(driver, false)
                && handleReviewOrderPage(driver, false);
    }

    /**
     * Complete a purchase once you are already on the 'Payment info' page. This
     * will fill in all the credit card information, then handle the
     * reviewOrderPage, and finally close the 'Thank You' page.
     *
     * @param driver Selenium WebDriver
     * @return true if successfully completed purchase and closed the 'Thank
     * You' page, false otherwise
     */
    public static boolean completePurchaseAndCloseThankYouPage(WebDriver driver) {
        return completePurchase(driver, false) && handleThankYouPage(driver);
    }

    /**
     * Purchase an entitlement through its PDP page.
     *
     * @param driver      Selenium WebDriver
     * @param entitlement Entitlement to purchase (note: for browser test,
     *                    partialURL for PDP must have been defined)
     * @return true if purchase successful, false otherwise
     */
    public static boolean purchaseEntitlement(WebDriver driver, EntitlementInfo entitlement) {
        return purchaseEntitlement(driver, entitlement.getParentName(), entitlement.getOfferId(), entitlement.getOcdPath(), entitlement.getPartialPdpUrl());
    }

    /**
     * Purchase an entitlement through its PDP page.
     *
     * @param driver          Selenium WebDriver
     * @param entitlementName Name of entitlement to purchase
     * @param offerId         Offer ID of entitlement to purchase
     * @param ocdPath         OCD Path of the entitlement to purchase
     * @param partialPdpUrl   Partial PDP URL for the entitlement to (required for
     *                        browser test only)
     * @return true if purchase successful, false otherwise
     */
    public static boolean purchaseEntitlement(WebDriver driver, String entitlementName, String offerId, String ocdPath, String partialPdpUrl) {
        clickPurchaseEntitlementAndWaitForPurchaseDialog(driver, entitlementName, offerId, ocdPath, partialPdpUrl);
        return completePurchaseAndCloseThankYouPage(driver);
    }

    /**
     * Complete a purchase once you are already on the credit card information
     * page. This will fill in all the credit card information, then complete on
     * the 'Review Order' page and close the 'Thank You' page.
     *
     * @param driver Selenium WebDriver
     * @return Order number if found, null if not found
     */
    public @Nullable
    static String completePurchaseReturnOrderNumber(WebDriver driver) {
        if (completePurchase(driver)) {
            //Thank You page
            ThankYouPage thankYouPage = new ThankYouPage(driver);
            thankYouPage.waitForThankYouPageToLoad();
            String orderNumber = thankYouPage.getOrderNumber().trim();
            thankYouPage.clickCloseButton();
            return orderNumber;
        }
        _log.error("Cannot complete purchase");
        return null;
    }

    /**
     * Purchase an entitlement through its PDP page.
     *
     * @param driver      Selenium WebDriver
     * @param entitlement Entitlement to purchase (note: for browser test,
     *                    partialURL for PDP must have been defined)
     * @return Order number from the 'Thank you' page if found, null if not
     * found
     */
    public @Nullable
    static String purchaseEntitlementReturnOrderNumber(WebDriver driver, EntitlementInfo entitlement) {
        clickPurchaseEntitlementAndWaitForPurchaseDialog(driver, entitlement);
        return completePurchaseReturnOrderNumber(driver);
    }

    /**
     * Purchase an entitlement when the DLC dialog appears. This will fill in
     * all the credit card information, then complete on the 'Review Order' page
     * and close the 'Thank You' page.
     *
     * @param driver Selenium WebDriver
     * @param entitlement the EntitlementInfo for the game that needs to be
     * added to the 'Game Library'
     * @return Order number if found, null otherwise
     */
    public @Nullable
    static String completePurchaseHandleDLCReturnOrderNumber(WebDriver driver, EntitlementInfo entitlement) {
        if (!MacroGDP.loadGdpPage(driver, entitlement)) {
            _log.error(String.format("Cannot open PDP page for: %s", entitlement.getName()));
            return null;
        }
        if (clickPurchaseEntitlementAndWaitForPurchaseDialog(driver, entitlement)) {
            return completePurchaseReturnOrderNumber(driver);
        } else {
            _log.error("Could not buy entitlement.");
            return null;
        }
    }

    /**
     * Verify tax in invoice after purchasing Origin Access
     *
     * @param invoice The invoice to check
     * @param countryEnum The country in which the order is placed
     * @return true if tax rate and tax calculation is correct, false otherwise
     */
    public static boolean verifyInvoiceTaxOriginAccess(String invoice, CountryInfo.CountryEnum countryEnum) {
        return verifyInvoiceTaxItemName(invoice, countryEnum, "Origin Access");
    }

    /**
     * Purchase an entitlement using a 100% prod off code(From GDP to completion of purchase)
     *
     * @param driver Selenium driver
     * @param entitlement Entitlement info
     * @param userEmail Email of the user making the purchase
     * @return true on successful entitlement purchase
     * @throws IOException
     */
    public static boolean purchaseThroughPaypalOffCode(WebDriver driver, EntitlementInfo entitlement, String userEmail) throws IOException {
        clickPurchaseEntitlementAndWaitForPurchaseDialog(driver, entitlement);
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.selectPayPal();
        Waits.pollingWait(() -> paymentInformationPage.verifyPayPalSelected());

        paymentInformationPage.clickContinueToPayPal();
        return completePurchaseByPaypalOffCode(driver, entitlement.getName(), userEmail);
    }
    
    /**
     * Purchase an entitlement using a 100% prod off code(From 'Payment Information' page to completion of purchase)
     *
     * @param driver Selenium driver
     * @param entitlement Entitlement info
     * @param userEmail Email of the user making the purchase
     * @return true on successful entitlement purchase
     * @throws IOException
     */
    public static boolean purchaseThroughPaypalOffCodeFromPaymentInformationPage(WebDriver driver, EntitlementInfo entitlement, String userEmail) throws IOException {
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        paymentInformationPage.selectPayPal();
        Waits.pollingWait(() -> paymentInformationPage.verifyPayPalSelected());

        paymentInformationPage.clickContinueToPayPal();
        return completePurchaseByPaypalOffCode(driver, entitlement.getName(), userEmail);
    }

    /**
     * Handles payment through 100% off code on review order page
     *
     * @param driver Selenium driver
     * @param entitlementName Name of the entitlement being purchased
     * @param userEmail Email of the user making the purchase
     * @return true on successful entitlement purchase
     * @throws IOException Exception thrown by unsuccessful Crs API call
     */
    public static boolean completePurchaseByPaypalOffCode(WebDriver driver, String entitlementName, String userEmail) throws IOException {
        ReviewOrderPage reviewOrderPage = new ReviewOrderPage(driver);
        Waits.pollingWait (() -> reviewOrderPage.verifyReviewOrderPageReached());
        reviewOrderPage.applyPromoCodeForProdPurchase(entitlementName, userEmail);
        reviewOrderPage.clickPayNow();
        Waits.sleep(5000); // Wait for payment processing to complete
        //the 'paypal popup' can remain on screen for a while so we can close it
        ArrayList<String> tab_handles = new ArrayList<>(driver.getWindowHandles());
        int number_of_tabs = tab_handles.size();
        if(number_of_tabs > 2) {
            driver.switchTo().window(tab_handles.get(2));
            new MainMenu(driver).clickCloseButton();// 
        }
        return Waits.pollingWait(() -> !reviewOrderPage.isTitleVisibleAndCorrect(), 20000, 1000, 3000)
                && handleThankYouPage(driver);
    }

    /**
     * Verify tax in invoice after purchasing an item
     *
     * @param invoice the order number of the invoice
     * @param countryEnum The country in which the order is placed
     * @param itemName the item name to verify
     * @return true if tax rate and tax calculation is correct, false otherwise
     */
    private static boolean verifyInvoiceTaxItemName(String invoice, CountryInfo.CountryEnum countryEnum, String itemName) {
        JSONArray invoiceJSON = new JSONObject(invoice)
                .getJSONObject("invoice")
                .getJSONObject("lineItems")
                .getJSONArray("lineItem");
        JSONObject item = getItemBasedOnName(invoiceJSON, itemName);

        BigDecimal subtotalPrice = new BigDecimal(item.getString("subtotal"));
        BigDecimal taxPrice = new BigDecimal(item.getString("tax"));
        BigDecimal taxRate = new BigDecimal(((JSONObject) item.getJSONObject("taxes").getJSONArray("tax").get(0)).getString("rate")).setScale(3, RoundingMode.FLOOR);
        BigDecimal totalPrice = new BigDecimal(item.getString("total"));

        _log.debug("Tax: " + taxPrice + "\t\tTax Rate: " + taxRate + "\t\tTotal: " + totalPrice + "\tSubtotal: " + subtotalPrice);

        return verifyInvoiceCalculation(countryEnum, subtotalPrice, taxPrice, taxRate, totalPrice);
    }

    /**
     * Get the JSONObject of the item
     *
     * @param jsonArray The JSONArray array if item in an invoice
     * @param itemName the item name to search for
     * @return JSONObject of the item in the list
     */
    private static JSONObject getItemBasedOnName(JSONArray jsonArray, String itemName) {
        for (int i = 0; i < jsonArray.length(); i++) {
            JSONObject tempJsonObj = jsonArray.getJSONObject(i);
            if (tempJsonObj.getString("itemName").contains(itemName)) {
                return tempJsonObj;
            }
        }
        throw new RuntimeException("Item has not been found");
    }

    /**
     * Verify invoice calculation is correct based on given parameters
     *
     * @param countryEnum The country in which the transaction took place
     * @param subtotalPrice Subtotal price of the transaction
     * @param taxPrice The tax price that was added in the transaction
     * @param taxRate The rate of tax in which the user was charged
     * @param totalPrice The total price of the transaction that includes the tax amount
     * @return true if tax rate and tax calculation is correct, false otherwise
     */
    private static boolean verifyInvoiceCalculation(CountryInfo.CountryEnum countryEnum, BigDecimal subtotalPrice, BigDecimal taxPrice, BigDecimal taxRate, BigDecimal totalPrice) {
        String countryName = countryEnum.getCountry();
        if (countryName.equals("Canada") || countryName.equals("United States of America"))
            return verifyTaxCalculation(subtotalPrice, taxPrice, taxRate, totalPrice, countryEnum.getTaxRate());
        else
            return subtotalPrice.equals(totalPrice);
    }

    /**
     * Verify if the tax amount added to the total price is correct
     *
     * @param subtotalPrice Subtotal price of the transaction
     * @param taxPrice The tax price that was added in the transaction
     * @param taxRate The rate of tax in which the user was charged
     * @param totalPrice The total price of the transaction that includes the tax amount
     * @return true if tax rate and tax calculation is correct, false otherwise
     */
    private static boolean verifyTaxCalculation(BigDecimal subtotalPrice, BigDecimal taxPrice, BigDecimal taxRate, BigDecimal totalPrice, BigDecimal actualTaxRate) {
        BigDecimal taxPriceCalc = subtotalPrice.multiply(actualTaxRate);
        BigDecimal taxPriceRounded = taxPriceCalc.setScale(2, RoundingMode.FLOOR);
        BigDecimal taxRateCalc = taxPriceRounded.divide(subtotalPrice, 10, RoundingMode.HALF_UP);

        if (taxRateCalc.compareTo(actualTaxRate) < 0) {
            taxPriceRounded = taxPriceCalc.setScale(2, RoundingMode.CEILING); //Round up
            taxRateCalc = taxPriceRounded.divide(subtotalPrice, 10, RoundingMode.HALF_UP);
        }

        BigDecimal taxRateRounded = taxRateCalc.setScale(3, RoundingMode.FLOOR);
        BigDecimal totalPriceCalc = subtotalPrice.add(taxPriceRounded);

        _log.debug("Tax(R): " + taxPriceRounded + "\tTax Rate(R): " + taxRateRounded + "\tTotal: " + totalPriceCalc);

        return (taxPriceRounded.equals(taxPrice)) && (taxRateRounded.equals(taxRate)) && (totalPriceCalc.equals(totalPrice));
    }
}
