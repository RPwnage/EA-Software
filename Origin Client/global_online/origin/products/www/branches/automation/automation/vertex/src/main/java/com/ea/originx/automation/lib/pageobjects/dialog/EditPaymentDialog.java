package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.account.OriginAccessSettingsPage;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.originx.automation.lib.pageobjects.account.OriginAccessSettingsPage.ORIGIN_ACCESS_PLAN;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.resources.OriginClientConstants;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;

/**
 * The dialog appears when a user clicks on edit link in My Account: Origin
 * Access - Payment & Billing Section
 *
 * @author nvarthakavi, micwang
 */
public class EditPaymentDialog extends Dialog {

    protected final String IFRAME_LOCATOR_STRING = "lockboxiframe";
    protected final By IFRAME_LOCATOR = By.cssSelector("#" + IFRAME_LOCATOR_STRING);
    protected final By SUBSCRIPTION_INFO_LOCATOR = By.cssSelector("#subscription_info ul li:nth-of-type(2)");
    protected final By SUB_TITLE_LOCATOR = By.cssSelector("#divPaymentType #ContextTitle");
    protected final By DIALOG_TITLE_LOCATOR = By.cssSelector("#Dialog_Edit_Title");
    protected final By CLOSE_LOCATOR = By.cssSelector(".head a");
    protected final By PAYPAL_LOCATOR = By.cssSelector("#PaymentType_PAYPAL_C");
    protected final By CREDIT_CARD_LOCATOR = By.cssSelector("#cardNumber");
    protected final By CREDIT_CARD_TYPE_LOCATOR = By.cssSelector("#CCTypeImg");

    protected final String EXPIRATION_MONTH_DROPDOWN_CSS = "a#expirationMonth-button";
    protected final By EXPIRATION_MONTH_DROPDOWN_LOCATOR = By.cssSelector(EXPIRATION_MONTH_DROPDOWN_CSS);
    protected final String EXPIRATION_YEAR_DROPDOWN_CSS = "a#expirationYear-button";
    protected final By EXPIRATION_YEAR_DROPDOWN_LOCATOR = By.cssSelector(EXPIRATION_YEAR_DROPDOWN_CSS);
    protected final String COUNTRY_DROPDOWN_CSS = "a#AddressCountry-button";
    protected final By COUNTRY_VALUE_LOCATOR = By.cssSelector(COUNTRY_DROPDOWN_CSS + " .ui-selectmenu-status");

    protected final By CVC_LOCATOR = By.cssSelector("#cvc");
    protected final By POSTAL_CODE_LOCATOR = By.cssSelector("#AddressZipCode");
    protected final By FIRST_NAME_LOCATOR = By.cssSelector("#AddressFirstName");
    protected final By LAST_NAME_LOCATOR = By.cssSelector("#AddressLastName");
    protected final By PHONE_NUMBER_LOCATOR = By.cssSelector("#AddressPhoneNumber");
    protected final By SAVE_BUTTON_LOCATOR = By.cssSelector("#SaveButton");
    protected final By CANCEL_BUTTON_LOCATOR = By.cssSelector("#CancelButton");
    protected final By PAYMENT_OPTIONS_LOCATOR = By.cssSelector("ul#PaymentMethodList label input");
    protected final By DESCRIPTION_LOCATOR = By.cssSelector("p#TextPrivacy");
    protected final String ROW_CSS = ".row dl dd label.inputstyle_1 ";
    protected final String DROPDOWN_ARROW_CSS = " span.ui-icon-triangle-1-s";
    protected final String SELECT_OPTION_LOCATOR = "//ul[@id='%s']//li//a[contains(text(),'%s')]";
    protected final String SAVED_PAYMENT_INFORMATION_DROPDOWN_LOCATOR = "a#SavedPaymentList-button";
    protected final String SAVED_PAYMENT_INFORMATION_OPTION_LOCATOR = "SavedPaymentList-menu";
    protected final String EXPIRATION_MONTH_OPTION_LOCATOR = "expirationMonth-menu";
    protected final String EXPIRATION_YEAR_OPTION_LOCATOR = "expirationYear-menu";
    protected final String COUNTRY_OPTION_LOCATOR = "AddressCountry-menu";
    protected final By DROPDOWN_ARROW_LOCATOR = By.cssSelector(SAVED_PAYMENT_INFORMATION_DROPDOWN_LOCATOR + DROPDOWN_ARROW_CSS);
    protected final By DROPDOWN_ADD_NEW_LOCATOR = By.xpath(String.format(SELECT_OPTION_LOCATOR, SAVED_PAYMENT_INFORMATION_OPTION_LOCATOR, "Add New..."));

