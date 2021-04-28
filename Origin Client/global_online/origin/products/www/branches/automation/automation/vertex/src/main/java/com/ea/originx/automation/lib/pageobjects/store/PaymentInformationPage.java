package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.helpers.I18NUtil;
import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.account.OriginAccessSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.OriginAccessSettingsPage.ORIGIN_ACCESS_PLAN;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.utils.Waits;
import org.apache.commons.collections.CollectionUtils;
import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.Dimension;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.StaleElementReferenceException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;

/**
 * Page object that represents the 'Payment Information' page.
 *
 * @author mkalaivanan
 */
public class PaymentInformationPage extends EAXVxSiteTemplate {

    protected static final By IFRAME_LOCATOR = By.cssSelector("origin-iframe-modal .origin-iframemodal .origin-iframemodal-content iframe");
    protected static final By IFRAME_TITLE_LOCATOR = By.cssSelector(".checkout-content .otkmodal-header .otkmodal-title");
    protected static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(".otkmodal-dialog.otkmodal-lg .otkicon.otkicon-closecircle");
    protected static final By DESCRIPTION_LOCATOR = By.cssSelector(".checkout-content .otkmodal-body.centered-order-confirmation.oa-confirmation p.otkc");
    protected static final String[] EXPECTED_DESCRIPTION_KEYWORDS = { "payment", "subscription", "purchase"};

    // Header section
    protected static final By HEADER_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-header");

    // Payment Selection Navigation
    protected static final List<String> PAYMENT_OPTIONS = Arrays.asList("CREDIT_CARD", "PAYPAL", "INTERAC");
    protected static final List<String> GIFT_PAYMENT_OPTIONS = Arrays.asList("CREDIT_CARD", "PAYPAL", "WALLET");
    protected static final By PAYMENT_OPTIONS_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-body #paymentMethodNav #payment-info-nav > li");
    protected static final By PAYMENT_OPTION_CREDIT_CARD_LOCATOR = By.cssSelector("#payment-info-nav > li[data-target='CREDIT_CARD']");

    // Credit card Selection Section in case there is one credit card which is already added
    protected static final String ACTIVE_SECTION_LOCATOR = ".tabbed-section.active-section";
    protected static final By SAVED_CREDIT_CARD_RADIO_BUTTON_LOCATOR = By.cssSelector(ACTIVE_SECTION_LOCATOR + " .saved-payment-details.disabled .otkradio.checkbox-align-top");
    protected static final By CREDIT_CARD_EXPIRED_TEXT_LOCATOR = By.cssSelector(ACTIVE_SECTION_LOCATOR + " .error-area .otkc");
    protected static final By CARD_SELECTION_LOCATOR = By.cssSelector(".tabbed-section.active-section .account-table ul.has-top-border #kcpInternational span.otkradio");

    // Credit Card Information Section
    protected static final String CREDIT_CARD_CSS = ".account-table .edit-card";
    protected static final By CREDIT_CARD_LOCATOR = By.cssSelector(CREDIT_CARD_CSS + " .credit-card");
    protected static final By CREDIT_CARD_EXPIRY_MONTH_LOCATOR = By.cssSelector(CREDIT_CARD_CSS + " .month-select .otkselect");
    protected static final By CREDIT_CARD_EXPIRY_YEAR_LOCATOR = By.cssSelector(CREDIT_CARD_CSS + " .year-select .otkselect");
    protected static final By CREDIT_CARD_CSC_LOCATOR = By.cssSelector(CREDIT_CARD_CSS + " .cvc-input .otkinput input");
    protected static final By COUNTRY_LOCATOR = By.cssSelector(CREDIT_CARD_CSS + " .country-select .otkselect");
    protected static final By ZIP_CODE_LOCATOR = By.cssSelector(CREDIT_CARD_CSS + " .row-2  .col-2 .otkinput input");
    protected static final By FIRST_NAME_LOCATOR = By.cssSelector(CREDIT_CARD_CSS + " .row-3 .col-1 .otkinput input");
    protected static final By LAST_NAME_LOCATOR = By.cssSelector(CREDIT_CARD_CSS + " .row-3 .col-2 .otkinput input");
    protected static final By SAVE_CREDIT_CARD_INFO_CHECKBOX_LOCATOR = By.cssSelector(CREDIT_CARD_CSS + " .save-card-info #creditCardCheckbox");

