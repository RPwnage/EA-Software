package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.helpers.I18NUtil;
import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import static com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage.CLOSE_BUTTON_LOCATOR;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import static com.ea.originx.automation.lib.resources.OriginClientData.ORIGIN_ACCESS_DISCOUNT_COLOUR;

import com.ea.vx.originclient.net.helpers.CrsHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;

import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.text.DecimalFormat;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.StaleElementReferenceException;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * A page object that represents the 'Review Order' page.
 *
 * @author mkalaivanan
 */
public class ReviewOrderPage extends EAXVxSiteTemplate {

    protected static final By IFRAME_LOCATOR = By.cssSelector("origin-iframe-modal .origin-iframemodal .origin-iframemodal-content iframe");
    protected static final By IFRAME_TITLE_LOCATOR = By.cssSelector(".checkout-content .otkmodal-header .otkmodal-title");
    protected static final By UPGRADE_ENTITLEMENT_NAME_LOCATOR = By.cssSelector(".cart-item .upgrade .otkc-small strong");
    protected static final By UPGRADE_ENTITLEMENT_BUTTON_LOCATOR = By.cssSelector(".cart-item .upgrade .otkbtn.otkbtn-light");
    protected static final By UPSELL_ENTITLEMENT_NAME_LOCATOR = By.cssSelector(".cart-item .upsell .otkc-small strong");
    protected static final By UPSELL_ENTITLEMENT_BUTTON_LOCATOR = By.cssSelector(".cart-item .upsell .otkbtn.otkbtn-light");
    protected static final By ENTITLEMENT_LIST_LOCATOR = By.cssSelector(".cart-item .cart-item-details .otkc strong");
    protected static final By GIFT_DETAILS_LOCATOR = By.cssSelector(".cart-item .cart-item-details .gift-details h6");
    protected static final String EXPECTED_GIFT_FOR_USER_MESSAGE_TMPL = "Gift for %s";
    protected static final By ENTITLEMENT_PRICE_LIST_LOCATOR = By.cssSelector(".cart-item .cart-item-price .otkc");
    protected static final By TAX_COST_LOCATOR = By.cssSelector(".cart-total-price .cart-tax-cost .otkc");
    protected static final By TOTAL_PRICE_LOCATOR = By.cssSelector(".cart-total-price .cart-total-cost .otktitle-2");
    protected static final By FREE_TRIAL_LOCATOR = By.cssSelector(".cart-item-price-wrapper .otkc");
    protected static final By PACKART_LOCATOR = By.cssSelector(".cart-item img");
    protected static final By IFRAME_CLOSE_CIRCLE_LOCATOR = By.cssSelector(".otkmodal-dialog.otkmodal-lg .otkicon.otkicon-closecircle");
    protected static final String[] PRICE_SALE_KEYWORDS = {"origin", "access", "discount"};
    protected static final String[] EXPECTED_FREE_TRIAL_KEYWORDS = {"free", "trial"};
    protected static final String YEAR = "year";
    protected static final String MONTH = "month";
    
    // Order Completion Button
    protected static final By COMPLETE_ORDER_BUTTON_LOCATOR = By.cssSelector(".checkout-content.otkmodal-content .otkmodal-footer .otkbtn.otkbtn-primary");
    protected static final By PAY_NOW_BUTTON_LOCATOR = COMPLETE_ORDER_BUTTON_LOCATOR;
    protected static final By START_MEMBERSHIP_BUTTON_LOCATOR = COMPLETE_ORDER_BUTTON_LOCATOR;
    protected static final By PRICE_LOCATOR = By.cssSelector(".cart-total-cost h4");
    protected static final By PER_MONTH_LOCATOR = By.cssSelector("div.cart-total-cost div h4 span");

    // Edit Payment Options Button
    protected static final By EDIT_PAYMENT_OPTIONS_BUTTON_LOCATOR = By.cssSelector("#reviewOrder .checkout-review-col-right .payment-options .edit-payment .otktitle-4");