    protected final List<String> PAYMENT_OPTIONS = Arrays.asList("CREDIT_CARD", "PAYPAL");
    protected final String[] EXPECTED_SUBSCRIPTION_INFO_KEYWORDS = {"billed", "end", "your"};
    protected final String[] EXPECTED_DESCRIPTION_KEYWORDS = {"charge", "renewal", "subscription"};
    protected final String[] PAYPAL_BUTTON_KEYWORDS = {"set", "paypal", "account"};

    public EditPaymentDialog(WebDriver driver) {
        super(driver);
    }

    /**
     * verify the Edit Payment Dialog has reached
     */
    public boolean verifyEditPaymentDialogReached() {
        return waitIsElementVisible(DIALOG_TITLE_LOCATOR);
    }

    /**
     * wait for the Edit Payment Dialog To load and switch to new frame
     */
    public void waitAndSwitchToEditPaymentDialog() {
        waitForPageToLoad();
        // if current frame is iframe, it does not do anything
        if (!checkIfFrameIsActive("iframe")) {
            waitForElementVisible(IFRAME_LOCATOR, 30);
            waitForFrameAndSwitchToIt(IFRAME_LOCATOR_STRING);
        }
    }

    /**
     * Verify the billing date in the subscription info for a given start date
     *
     * @param startDate start date of the subscription
     * @param origin_access_plan the plan the user selected for the subscription
     * @return true if the billing date matches
     * @throws ParseException
     */
    public boolean verifyBillingDateSubInfo(String startDate, ORIGIN_ACCESS_PLAN origin_access_plan) throws ParseException {
        String billingDate = new OriginAccessSettingsPage(driver).calculateBillingDate(startDate, origin_access_plan);
        return StringHelper.containsIgnoreCase(waitForElementVisible(SUBSCRIPTION_INFO_LOCATOR).getText(), billingDate);
    }

    /**
     * Verify the billing date in the subscription info with the start as
     * current date
     *
     * @param origin_access_plan the plan the user selected for the subscription
     * @return true if the billing date matches
     * @throws ParseException
     */
    public boolean verifyBillingDateSubInfo(ORIGIN_ACCESS_PLAN origin_access_plan) throws ParseException {
        SimpleDateFormat dateFormat = new SimpleDateFormat("MM/dd/yyyy");
        String startDate = dateFormat.format(new Date());
        String billingDate = new OriginAccessSettingsPage(driver).calculateBillingDate(startDate, origin_access_plan);
        return StringHelper.containsIgnoreCase(waitForElementVisible(SUBSCRIPTION_INFO_LOCATOR).getText(), billingDate);
    }