    // Credit Card Input Error Messages
    protected static final By CREDIT_CARD_NUMBER_INPUT_ERROR_LOCATOR = By.cssSelector(CREDIT_CARD_CSS + " .row-1 .col-1 .otkinput-errormsg");
    protected static final By CREDIT_CARD_EXPIRATION_INPUT_ERROR_LOCATOR = By.cssSelector(CREDIT_CARD_CSS + " .inner-col .otkform-group-haserror .otkinput-errormsg");
    protected static final By CREDIT_CARD_CSC_INPUT_ERROR_LOCATOR = By.cssSelector(CREDIT_CARD_CSS + " .cvc-input .otkinput-errormsg");
    protected static final By CREDIT_CARD_ZIP_POSTAL_CODE_INPUT_ERROR_LOCATOR = By.cssSelector(CREDIT_CARD_CSS + " .row-2 .col-2 .otkinput-errormsg");
    protected static final By CREDIT_CARD_FIRST_NAME_INPUT_ERROR_LOCATOR = By.cssSelector(CREDIT_CARD_CSS + " .row-3 .col-1 .otkinput-errormsg");
    protected static final By CREDIT_CARD_LAST_NAME_INPUT_ERROR_LOCATOR = By.cssSelector(CREDIT_CARD_CSS + " .row-3 .col-2 .otkinput-errormsg");

    // Proceed Button
    protected static final By PROCEED_TO_REVIEW_ORDER_BUTTON_LOCATOR = By.cssSelector(".checkout-content.otkmodal-content .otkmodal-footer .otkbtn.otkbtn-primary");
    protected static final By PAYMENT_OPTION_PAYPAL_LOCATOR = By.cssSelector("#payment-info-nav > li[data-target='PAYPAL']");
    protected static final By PAYMENT_OPTION_SOFORT_LOCATOR = By.cssSelector("#payment-info-nav > li[data-target='SOFORT']");
    protected static final By PAYMENT_OPTION_EA_WALLET_LOCATOR = By.cssSelector("#payment-info-nav > li[data-target='WALLET']");
    private static final String OPTION_WITH_TEXT_TMPL = "//option[text() = '%s']";
    protected static final String PROCEED_TO_REVIEW_ORDER_BUTTON_TEXT = "Proceed to Review Order";

    //OA Free Trial
    protected static final By OA_TRIAL_HEADER_LOCATOR = By.cssSelector(".oa-trial-header p.otkc");
    private static String[] EXPECTED_CANCEL_ANYTIME_DESCRIPTION_KEYWORDS = {"cancel", "anytime", "before", "trial", "ends"};
    private static String[] EXPECTED_TRIAL_DURATION_DESCRIPTION_KEYWORDS = {"free", Integer.toString(OriginClientData.ORIGIN_ACCESS_TRIAL_LENGTH)};
    private static String[] EXPECTED_NO_CHARGE_DESCRIPTION_KEYWORDS = {"won't", "charged"};