    // Terms of Service Footer
    protected static final String TOS_FOOTER_CSS = ".otkmodal-footer .checkout-disclaimer";
    protected static final By TOS_FOOTER_LOCATOR = By.cssSelector(TOS_FOOTER_CSS);
    protected static final By TOS_LINKS_LOCATOR = By.cssSelector(TOS_FOOTER_CSS + " .otkc-small > a");
    protected static final String[] ORIGIN_ACCESS_EXPECTED_KEYWORDS_TOS = {"Terms of Sale", "Origin Access", "Terms and Conditions", "EULA", "subscription", "Visit EA Help"};
    protected static final String[] ENTITLEMENT_EXPECTED_KEYWORDS_TOS = {"Terms of Sale", "EULA", "User Agreement", "Online Disclaimer", "About Origin", "purchase", "Visit EA Help"};

    // Refund 
    protected static final By ENTITLEMENT_REFUND_PRICE_LIST_LOCATOR = By.cssSelector("#refund-view .cart-total-price .cart-total-cost h4");

    // Cost and Tax
    protected static final String CHECKOUT_REVIEW_CSS = "#reviewOrder .checkout-review-col-left";
    protected static final By DISCOUNTED_PRICE_LOCATOR = By.cssSelector(CHECKOUT_REVIEW_CSS + " .cart-item-price .cart-item-price-wrapper .otkc");
    protected static final By TAX_TEXT_LOCATOR = By.cssSelector(CHECKOUT_REVIEW_CSS + " .cart-total-price .cart-tax");
    protected static final By PRICE_SALE_LOCATOR = By.cssSelector(CHECKOUT_REVIEW_CSS + " .cart-item-price .cart-item-price-wrapper .otkprice-sale");
    protected static final By ORIGINAL_PRICE_LOCATOR = By.cssSelector(CHECKOUT_REVIEW_CSS + " .cart-item-price .cart-item-price-wrapper .otkprice-original");

    //Origin Access Free Trial
    protected static final String CART_FREE_TRIAL_CSS = ".cart-item-price-free-trial .cart-item-price-wrapper ";
    protected static final By ORIGIN_ACCESS_FREE_TRIAL_LOCATOR = By.cssSelector(CART_FREE_TRIAL_CSS + "h4.otkc");
    protected static final By ORIGIN_ACCESS_PLAN_LOCATOR = By.cssSelector(CART_FREE_TRIAL_CSS + ".otkc-small");
    protected static final String[] EXPECTED_FREE_TRIAL_LOCATOR = {"free", Integer.toString(OriginClientData.ORIGIN_ACCESS_TRIAL_LENGTH)};

    // BioWare points
    protected static final By BIOWARE_POINTS_BALANCE_LOCATOR = By.cssSelector(".origin-checkoutusingbalance-cart-current-balance-value");