    /**
     * Verify the billing date in the subscription info with the start as
     * current date
     */
    public boolean verifySubInfo() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(SUBSCRIPTION_INFO_LOCATOR).getText(), EXPECTED_SUBSCRIPTION_INFO_KEYWORDS);
    }

    /**
     * Verifies that the only payment methods available for Origin Access are credit card and PayPal.
     *
     * @return true if the only payment method options are PayPal and credit card.
     *         false if one of those options is not present, or there is another option.
     */
    public boolean verifyPaymentMethods() {
        List<WebElement> paymentOptions = driver.findElements(PAYMENT_OPTIONS_LOCATOR);
        List<String> paymentStrings = new ArrayList<>();
        paymentOptions.stream().forEach((option) -> paymentStrings.add(option.getAttribute("value")));
        return paymentStrings.equals(PAYMENT_OPTIONS);
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

    /**
     * click the Saved Payment Information drop down to add new payment info
     */
    public void clickAddNewPaymentInfo() {
        waitForElementClickable(DROPDOWN_ARROW_LOCATOR, 30).click(); //click the drop-down
        waitForElementClickable(DROPDOWN_ADD_NEW_LOCATOR, 30).click(); //click the option in drop-down
    }

    /**
     * Input the credit card details
     *
     * @param creditCardNumber the credit card number to be input
     */
    public void inputCreditCard(String creditCardNumber) {
        WebElement creditCard = waitForElementClickable(CREDIT_CARD_LOCATOR);
        creditCard.click();
        creditCard.clear();
        for (int i = 0; i < creditCardNumber.length(); i += 4) {
            int endIndex = Math.min(i + 4, creditCardNumber.length());
            CharSequence cardNumber = creditCardNumber.substring(i, endIndex);
            creditCard.sendKeys(cardNumber);
        }
    }

    /**
     * Click the drop down of Expiration Month and select the month
     *
     * @param month the month to be selected
     */
    public void selectExpirationMonth(String month) {
        waitForElementClickable(By.cssSelector(ROW_CSS + EXPIRATION_MONTH_DROPDOWN_CSS + DROPDOWN_ARROW_CSS)).click(); //open the drop-down
        waitForElementClickable(By.xpath(String.format(SELECT_OPTION_LOCATOR, EXPIRATION_MONTH_OPTION_LOCATOR, month))).click(); //click the option in drop-down
    }

    /**
     * Click the drop down of Expiration Year and select the year
     *
     * @param year the year to be selected
     */
    public void selectExpirationYear(String year) {
        waitForElementClickable(By.cssSelector(ROW_CSS + EXPIRATION_YEAR_DROPDOWN_CSS + DROPDOWN_ARROW_CSS)).click(); //open the drop-down
        waitForElementClickable(By.xpath(String.format(SELECT_OPTION_LOCATOR, EXPIRATION_YEAR_OPTION_LOCATOR, year))).click(); //click the option in drop-down
    }

    /**
     * Input the csc of the card details
     *
     * @param csc for the given card details
     */
    public void inputCSV(String csc) {
        WebElement CVC = waitForElementClickable(CVC_LOCATOR);
        CVC.click();
        CVC.sendKeys(csc);
    }

    /**
     * Click the drop down of Country and select the country
     *
     * @param country the year to be selected
     */
    public void selectCountry(String country) {
        waitForElementClickable(By.cssSelector(ROW_CSS + COUNTRY_DROPDOWN_CSS + DROPDOWN_ARROW_CSS)).click(); //open the drop-down
        waitForElementClickable(By.xpath(String.format(SELECT_OPTION_LOCATOR, COUNTRY_OPTION_LOCATOR, country))).click(); //click the option in drop-down
    }

    /**
     * Input the postal code of the card details
     *
     * @param postalcode for the given card details
     */
    public void inputPostalCode(String postalcode) {
        WebElement postalCode = waitForElementClickable(POSTAL_CODE_LOCATOR);
        postalCode.click();
        postalCode.sendKeys(postalcode);
    }

    /**
     * Input the first name of the card details
     *
     * @param firstname for the given card details
     */
    public void inputFirstName(String firstname) {
        WebElement firstName = waitForElementClickable(FIRST_NAME_LOCATOR);
        firstName.click();
        firstName.sendKeys(firstname);
    }

    /**
     * Input the last name of the card details
     *
     * @param lastname for the given card details
     */
    public void inputLastName(String lastname) {
        WebElement lastName = waitForElementClickable(LAST_NAME_LOCATOR);
        lastName.click();
        lastName.sendKeys(lastname);
    }

    /**
     * Click save to save the payment details
     */
    public void clickSave() {
        waitForElementClickable(SAVE_BUTTON_LOCATOR, 30).click();
        driver.switchTo().defaultContent();
    }

    /**
     * Click cancel to cancel updating the payment details
     */
    public void clickCancel() {
        waitForElementClickable(CANCEL_BUTTON_LOCATOR, 30).click();
        driver.switchTo().defaultContent();
    }

    /**
     * Enter the credit card details for the given details
     *
     * @param creditCard The credit card number
     */
    public void enterCreditCardDetailsDefaultValues(String creditCard) {
        clickAddNewPaymentInfo();
        inputCreditCard(creditCard);
        selectExpirationMonth(OriginClientData.CREDIT_CARD_EXPIRATION_MONTH);
        selectExpirationYear(OriginClientData.CREDIT_CARD_EXPIRATION_YEAR);
        inputCSV(OriginClientData.CSV_CODE);
        selectCountry(OriginClientConstants.COUNTRY_CANADA);
        inputPostalCode(OriginClientData.POSTAL_CODE);
        inputFirstName(OriginClientConstants.FIRST_NAME);
        inputLastName(OriginClientConstants.LAST_NAME);
        clickSave();
    }
    
    /**
     * Clicks the 'PayPal' radio button
     */
    public void clickPayPalRadioButton() {
        waitForElementClickable(PAYPAL_LOCATOR, 30).click();
    }
    
    /**
     * Verify the 'Set Up Your PayPal Account' button is visible
     *
     * @return true if the button is visible and contains keywords, false
     * otherwise
     */
    public boolean verifySetUpPayPalAccountButtonVisible() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(SAVE_BUTTON_LOCATOR, 30).getText(), PAYPAL_BUTTON_KEYWORDS);
    }

    /**
     * Verify if credit card number field is empty
     *
     * @return true if empty, false otherwise.
     */
    public boolean verifyCreditCardNumberEmpty() {
        return waitForElementVisible(CREDIT_CARD_LOCATOR).getText().equals("");
    }

    /**
     * Verify if an invalid credit number is inputted, it shows an orange indicator
     *
     * @return true if orange indicator shows, false otherwise
     */
    public boolean verifyCreditCardNumberShowsInvalid() {
        waitForElementClickable(CVC_LOCATOR).click();
        return waitForElementVisible(CREDIT_CARD_LOCATOR).getCssValue("background-position").contains("0px -128px");
    }

    /**
     * Verify if credit card type is visible and displayed
     *
     * @return true if credit card type is visible, false otherwise.
     */
    public boolean verifyCreditCardTypeVisible() {
        return isElementVisible(CREDIT_CARD_TYPE_LOCATOR);
    }

    /**
     * Verify if the all payment address fields are shown
     *
     * @return true if the fields are shown, false if one of the fields is not shown.
     */
    public boolean verifyAddressFieldsShown() {
        if(!isElementVisible(CREDIT_CARD_LOCATOR))
            return false;

        if(!isElementVisible(EXPIRATION_MONTH_DROPDOWN_LOCATOR))
            return false;

        if(!isElementVisible(EXPIRATION_YEAR_DROPDOWN_LOCATOR))
            return false;

        if(!isElementVisible(CVC_LOCATOR))
            return false;

        if(!isElementVisible(COUNTRY_VALUE_LOCATOR))
            return false;

        if(!isElementVisible(POSTAL_CODE_LOCATOR))
            return false;

        if(!isElementVisible(FIRST_NAME_LOCATOR))
            return false;

        if(!isElementVisible(LAST_NAME_LOCATOR))
            return false;

        if(!isElementVisible(PHONE_NUMBER_LOCATOR))
            return false;

        return true;
    }
}