    private static final Logger _log = LogManager.getLogger(PaymentInformationPage.class);

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public PaymentInformationPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Wait for the page to load.
     */
    public void waitForPaymentInfoPageToLoad() {
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 30);
        waitForPageToLoad();
        // if current frame is iframe, it does not do anything
        if (!checkIfFrameIsActive("iframe")) {
            waitForFrameAndSwitchToIt(waitForElementVisible(IFRAME_LOCATOR, 30));
        }
    }

    /**
     * Wait for the page to load. Retry and throw exception after a certain number of retries.
     *
     * Work around to minimize and maximize for there is an issue with javascript not getting the screen
     */
    public void waitForPaymentInfoPageToLoadRetry() {
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 30);
        waitForPageToLoad();

        // if current frame is iframe, it does not do anything
        if (!checkIfFrameIsActive("iframe")) {
            boolean inIFrame = false;
            int retry = 6;
            // keep retrying, minimize then maximize when exception occurs
            while (!inIFrame) {
                try {
                    waitForFrameAndSwitchToIt(waitForElementVisible(IFRAME_LOCATOR, 10));
                    inIFrame = true;
                } catch (TimeoutException e) {
                    if (retry == 0) {
                        throw new TimeoutException(e.getMessage());
                    }
                    driver.manage().window().setSize(new Dimension(1000,1000));
                    driver.manage().window().maximize();
                    retry--;
                }
            }

        }
    }

    /**
     * Verify the 'Payment Information' page has loaded with the
     * 'Proceed to Review Order' button.
     *
     * @return true if button exists, false otherwise
     */
    public boolean verifyPaymentInformationReached() {
        return waitIsElementVisible(PROCEED_TO_REVIEW_ORDER_BUTTON_LOCATOR);
    }

    /**
     * Enter credit card details.
     */
    public void enterCreditCardDetails() {
        String displayCountry = I18NUtil.getLocale().getDisplayCountry();

        inputCreditCardNumber(OriginClientData.CREDIT_CARD_NUMBER);
        inputCreditCardYearExpiration(OriginClientData.CREDIT_CARD_EXPIRATION_YEAR);
        inputCreditCardExpiryMonth(OriginClientData.CREDIT_CARD_EXPIRATION_MONTH);
        inputCreditCardCSC(OriginClientData.CSV_CODE);
        if (StringHelper.containsIgnoreCase(displayCountry, "United States")) {
            inputCountry(OriginClientData.CHECKOUT_COUNTRY_USA); // checkout is 'United States' instead of 'United States of America'
        } else {
            inputCountry(I18NUtil.getMessage("countryLocalName"));
        }
        if (StringHelper.containsIgnoreCase(displayCountry, "Canada") ||
                StringHelper.containsIgnoreCase(displayCountry, "United Kingdom")) {
            inputZipCode(OriginClientData.POSTAL_CODE);
        } else if (StringHelper.containsIgnoreCase(displayCountry, "United States")) {
            inputZipCode(OriginClientData.ZIP_CODE);
        }
        inputFirstName(OriginClientConstants.FIRST_NAME);
        inputLastName(OriginClientConstants.LAST_NAME);
        inputFirstName(OriginClientConstants.FIRST_NAME);
        inputLastName(OriginClientConstants.LAST_NAME);
    }

    /**
     * Switch to Payment by Paypal and Continue to Review order page
     */
    public void switchToPaymentByPaypalAndSkipToReviewOrderPage() {
        selectPayPal();
        clickContinueToPayPal();
    }

    /**
     * Enter credit card details
     *
     * @param creditCardNumber - Number of the credit card
     * @param creditCardExpirationMonth - Month on which credit card expires
     * @param creditCardExpirationYear - Year on which credit card expires
     * @param csvCode - CSV code of the credit card
     * @param country - Country of the credit card
     * @param postalCode - Postal Code of the credit card
     * @param firstName - credit card holder's first name
     * @param lastName - credit card holder's last name
     *
     */
    public void enterCreditCardDetails(String creditCardNumber,
                                       String creditCardExpirationMonth,
                                       String creditCardExpirationYear,
                                       String csvCode,
                                       String country,
                                       String postalCode,
                                       String firstName,
                                       String lastName) {
        inputCreditCardNumber(creditCardNumber);
        inputCreditCardExpiryMonth(creditCardExpirationMonth);
        inputCreditCardYearExpiration(creditCardExpirationYear);
        inputCreditCardCSC(csvCode);
        inputCountry(country);
        inputZipCode(postalCode);
        inputFirstName(firstName);
        inputLastName(lastName);
    }

    /**
     * Input credit card number.
     *
     * @param creditCardNumber Credit card number
     */
    public void inputCreditCardNumber(String creditCardNumber) {
        WebElement creditCard = waitForElementClickable(CREDIT_CARD_LOCATOR);
        creditCard.click();
        creditCard.clear();
        // Sending single send key with complete credit card number keeps failing consistently.
        //Here is the workaround where subset of the complete card number are sent one after another.
        for (int i = 0; i < creditCardNumber.length(); i += 4) {
            int endIndex = Math.min(i + 4, creditCardNumber.length());
            CharSequence cardNumber = creditCardNumber.substring(i, endIndex);
            creditCard.sendKeys(cardNumber);
        }
    }

    /**
     * Input month on which the credit card expires.
     *
     * @param month Month on which credit card expires
     */
    public void inputCreditCardExpiryMonth(String month) {
        WebElement monthCreditCard = waitForElementClickable(CREDIT_CARD_EXPIRY_MONTH_LOCATOR);
        monthCreditCard.click();
        WebElement monthOption = waitForChildElementPresent(monthCreditCard, By.xpath(String.format(OPTION_WITH_TEXT_TMPL, month)));
        monthOption.click();
    }

    /**
     * Input year when the credit card expires.
     *
     * @param year Year when the credit card expires
     */
    public void inputCreditCardYearExpiration(String year) {
        WebElement yearCreditCard = waitForElementClickable(CREDIT_CARD_EXPIRY_YEAR_LOCATOR);
        yearCreditCard.click();
        WebElement yearOption = waitForChildElementPresent(yearCreditCard, By.xpath(String.format(OPTION_WITH_TEXT_TMPL, year)));
        yearOption.click();
    }

    /**
     * Input the credit card 'CSC'.
     *
     * @param csc 'CSC' of the credit card
     */
    public void inputCreditCardCSC(String csc) {
        waitForElementVisible(CREDIT_CARD_CSC_LOCATOR).sendKeys(csc);
    }

    /**
     * Input the country information.
     *
     * @param countryInfo The country information to input
     */
    public void inputCountry(String countryInfo) {
        WebElement country = waitForElementClickable(COUNTRY_LOCATOR);
        country.click();
        WebElement countryOption = waitForChildElementPresent(country, By.xpath(String.format(OPTION_WITH_TEXT_TMPL, countryInfo)));
        countryOption.click();
    }

    /**
     * Input the 'Zip Code'.
     *
     * @param postalCode The 'Postal Code'/'Zip Code' to enter
     */
    public void inputZipCode(String postalCode) {
        waitForElementVisible(ZIP_CODE_LOCATOR).sendKeys(postalCode);
    }

    /**
     * Input the first name of the credit card holder.
     *
     * @param firstName First name of the credit card holder
     */
    public void inputFirstName(String firstName) {
        waitForElementVisible(FIRST_NAME_LOCATOR).sendKeys(firstName);
    }

    /**
     * Input the last name of the credit card holder.
     *
     * @param lastName Last name of the credit card holder
     */
    public void inputLastName(String lastName) {
        waitForElementVisible(LAST_NAME_LOCATOR).sendKeys(lastName);
    }

    /**
     * Click on the 'Proceed to Review Order' button.
     */
    public void clickOnProceedToReviewOrderButton() {
        boolean clickedOnButton = false;
        for (int i = 1; i <= 10; i++) {
            try {
                if (!clickedOnButton) {
                    clickOnElement(waitForElementClickable(PROCEED_TO_REVIEW_ORDER_BUTTON_LOCATOR));
                    if (Waits.pollingWait(() -> new ReviewOrderPage(driver).verifyReviewOrderPageReached())) {
                        clickedOnButton = true;
                    }
                }
            } catch (Exception ex) {
                _log.debug("Clicking on Proceed to review order button failed and trying it for " + i + "time");
            }
        }
    }

    /**
     * Verify if the 'Proceed to Review Order' button is enabled.
     *
     * @return true if 'Proceed to Review Order' button is enabled, false otherwise
     */
    public boolean isProceedToReviewOrderButtonEnabled() {
        try {
            WebElement element = waitForElementVisible(PROCEED_TO_REVIEW_ORDER_BUTTON_LOCATOR);

            //Checking that the otkbtn-disabled class is missing is important
            //because we might get a StaleElementReferenceException if we then
            //try to click on the button but the element gets refreshed due to
            //the otkbtn-disabled class being removed
            return element.isEnabled() && !StringUtils.containsIgnoreCase(element.getAttribute("class"), "otkbtn-disabled");
        } catch (TimeoutException | StaleElementReferenceException e) {
            return false;
        }
    }

    /**
     * Verifies that the only payment methods available for Origin Access are
     * credit card and PayPal.
     *
     * @return true if the only payment method options are PayPal and credit
     * card. Will return false if one of those options is not present, or there
     * is another option.
     */
    public boolean verifyPaymentMethods() {
        List<WebElement> paymentOptions = driver.findElements(PAYMENT_OPTIONS_LOCATOR);
        List<String> paymentStrings = new ArrayList<>();
        for (WebElement option : paymentOptions) {
            paymentStrings.add(option.getAttribute("data-target"));
        }
        return CollectionUtils.isEqualCollection(paymentStrings, PAYMENT_OPTIONS);
    }

    /**
     * Verifies that the only payment methods available when gifting are
     * credit card , Paypal and EA Wallet.
     *
     * @return true if the only payment method options are are
     * credit card , Paypal and EA Wallet.. Will return false if one of those options is not present, or there
     * is another option.
     */
    public boolean verifyGiftPaymentMethods() {
        List<WebElement> paymentOptions = driver.findElements(PAYMENT_OPTIONS_LOCATOR);
        List<String> paymentStrings = new ArrayList<>();
        for (WebElement option : paymentOptions) {
            paymentStrings.add(option.getAttribute("data-target"));
        }
        return CollectionUtils.isEqualCollection(paymentStrings, GIFT_PAYMENT_OPTIONS);
    }


    /**
     * Verifies 'Credit Card' is listed as a payment method.
     *
     * @return true if 'Credit Card' is listed as a payment method, false otherwise
     */
    public boolean verifyPaymentOptionCreditCardVisible(){
        return waitIsElementVisible(PAYMENT_OPTION_CREDIT_CARD_LOCATOR);
    }

    /**
     * Verifies 'PayPal' is listed as a payment method.
     *
     * @return true if 'PayPal' is listed as a payment method, false otherwise
     */
    public boolean verifyPaymentOptionPayPalVisible(){
        return waitIsElementVisible(PAYMENT_OPTION_PAYPAL_LOCATOR);
    }

    /**
     * Verifies 'EA WALLET' is listed as a payment method.
     *
     * @return true if 'EA WALLET' is listed as a payment method, false otherwise
     */
    public boolean verifyPaymentOptionWalletVisible(){
        return waitIsElementVisible(PAYMENT_OPTION_EA_WALLET_LOCATOR);
    }

    /**
     * Verifies 'Sofort' is listed as a payment method
     *
     * @param seconds max time to wait in seconds
     * @return true if payment method is visible, false otherwise
     */
    public boolean verifyPaymentOptionSofortVisible(int seconds){
        return waitIsElementVisible(PAYMENT_OPTION_SOFORT_LOCATOR, seconds);
    }

    /**
     * Click on 'Credit Card' or '신용 카드' for Korea checkout
     */
    public void selectCreditCardForKoreaCheckout() {
        List<WebElement> paymentOptions = driver.findElements(PAYMENT_OPTIONS_LOCATOR);
        for (WebElement option : paymentOptions) {
            if (option.getAttribute("data-target").equals("KCP_CREDIT_CARD")) {
                option.click();
            }
        }
    }

    /**
     * Select 'International Card' or ' ' for Korea checkout
     */
    public void selectInternationalCard() {
        waitForElementVisible(CARD_SELECTION_LOCATOR).click();
    }

    /**
     * Selects 'PayPal' as the payment option.
     */
    public void selectPayPal() {
        waitForElementVisible(PAYMENT_OPTION_PAYPAL_LOCATOR).click();
    }

    /**
     * Verify that the 'PayPal' pay with option is selected.
     *
     * @return true if it is selected, false otherwise
     */
    public boolean verifyPayPalSelected() {
        // If PayPal is selected the li element containing the link
        // will have the class "active"
        return waitForElementVisible(PAYMENT_OPTION_PAYPAL_LOCATOR)
                .getAttribute("class")
                .contains("active");
    }

    /**
     * Clicks the 'Continue to PayPal' button.
     */
    public void clickContinueToPayPal() {
        waitForElementVisible(PROCEED_TO_REVIEW_ORDER_BUTTON_LOCATOR).click();
    }

    /**
     * Selects 'Sofort' as the payment option.
     */
    public void selectSofort() {
        waitForElementVisible(PAYMENT_OPTION_SOFORT_LOCATOR).click();
    }

    /**
     * Verify that the 'Sofort' pay with option is selected.
     *
     * @return true if it is selected, false otherwise
     */
    public boolean verifySofortSelected() {
        // If Sofort is selected the li element containing the link
        // will have the class "active"
        return waitForElementVisible(PAYMENT_OPTION_SOFORT_LOCATOR)
                .findElement(By.xpath(".."))
                .getAttribute("class")
                .contains("active");
    }

    /**
     * Clicks the 'Continue to Sofort' button.
     */
    public void clickContinueToSofort() {
        waitForElementVisible(PROCEED_TO_REVIEW_ORDER_BUTTON_LOCATOR).click();
    }

    /**
     * Clicks the 'Save Credit Card Info' checkbox.
     */
    public void clickSaveCreditCardInfoCheckbox() {
        forceClickElement(waitForElementPresent(SAVE_CREDIT_CARD_INFO_CHECKBOX_LOCATOR));
    }

    /**
     * Clicks the saved credit card element.
     */
    public void clickSavedCreditCardRadioButton() {
        waitForElementVisible(SAVED_CREDIT_CARD_RADIO_BUTTON_LOCATOR).click();
    }

    /**
     * Clicks the 'Close' button for the 'Payment Information' page.
     */
    public void clickCloseButton() {
        driver.switchTo().defaultContent();
        waitForElementVisible(CLOSE_BUTTON_LOCATOR).click();
    }

    /**
     * Verify that the error message contains 'expired'.
     *
     * @return true if it contains 'expired', false otherwise
     */
    public boolean verifyCreditCardExpiredText() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(CREDIT_CARD_EXPIRED_TEXT_LOCATOR).getText(), "expired");
    }

    /**
     * Verify that there is an input error message for entering invalid credit
     * card number.
     *
     * @return true if the message contains "Enter a valid credit card",
     * false otherwise
     */
    public boolean verifyCreditCardNumberInputError() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(CREDIT_CARD_NUMBER_INPUT_ERROR_LOCATOR).getText(), "enter a valid Credit Card");
    }

    /**
     * Verify that there is an input error message for not entering credit card
     * number.
     *
     * @return true if the message contains "Enter your credit card",
     * false otherwise
     */
    public boolean verifyCreditCardNumberBlankError() {
        inputCreditCardNumber("");
        sleep(1000); // wait for error message
        return StringHelper.containsIgnoreCase(waitForElementVisible(CREDIT_CARD_NUMBER_INPUT_ERROR_LOCATOR).getText(), "enter your Credit Card");
    }

    /**
     * Verify that there is am input error message for credit card number field.
     *
     * @return true if the input error message is found for credit card number
     * field, false otherwise
     */
    public boolean verifyCreditCardNumberErrorMessage() {
        sleep(1000);   // wait for error message to be disappeared
        return waitIsElementVisible(CREDIT_CARD_NUMBER_INPUT_ERROR_LOCATOR, 2);
    }

    /**
     * Verify that there is an input error message for entering past month.
     *
     * @return true if the message contains 'expired', false otherwise
     */
    public boolean verifyExpiryMonthInputError() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(CREDIT_CARD_EXPIRATION_INPUT_ERROR_LOCATOR).getText(), "expired");
    }

    /**
     * Verify that there is an input error message for expiry month field.
     *
     * @return true if the input error message is found for expiry month field,
     * false otherwise
     */
    public boolean verifyExpiryMonthErrorMessage() {
        sleep(1000);   // wait for error message to be disappeared
        return waitIsElementVisible(CREDIT_CARD_EXPIRATION_INPUT_ERROR_LOCATOR, 2);
    }

    /**
     * Verify that there is an input error message for not entering CSC code.
     *
     * @return true if the message contains 'CSC', false otherwise
     */
    public boolean verifyCSCCodeBlankError() {
        inputCreditCardCSC("1234");
        waitForElementVisible(CREDIT_CARD_CSC_LOCATOR).clear();
        sleep(1000); // wait for error message
        return StringHelper.containsIgnoreCase(waitForElementVisible(CREDIT_CARD_CSC_INPUT_ERROR_LOCATOR).getText(), "csc");
    }

    /**
     * Verify that there is an input error message for CSC code field.
     *
     * @return true if the input error message is found for CSC code field,
     * false otherwise
     */
    public boolean verifyCSCCodeErrorMessage() {
        sleep(1000);   // wait for error message to be disappeared
        return waitIsElementVisible(CREDIT_CARD_CSC_INPUT_ERROR_LOCATOR, 2);
    }

    /**
     * Verify that there is an input error message for not entering postal/zip
     * code.
     *
     * @return true if the message contains 'zip', false otherwise
     */
    public boolean verifyPostalZipCodeBlankError() {
        inputZipCode("1234");
        waitForElementVisible(ZIP_CODE_LOCATOR).clear();
        sleep(1000); // wait for error message
        return StringHelper.containsIgnoreCase(waitForElementVisible(CREDIT_CARD_ZIP_POSTAL_CODE_INPUT_ERROR_LOCATOR).getText(), "zip");
    }

    /**
     * Verify that there is an input error message for postal/zip code field.
     *
     * @return true if the input error message is found for postal/zip code
     * field, false otherwise
     */
    public boolean verifyPostalZipCodeErrorMessage() {
        sleep(1000);   // wait for error message to be disappeared
        return waitIsElementVisible(CREDIT_CARD_ZIP_POSTAL_CODE_INPUT_ERROR_LOCATOR, 2);
    }

    /**
     * Verify that there is an input error message for not entering first name.
     *
     * @return true if the message contains 'first', false otherwise
     */
    public boolean verifyFirstNameInputBlankError() {
        inputFirstName("aaaa");
        waitForElementVisible(FIRST_NAME_LOCATOR).clear();
        sleep(1000); // wait for error message
        return StringHelper.containsIgnoreCase(waitForElementVisible(CREDIT_CARD_FIRST_NAME_INPUT_ERROR_LOCATOR).getText(), "First");
    }

    /**
     * Verify that there is an input error message for first name field.
     *
     * @return true if the input error message is found for first name field,
     * false otherwise
     */
    public boolean verifyFirstNameErrorMessage() {
        sleep(1000);   // wait for error message to be disappeared
        return waitIsElementVisible(CREDIT_CARD_FIRST_NAME_INPUT_ERROR_LOCATOR, 2);
    }

    /**
     * Verify that there is an input error message for not entering last name.
     *
     * @return true if the message contains 'last', false otherwise
     */
    public boolean verifyLastNameInputBlankError() {
        inputLastName("bbbb");
        waitForElementVisible(LAST_NAME_LOCATOR).clear();
        sleep(1000); // wait for error message
        return StringHelper.containsIgnoreCase(waitForElementVisible(CREDIT_CARD_LAST_NAME_INPUT_ERROR_LOCATOR).getText(), "Last");
    }

    /**
     * Verify that there is an input error message for last name field.
     *
     * @return true if the input error message is found for last name field,
     * false otherwise
     */
    public boolean verifyLastNameErrorMessage() {
        sleep(1000);   // wait for error message to be disappeared
        return waitIsElementVisible(CREDIT_CARD_LAST_NAME_INPUT_ERROR_LOCATOR, 2);
    }

    /**
     * Verify the free trial description.
     *
     * @return true if the text matches, false otherwise
     */
    public boolean verifyFreeTrialHeaderMatches(String[] expectedKeywords) {
        return StringHelper.containsIgnoreCase(waitForElementVisible(OA_TRIAL_HEADER_LOCATOR).getText(), expectedKeywords);
    }

    /**
     * Verify the free trial heading indicates it can be cancelled anytime.
     *
     * @return true if the text matches, false otherwise
     */
    public boolean verifyCancelAnytimeHeader() {
        return verifyFreeTrialHeaderMatches(EXPECTED_CANCEL_ANYTIME_DESCRIPTION_KEYWORDS);
    }

    /**
     * Verify the free trial heading indicates its trial duration.
     *
     * @return true if the text matches, false otherwise
     */
    public boolean verifyTrialDurationHeader() {
        return verifyFreeTrialHeaderMatches(EXPECTED_TRIAL_DURATION_DESCRIPTION_KEYWORDS);
    }

    /**
     * Verify the free trial heading indicates given plan.
     *
     * @param selectedPlan The plan selected by the user
     * @return true if the text matches, false otherwise
     */
    public boolean verifyPlanHeader(String selectedPlan) {
        if (selectedPlan.contains("per")) {
            selectedPlan = selectedPlan.replace(" per ", "/");
            selectedPlan = selectedPlan.replaceAll("\\$", "\\$ "); // currently the text is written with a gap so adding a gap to match with the string
        }
        String[] EXPECTED_PLAN_DESCRIPTION_KEYWORDS = {selectedPlan};
        return verifyFreeTrialHeaderMatches(EXPECTED_PLAN_DESCRIPTION_KEYWORDS);
    }

    /**
     * Verify the free trial heading indicates correct end date for a given
     * start date.
     *
     * @return true if the text matches, false otherwise
     */
    public boolean verifyEndDateHeader() throws ParseException {
        SimpleDateFormat dateFormat = new SimpleDateFormat("MM/dd/yyyy");
        String startDate = dateFormat.format(new Date());
        String billingDate = new OriginAccessSettingsPage(driver).calculateBillingDate(startDate, ORIGIN_ACCESS_PLAN.TRIAL);
        if (billingDate.charAt(0) == '0') {
            billingDate = StringHelper.replaceCharAt(billingDate, 0, '\0');
            billingDate = billingDate.replaceAll("\0", "");
        }
        if (billingDate.charAt(3) == '0') {
            billingDate = StringHelper.replaceCharAt(billingDate, 3, '\0');
            billingDate = billingDate.replaceAll("\0", "");
        }
        String[] EXPECTED_BILLING_DATE_DESCRIPTION = {billingDate};
        return verifyFreeTrialHeaderMatches(EXPECTED_BILLING_DATE_DESCRIPTION);
    }

    /**
     * Verify the free trial heading indicates it won't be charged until the
     * trial ends.
     *
     * @return true if the text matches, false otherwise
     */
    public boolean verifyNoChargeHeader() {
        return verifyFreeTrialHeaderMatches(EXPECTED_NO_CHARGE_DESCRIPTION_KEYWORDS);
    }

    //////////////////////////////////////////////////////////////////////////
    // 'Subscribe and Save' related
    // Special methods because checkout flow is slightly different from
    // regular checkout flow.
    //////////////////////////////////////////////////////////////////////////
    /**
     * Click on the 'Next' button for pre-order entitlement checkout
     * for 'Subscribe and Save'.
     */
    public void clickOnNextButton() {
        waitForElementClickable(PROCEED_TO_REVIEW_ORDER_BUTTON_LOCATOR).click();
    }

    /**
     * Get description text.
     *
     * @return Description text
     */
    public String getDescriptionText() {
        return waitForElementVisible(DESCRIPTION_LOCATOR).getText();
    }

    /**
     * Verify description text is correct.
     *
     * @return true if description text is correct, false otherwise
     */
    public boolean verifyDescriptionCorrect() {
        return StringHelper.containsAnyIgnoreCase(getDescriptionText(), EXPECTED_DESCRIPTION_KEYWORDS);
    }
}