    //Promo code input
    protected static final By ENTER_PROMO_CODE_BUTTON_LOCATOR = By.cssSelector(".cart-promo-code");
    protected static final By PROMO_CODE_INPUT_LOCATOR = By.cssSelector("#promoCodeInputBox");
    protected static final By APPLY_PROMO_CODE_BUTTON_LOCATOR = By.cssSelector(".submit-btn-with-loader-container");

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ReviewOrderPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Wait for the 'Review Order' page to load.
     */
    public void waitForReviewOrderPageToLoad() {
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 30);
        waitForPageToLoad();
        // if current frame is iframe, it does not do anything
        if (!checkIfFrameIsActive("iframe")) {
            waitForFrameAndSwitchToIt(waitForElementVisible(IFRAME_LOCATOR, 10));
        }
    }

    /**
     * Checks if title is visible and correct.
     *
     * @return true if visible and correct, false otherwise
     */
    public boolean isTitleVisibleAndCorrect() {
        return waitIsElementVisible(IFRAME_TITLE_LOCATOR) && StringHelper.containsAnyIgnoreCase(waitForElementVisible(IFRAME_TITLE_LOCATOR).getText(),
                I18NUtil.getMessage("reviewOrderPageHeaderText"));
    }

    /**
     * Verify the 'Review Order' page is reached if the header has the correct text.
     *
     * @return true if 'review' and 'order' are in the title, false otherwise
     */
    public boolean verifyReviewOrderPageReached() {
        for (int i = 0; i < 5; i++) {
            try {
                waitForInAnyFrame(PAY_NOW_BUTTON_LOCATOR);
                return StringHelper.containsAnyIgnoreCase(waitForElementVisible(IFRAME_TITLE_LOCATOR).getText(),
                        I18NUtil.getMessage("reviewOrderPageHeaderText"));
            } catch (StaleElementReferenceException e) {
                // Page is still being updated, wait and try again
                sleep(1000);
            } catch (TimeoutException e) {
                // Page is loading slowly
                sleep(1000);
            }
        }
        return false;
    }

    /**
     * Click on the 'Complete Order' button to complete the purchase.
     */
    public void clickCompleteOrderButton() {
        waitForElementClickable(COMPLETE_ORDER_BUTTON_LOCATOR).click();
    }

    /**
     * Clicks the 'Close' button
     */
    public void clickCloseButton() {
        driver.switchTo().defaultContent();
        waitForElementVisible(CLOSE_BUTTON_LOCATOR).click();
    }

    /**
     * Click on the 'Start Membership' button.
     */
    public void clickStartMembershipButton() {
        waitForElementClickable(START_MEMBERSHIP_BUTTON_LOCATOR).click();
    }

    /**
     * Click on 'Pay Now' to continue the purchase with retries.
     */
    public void clickPayNow() {
        clickPayNow(true);
    }

    /**
     * Clicks the 'Pay Now' button to continue the purchase
     *
     * @param isWithRetries pass in true if you want to try and click the 'Pay Now' button multiple times
     */
    public void clickPayNow(boolean isWithRetries) {
        if (isWithRetries) {
            for (int i = 1; i <= 3; i++) { // retry because sometimes click doesn't work right away
                try {
                    clickOnElement(waitForElementClickable(PAY_NOW_BUTTON_LOCATOR, 10));
                    waitForElementInvisibleByLocator(PAY_NOW_BUTTON_LOCATOR, 10);
                    return;
                } catch (Exception ex) {
                    _log.debug("Clicking on 'Pay Now' button failed and trying it for " + i + "th time");
                }
            }
        } else {
            clickOnElement(waitForElementClickable(PAY_NOW_BUTTON_LOCATOR));
        }
    }

    /**
     * Click on 'Pay Now' button
     * Used when user needs to access other payment methods (eg: PayPal, Sofort)
     */
    public void clickPayNowAndContinueToThirdPartyVendor() {
        clickOnElement(waitForElementClickable(PAY_NOW_BUTTON_LOCATOR));
    }

    /**
     * Gets the price on the 'Review Order' page.
     *
     * @return Price as a String
     */
    public String getPrice() {
        return StringHelper.normalizeNumberString(waitForElementVisible(PRICE_LOCATOR).getText());
    }

    /**
     * Gets the tax price on the 'Review Order' page.
     *
     * @return The tax amount
     */
    public String getTax() {
        return StringHelper.normalizeNumberString(waitForElementVisible(TAX_COST_LOCATOR).getText());
    }

    /**
     * Get gifting details on the 'Review Order' page.
     *
     * @return Gifting details, or throw TimeoutException if not visible
     */
    public String getGiftDetails() {
        return waitForElementVisible(GIFT_DETAILS_LOCATOR).getText();
    }

    /**
     * Verify the (green) 'Gift for <user>' message matches the recipient name.
     *
     * @param username Name of recipient
     * @return true if gift message matches the recipient name, false otherwise, or throw TimeoutException if message is not visible
     */
    public boolean verifyGiftForUser(String username) {
        String expectedGiftingMessage = String.format(EXPECTED_GIFT_FOR_USER_MESSAGE_TMPL, username);
        return StringHelper.containsIgnoreCase(getGiftDetails(), expectedGiftingMessage);
    }

    /**
     * Verify the (green) 'Origin Access Discount Applied' message is displayed
     * and is green for an Origin Access user.
     *
     * @return true if message is displayed and is green, false otherwise
     */
    public boolean verifyOriginAccessDiscountIsVisible() {
        try {
            String header = waitForElementVisible(PRICE_SALE_LOCATOR).getText();
            String textColor = getColorOfElement(waitForElementVisible(PRICE_SALE_LOCATOR));
            boolean isVisible = StringHelper.containsIgnoreCase(header, PRICE_SALE_KEYWORDS);
            boolean isGreen = StringHelper.containsIgnoreCase(textColor, ORIGIN_ACCESS_DISCOUNT_COLOUR);
            return (isVisible && isGreen);
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify the price after Origin Access discount is correct for an Origin
     * Access user.
     *
     * @param expectedPrice Expected price
     * @return true if price is correct, false otherwise
     */
    public boolean verifyPriceAfterDiscountIsCorrect(String expectedPrice) {
        try {
            String price = waitForElementVisible(DISCOUNTED_PRICE_LOCATOR).getText();
            return StringHelper.containsIgnoreCase(price, expectedPrice);
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify price after discount is visible
     *
     * @return true if price after discount is visible, false otherwise
     */
    public boolean verifyPriceAfterDiscountDisplayed() {
        return waitIsElementVisible(DISCOUNTED_PRICE_LOCATOR);
    }

    /**
     * Verify the (green) 'Origin Access Discount Applied' message is displayed
     * for an Origin Access user
     *
     * @return true if message is displayed, false otherwise
     */
    public boolean verifyOriginalPriceIsStrikedThrough() {
        try {
            String textDeco = driver.findElement(ORIGINAL_PRICE_LOCATOR).getCssValue("text-decoration");
            return StringHelper.containsIgnoreCase(textDeco, "line-through");
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * To get the base amount for an entitlement for an Origin Access user.
     *
     * @return The base amount
     */
    public double getBaseAmount() {
        return StringHelper.extractNumberFromText(waitForElementVisible(ENTITLEMENT_PRICE_LIST_LOCATOR).getText());
    }

    /**
     * Get the base amount for an entitlement as a String
     *
     * @return The base amount as a String
     */
    public String getBaseAmountAsString() {
        return waitForElementVisible(ENTITLEMENT_PRICE_LIST_LOCATOR).getText();
    }

    /**
     * Click on the 'Upgrade' button to upgrade the base game entitlement.
     */
    public void clickUpgradeEntitlement() {
        waitForElementVisible(UPGRADE_ENTITLEMENT_BUTTON_LOCATOR).click();
    }

    /**
     * Click on the 'Add' button to add extra content such as add-on or expansion.
     */
    public void clickUpsellEntitlement() {
        waitForElementVisible(UPSELL_ENTITLEMENT_BUTTON_LOCATOR).click();
    }

    /**
     * Verify cart items such as upgrade entitlement and upsell entitlement
     * (=extra content) exist on the 'Review Order' page before calculating
     * total cost.
     *
     * @return true if the items exist on 'Review Order' page, false otherwise
     */
    public boolean verifyUpsellUpgradeCartItemExists() {
        return waitIsElementVisible(UPGRADE_ENTITLEMENT_NAME_LOCATOR) && waitIsElementVisible(UPSELL_ENTITLEMENT_NAME_LOCATOR);
    }

    /**
     * Verify added cart item exists on 'Review Order' page for calculating total
     * cost.
     *
     * @param name Expected entitlement name
     * @return true if the added cart item exists on 'Review Order' page
     */
    public boolean verifyEntitlementExistsForReviewingOrder(String name) {
        List<WebElement> elements = waitForAllElementsVisible(ENTITLEMENT_LIST_LOCATOR);
        for (WebElement element : elements) {
            if (element.getText().equalsIgnoreCase(name)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Verify total cost which is same as tax + price for all entitlements
     * added/upgraded.
     *
     * @return true if the total cost is verified, false otherwise
     */
    public boolean verifyTotalCost() {
        double actualTotalPrice = StringHelper.extractNumberFromText(waitForElementVisible(TOTAL_PRICE_LOCATOR, 30).getText());
        double taxPrice = StringHelper.extractNumberFromText(waitForElementVisible(TAX_COST_LOCATOR).getText());
        double calculatedSubTotal = 0.00;
        double calculatedFinalTotal = 0.00;

        // get list of price
        List<WebElement> elements = waitForAllElementsVisible(ENTITLEMENT_PRICE_LIST_LOCATOR);
        for (WebElement element : elements) {
            double price = StringHelper.extractNumberFromText(element.getText());
            calculatedSubTotal += price;
        }
        calculatedFinalTotal = calculatedSubTotal + taxPrice;
        return actualTotalPrice == Double.parseDouble(new DecimalFormat("0.00").format(calculatedFinalTotal));
    }

    /**
     * Returns the total cost (tax + price for all entitlements added/upgraded).
     *
     * @return The total cost
     */
    public double getTotalCost() {
        return StringHelper.extractNumberFromText(waitForElementVisible(TOTAL_PRICE_LOCATOR).getText());
    }

    /**
     * Verify total price is displayed.
     *
     * @return true if total cost is displayed, false otherwise
     */
    public boolean verifyTotalPriceDisplayed() {
        return waitIsElementVisible(TOTAL_PRICE_LOCATOR);
    }

    /**
     * Verify the free trial for Origin Access exists.
     *
     * @return true if displayed, false otherwise
     */
    public boolean verifyFreeTrialText() {
        return StringHelper.containsAnyIgnoreCase(waitForElementVisible(FREE_TRIAL_LOCATOR).getText(), EXPECTED_FREE_TRIAL_KEYWORDS);
    }

    /**
     * Gets the upgrade entitlement name.
     *
     * @return Upgrade entitlement name
     */
    public String getUpgradeEntitlementName() {
        return waitForElementVisible(UPGRADE_ENTITLEMENT_NAME_LOCATOR).getText();
    }

    /**
     * Gets the upgrade entitlement offer ID.
     *
     * @return Upgrade entitlement offer ID
     */
    public String getUpgradeEntitlementOfferID() {
        return waitForElementVisible(UPGRADE_ENTITLEMENT_BUTTON_LOCATOR).getAttribute("data-value");
    }

    /**
     * Gets the upsell entitlement name.
     *
     * @return Upsell entitlement name
     */
    public String getUpsellEntitlementName() {
        return waitForElementVisible(UPSELL_ENTITLEMENT_NAME_LOCATOR).getText();
    }

    /**
     * Gets the upsell entitlement offer ID.
     *
     * @return Upsell entitlement offer ID
     */
    public String getUpsellEntitlementOfferID() {
        return waitForElementVisible(UPSELL_ENTITLEMENT_BUTTON_LOCATOR).getAttribute("data-value");
    }

    /**
     * Verify that a few key phrases appear in the 'Terms of Service' text. Not
     * checking the text verbatim as the legal text might change over time.
     *
     * @return true if all key phrases are present in the 'Terms of Service' text,
     * false otherwise
     */
    public boolean verifyOriginAccessTermsOfServiceText() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(TOS_FOOTER_LOCATOR).getText(), ORIGIN_ACCESS_EXPECTED_KEYWORDS_TOS);
    }

    /**
     * Verify that a few key phrases appear in the Terms of Service text when purchasing an Entitlement.
     *
     * @return true if all key phrases are present in the Terms of Service text
     */
    public boolean verifyEntitlementPurchaseTermsOfServiceText() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(TOS_FOOTER_LOCATOR).getText(), ENTITLEMENT_EXPECTED_KEYWORDS_TOS);
    }

    /**
     * Verify that each of the links in the 'Terms of Service' text all open an
     * external browser.
     *
     * @return true if every link opens an external browser, false
     * and log the URL of the offending link otherwise
     */
    public boolean verifyAllTermsOfServiceLinks() {

        boolean linksWork = true;
        List<WebElement> tosLinks = driver.findElements(TOS_LINKS_LOCATOR);
        for (WebElement tosLink : tosLinks) {
            TestScriptHelper.killBrowsers(client);
            tosLink.click();
            if (!(Waits.pollingWaitEx(() -> TestScriptHelper.verifyBrowserOpen(client)))) {
                _log.debug(tosLink.getAttribute("href") + " failed");
                linksWork = false;
            }
            TestScriptHelper.killBrowsers(client);
        }

        return linksWork;
    }

    /**
     * Verify that tax is being displaying, not taking into account region or
     * locale (currency is currently hard coded).
     *
     * @return true if tax is appearing on the 'Review Order' page, false otherwise
     */
    public boolean verifyTaxIsDisplayed() {
        try {
            String taxText = waitForElementVisible(TAX_TEXT_LOCATOR).getText();
            String[] verifiableText = {"Tax", "$"};

            return StringHelper.containsAnyIgnoreCase(taxText, verifiableText);
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Click on the button to move the 'Payment Information' page.
     */
    public void clickEditPaymentOptionsButton() {
        waitForElementClickable(EDIT_PAYMENT_OPTIONS_BUTTON_LOCATOR).click();
    }

    /**
     * Verify the Origin Access Free Trial duration is indicated.
     *
     * @return true if the text is displayed, false otherwise
     */
    public boolean verifyTrialDuration() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(ORIGIN_ACCESS_FREE_TRIAL_LOCATOR).getText(), EXPECTED_FREE_TRIAL_LOCATOR);
    }

    /**
     * Verify the total cost is as expected for the given plan.
     *
     * @param selectedPlan The plan selected by the user
     * @return true if the cost is as expected, false otherwise
     */
    public boolean verifyPlanTotalCost(String selectedPlan) {
        double baseAmount = StringHelper.extractNumberFromText(selectedPlan);
        double tax = baseAmount * 0.05;
        double totalCost = baseAmount + tax;
        double price = StringHelper.extractNumberFromText(waitForElementVisible(ORIGIN_ACCESS_PLAN_LOCATOR).getText());
        double parsedTotalCost = Double.parseDouble(new DecimalFormat("0.00").format(totalCost));
        double parsedPrice = Double.parseDouble(new DecimalFormat("0.00").format(price));
        return parsedPrice == parsedTotalCost;
    }

    /**
     * Verify the plan duration (yearly/monthly) selected by the user is
     * displayed correctly in the page.
     *
     * @param selectedPlan The plan selected by the user
     * @return true if the plan duration matches, false otherwise
     */
    public boolean verifyPlanDuration(String selectedPlan) {
        if (selectedPlan.contains(YEAR)) {
            return StringHelper.containsIgnoreCase(waitForElementVisible(ORIGIN_ACCESS_PLAN_LOCATOR).getText(), YEAR);
        } else if (selectedPlan.contains(MONTH)) {
            return StringHelper.containsIgnoreCase(waitForElementVisible(ORIGIN_ACCESS_PLAN_LOCATOR).getText(), MONTH);
        } else {
            return false;
        }
    }

    /**
     * Verify if pack art exists on the 'Review Order' page.
     *
     * @return true if exists, false otherwise
     */
    public boolean verifyPackArtExists() {
        return waitIsElementVisible(PACKART_LOCATOR);
    }

    /**
     * Verify Origin Access plan duration matches the given plan.
     *
     * @param selectedPlan The plan selected by the user
     * @return true if the plan duration matches, false otherwise
     */
    public boolean verifyCorrectPlanDuration(String selectedPlan) {
        if (selectedPlan.contains(YEAR)) {
            return StringHelper.containsIgnoreCase(waitForElementVisible(PER_MONTH_LOCATOR).getText(), YEAR);
        } else if (selectedPlan.contains(MONTH)) {
            return StringHelper.containsIgnoreCase(waitForElementVisible(PER_MONTH_LOCATOR).getText(), MONTH);
        } else {
            return false;
        }
    }

    /**
     * For the Subscribe and Save 'Review Order' page (Origin Access checkout part),
     * verify that the product displayed is the correct Origin Access plan with
     * appropriate price and tax information.
     *
     * @param planType type of Origin Access plan (monthly or yearly)
     * @return true if the above are correct, false otherwise
     */
    public boolean checkJoinAndSaveOAReview(String planType) {
        String expectedTotalPrice;
        String expectedTax = "";
        if (planType.equalsIgnoreCase(OriginClientData.ORIGIN_ACCESS_PREMIER_MONTHLY_PLAN)) {
            expectedTotalPrice = StringHelper.formatDoubleToPriceAsString(StringHelper.extractNumberFromText(OriginClientData.ORIGIN_ACCESS_MONTHLY_PLAN_PRICE)
                    + (StringHelper.extractNumberFromText(OriginClientData.ORIGIN_ACCESS_MONTHLY_PLAN_PRICE) * 0.05));
            expectedTax = StringHelper.formatDoubleToPriceAsString(StringHelper.extractNumberFromText(OriginClientData.ORIGIN_ACCESS_MONTHLY_PLAN_PRICE) * 0.05);
        } else if (planType.equalsIgnoreCase(OriginClientData.ORIGIN_ACCESS_PREMIER_YEARLY_PLAN)) {
            expectedTotalPrice = StringHelper.formatDoubleToPriceAsString(StringHelper.extractNumberFromText(OriginClientData.ORIGIN_ACCESS_YEARLY_PLAN_PRICE)
                    + (StringHelper.extractNumberFromText(OriginClientData.ORIGIN_ACCESS_YEARLY_PLAN_PRICE) * 0.05));
        } else {
            return false;
        }
        ReviewOrderPage reviewOrderPage = new ReviewOrderPage(driver);
        boolean totalPriceCorrect = reviewOrderPage.getPrice().equals(expectedTotalPrice);
        boolean taxCorrect = reviewOrderPage.getTax().equals(expectedTax);
        boolean correctPlanDuration = reviewOrderPage.verifyCorrectPlanDuration(planType);
        return totalPriceCorrect && taxCorrect && correctPlanDuration;
    }

    /**
     * Verify pack art, game name, price after discount, strike through pricing with
     * Origin Access discount applied message, and the total price is displayed.
     *
     * @param vaultEntitlementName Name of vault entitlement
     * @return true if the above are correct, false otherwise
     */
    public boolean checkJoinAndSaveReview(String vaultEntitlementName) {
        boolean packArtDisplayed = verifyPackArtExists();
        boolean gameNameDisplayed = verifyEntitlementExistsForReviewingOrder(vaultEntitlementName);
        boolean priceAfterDiscountDisplayed = verifyPriceAfterDiscountDisplayed();
        boolean strikeThroughPricingDisplayed = verifyOriginalPriceIsStrikedThrough();
        boolean originAccessMessageDisplayed = verifyOriginAccessDiscountIsVisible();
        boolean totalPriceDisplayed = verifyTotalPriceDisplayed();
        return packArtDisplayed && gameNameDisplayed && priceAfterDiscountDisplayed && strikeThroughPricingDisplayed
                && originAccessMessageDisplayed && totalPriceDisplayed;
    }

    /**
     * Get 'BioWare Points Balance' on the 'Review Order' page.
     * @return true if element is visible, false otherwise
     */
    public String getBioWarePointsBalance() {
        return waitForElementVisible(BIOWARE_POINTS_BALANCE_LOCATOR).getText();
    }

    /**
     * Verifies the selected subscription plan is correctly displayed
     *
     * @param subscriptionType the subscription plan type (premier / basic)
     * @param subscriptionRecurrence the subscription recurrence option (annual
     * / monthly)
     * @return true if the subscription plan and recurrence are a match, false
     * otherwise
     */
    public boolean verifySubscriptionPlan(String subscriptionType, String subscriptionRecurrence) {
        return StringHelper.containsIgnoreCase(waitForElementVisible(ENTITLEMENT_LIST_LOCATOR).getText(), subscriptionType, subscriptionRecurrence);
    }

    /**
     * Verifies the refund awarded when purchasing 'Premier' is correct
     *
     * @param previusPlanPrice the price of the previous subscription plan to
     * check
     * @return true if the previous plan price is a match, false otherwise
     */
    public boolean verifySubscriptionRefundPriceIsCorrect(String previusPlanPrice) {
        double price = StringHelper.extractNumberFromText(waitForElementVisible(ENTITLEMENT_REFUND_PRICE_LIST_LOCATOR).getText());
        return previusPlanPrice.equals(StringHelper.formatDoubleToPriceAsString(price));
    }

    /**
     * Applies 100% off code while purchasing on prod on review order page
     *
     * @param entitlementName Name of the entitlement being purchased
     * @param userEmail Email of the user making the purchase
     * @throws IOException Exception thrown by unsuccessful Crs API call
     */
    public void applyPromoCodeForProdPurchase(String entitlementName, String userEmail) throws IOException {
        String code = CrsHelper.getProdOffCode(userEmail, entitlementName, "paypal");
        waitForElementVisible(ENTER_PROMO_CODE_BUTTON_LOCATOR).click();
        waitForElementVisible(PROMO_CODE_INPUT_LOCATOR).sendKeys(code);
        waitForElementVisible(APPLY_PROMO_CODE_BUTTON_LOCATOR).click();
    }
}