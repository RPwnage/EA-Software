package com.ea.originx.automation.lib.pageobjects.account;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.dialog.CancelMembership;
import com.ea.originx.automation.lib.resources.OriginClientData;
import java.lang.invoke.MethodHandles;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import org.apache.commons.lang3.time.DateUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represent the 'Origin Access' settings page ('Account Settings' page with the
 * 'Origin Access' section open)
 *
 * @author palui, micwang
 */
public class OriginAccessSettingsPage extends AccountSettingsPage {

    // Origin Access Section
    private final By CANCEL_MEMBERSHIP_LOCATOR = By.cssSelector("#CancelSubscription");
    private final By MEMBERSHIP_STATUS_LOCATOR = By.cssSelector("#paymentBillingDesc > dl > dd:nth-of-type(2)");
    private final By MEMBERSHIP_LOCATOR = By.cssSelector("#paymentBillingDesc > dl > #PaymentPlan > span");
    private final By BILLING_DATE_LOCATOR = By.cssSelector("#paymentBillingDesc > dl > #NextBillingDate > span");
    private final By PAYMENT_METHOD_LOCATOR = By.cssSelector("#paymentBillingDesc > dl > #PaymentMethod > img");
    private final By PAYMENT_TEXT_LOCATOR = By.cssSelector("#paymentBillingDesc > dl > #PaymentMethod > .paymentmethod_txt");
    private final By UPDATE_MEMBERSHIP_BANNER_MESSAGE_LOCATOR = By.cssSelector("#Msg_UpdatePlanSuccess");
    private final By CANCEL_MEMBERSHIP_SUCCESS_BANNER_LOCATOR = By.cssSelector("#Msg_CancelAndExpire");
    private final By EDIT_PAYMENT_LOCATOR = By.cssSelector("#EditAtPayment");
    private final By UPDATE_PAYMENT_LOCATOR = By.cssSelector("#UpdatePayment");
    private final By RENEW_MEMBERSHIP_LOCATOR = By.cssSelector("#UndoCancellation");
    private final By SUCCESS_MESSAGE_LOCATOR = By.id("Msg_OperationSuccess");
    private final By CREDIT_CARD_EXPIRED_MESSAGE = By.id("Msg_FreeTrial_CreditExpired");
    private final By UPDATE_PAYMENT_LINK_LOCATOR = By.cssSelector("#Msg_FreeTrial_CreditExpired a");
    private final By PAYPAL_WALLET_INFO_LOCATOR = By.cssSelector("#walletinfo");
    private final String[] EXPIRED_CREDIT_CARD_KEYWORDS = {"credit", "card", "expired"};
    
    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OriginAccessSettingsPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'Origin Access' section of the 'Account Settings' page is open
     *
     * @return true if open, false otherwise
     */
    public boolean verifyOriginAccessSectionReached() {
        return verifySectionReached(NavLinkType.ORIGIN_ACCESS);
    }

    /**
     * The Origin Access Plan
     */
    public enum ORIGIN_ACCESS_PLAN {
        TRIAL, YEARLY, MONTHLY
    }

    /**
     * Click on the "Cancel Membership" link
     */
    public void clickCancelMembership() {
        waitForElementClickable(CANCEL_MEMBERSHIP_LOCATOR).click();
    }

    /**
     * Cancel the currently active Origin Access Subscription. This will allow
     * the remainder of playtime to run out naturally
     */
    public void cancelMembership() {
        clickCancelMembership();
    }
    
    /**
     * Click on the "Update Payment" link
     */
    public void clickUpdatePaymentLink() {
        waitForElementClickable(UPDATE_PAYMENT_LINK_LOCATOR).click();
    }

    /**
     * Check if the "Renew Membership" link is visible on the page
     *
     * @return true if the "Renew Membership" link is visible
     */
    public boolean isRenewMemebershipVisible() {
        return waitIsElementVisible(RENEW_MEMBERSHIP_LOCATOR);
    }

    /**
     * Click the "Renew Membership" link on the Origin Access Page
     */
    public void clickRenewMembership() {
        waitForElementClickable(RENEW_MEMBERSHIP_LOCATOR).click();
    }

    /**
     * Click the Edit Payment Link
     */
    public void clickEditPaymentLink() {
        waitForElementClickable(EDIT_PAYMENT_LOCATOR, 15).click(); // add extra time to wait for Edit link
    }

    /**
     * Click one of the links that modify the payment method
     * For credit card that has not expired, click edit.
     * For credit card that has expired, click update.
     */
    public void clickModifyPaymentLink() {
        WebElement modifyButton;
        if(isElementVisible(UPDATE_PAYMENT_LINK_LOCATOR))
            modifyButton = waitForElementClickable(UPDATE_PAYMENT_LOCATOR);
        else
            modifyButton = waitForElementClickable(EDIT_PAYMENT_LOCATOR);
        modifyButton.click();
    }

    /**
     * Verify the edit link is visible
     *
     * @return true if the link is displayed
     */
    public boolean verifyEditPaymentLinkVisible() {
        return waitIsElementVisible(EDIT_PAYMENT_LOCATOR);
    }

    /**
     * Verify one of the links that modify payment is visible
     *
     * @return true if one of the link is displayed, false otherwise
     */
    public boolean verifyModifyPaymentLinkVisible() {
        return waitIsElementVisible(EDIT_PAYMENT_LOCATOR) || waitIsElementVisible(UPDATE_PAYMENT_LOCATOR);
    }

    /**
     * Verifies 'Cancel Membership' CTA is displayed
     *
     * @return true if the CTA is displayed, false otherwise
     */
    public boolean verifyCancelMembershipCTA() {
        return waitIsElementVisible(CANCEL_MEMBERSHIP_LOCATOR);
    }

    /**
     * Calculate the billing date for the given start date and subscription type
     *
     * @param startDate The date when the membership was bought
     * @param origin_access_plan the selected plan by user
     * @return the calculated billing date
     * @throws ParseException
     */
    public String calculateBillingDate(String startDate, ORIGIN_ACCESS_PLAN origin_access_plan) throws ParseException {
        SimpleDateFormat dateFormat = new SimpleDateFormat("MM/dd/yyyy");
        Date parsedStartDate = dateFormat.parse(startDate);
        Date expectedDate = null;
        switch (origin_access_plan) {
            case TRIAL:
                expectedDate = DateUtils.addDays(parsedStartDate, OriginClientData.ORIGIN_ACCESS_TRIAL_LENGTH);
                break;
            case MONTHLY:
                expectedDate = DateUtils.addMonths(parsedStartDate, 1);
                break;
            case YEARLY:
                expectedDate = DateUtils.addYears(parsedStartDate, 1);
                break;
            default:
                throw new RuntimeException("Error unexpected plan" + origin_access_plan);
        }
        return dateFormat.format(expectedDate);
    }

    /**
     * Verify the membership is active by checking the text of the membership
     * status in the 'Origin Access' section
     *
     * @return true if the membership is active, false otherwise
     */
    public boolean isMembershipActive() {
        return waitForElementPresent(MEMBERSHIP_STATUS_LOCATOR).getText().contains("Basic") || waitForElementPresent(MEMBERSHIP_STATUS_LOCATOR).getText().contains("Premier");
    }

    /**
     * Verify the membership is active by checking the text of the membership
     * status in the 'Origin Access' section
     *
     * @return true if the membership is 'canceled', false otherwise
     */
    public boolean isMembershipCanceled() {
        return waitForElementPresent(MEMBERSHIP_STATUS_LOCATOR).getText().equalsIgnoreCase("Canceled");
    }

    /**
     * Verify the membership is inactive by checking the text of the membership
     * status in the 'Origin Access' section
     *
     * @return true if the membership is 'inactive', false otherwise
     */
    public boolean isMembershipInactive() {
        return waitForElementPresent(MEMBERSHIP_STATUS_LOCATOR).getText().equalsIgnoreCase("Inactive");
    }

    /**
     * Verify the membership type provided
     *
     * @param membershipType : The type of membership of origin access
     * (Yearly/Monthly)
     * @return true if the membership matches
     */
    public boolean verifyMembership(String membershipType) {
        return StringHelper.containsIgnoreCase(waitForElementVisible(MEMBERSHIP_LOCATOR).getText(), membershipType);
    }
    
    /**
     * Verify the end of the subscription is correct
     *
     * @param startDate The date when the membership/free-trial was bought
     * @param origin_access_plan The selected plan by the user
     * @return true if the date matches
     */
    public boolean verifyEndDate(String startDate, ORIGIN_ACCESS_PLAN origin_access_plan) throws ParseException {
        SimpleDateFormat dateFormat = new SimpleDateFormat("MM/dd/yyyy");
        Date billingDate = dateFormat.parse(waitForElementVisible(BILLING_DATE_LOCATOR).getText());
        String expectedDate = calculateBillingDate(startDate, origin_access_plan);
        Date parsedExpectedDate = dateFormat.parse(expectedDate);
        return parsedExpectedDate.equals(billingDate);
    }

    /**
     * Verify the end of the subscription is correct with the start date as
     * current date
     *
     * @param origin_access_plan The selected plan by the user
     * @return true if the date matches
     */
    public boolean verifyEndDate(ORIGIN_ACCESS_PLAN origin_access_plan) throws ParseException {
        SimpleDateFormat dateFormat = new SimpleDateFormat("MM/dd/yyyy");
        String startDate = dateFormat.format(new Date());
        Date billingDate = dateFormat.parse(waitForElementVisible(BILLING_DATE_LOCATOR).getText());
        String expectedDate = calculateBillingDate(startDate, origin_access_plan);
        Date parsedExpectedDate = dateFormat.parse(expectedDate);
        return parsedExpectedDate.equals(billingDate);
    }

    /**
     * Verify the Plan for the origin access membership is correct
     *
     * @param planPrice the price of the plan the user selected
     * @return true if the price matches
     */
    public boolean verifyPlanPrice(double planPrice) {
        double membershipPrice = StringHelper.extractNumberFromText(waitForElementVisible(MEMBERSHIP_LOCATOR).getText());
        return membershipPrice == planPrice;
    }

    /**
     * Verify the payment method
     *
     * @param paymentMethod the method through which the free-trial.origin
     * access was bought
     * @return true if the payment method matches
     */
    public boolean verifyPaymentMethod(String paymentMethod) {
        return StringHelper.containsIgnoreCase(waitForElementVisible(PAYMENT_METHOD_LOCATOR).getAttribute("src"), paymentMethod);
    }

    /**
     * Verify the payment credit card details
     *
     * @param creditCardNumber the method through which the free-trial.origin access was bought
     * @return true if the payment method matches
     */
    public boolean verifyCreditCardNumber(String creditCardNumber) {
        String paymentText = creditCardNumber.substring(12); //extracting only the last 4 digits
        return StringHelper.containsIgnoreCase(waitForElementVisible(PAYMENT_TEXT_LOCATOR).getText(), paymentText);
    }

    /**
     * Verify the message shown indicating payment method updated successfully
     *
     * @return true if success message shows
     */
    public boolean verifyPaymentMethodUpdateSucessMsg() {
        return waitIsElementVisible(SUCCESS_MESSAGE_LOCATOR, 30);
    }
    
    /**
     * Verify the 'PayPal' wallet info section is visible
     *
     * @return true if the section is visible, false otherwise
     */
    public boolean verifyPayPalWalletInfoSectionVisible() {
        return waitIsElementVisible(PAYPAL_WALLET_INFO_LOCATOR, 30);
    }
    
    /**
     * Verify the message shown indicating credit card expired
     *
     * @return true if message is visible and contains keywords, false otherwise
     */
    public boolean verifyCreditCardExpiredMsg() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(CREDIT_CARD_EXPIRED_MESSAGE).getText(), EXPIRED_CREDIT_CARD_KEYWORDS);
    }

    /**
     * Verify the Plan duration(yearly/monthly) or the plan type
     *
     * @param selectedPlan is the duration of the plan selected by user
     * @return true if the duration matches
     */
    public boolean verifyPlanType(String selectedPlan) {
        if (selectedPlan.contains("per")) {
            selectedPlan = selectedPlan.replace("per", "/");
            selectedPlan = selectedPlan.replaceAll(" ", "");
        }
        return StringHelper.containsIgnoreCase(waitForElementVisible(MEMBERSHIP_LOCATOR).getText(), selectedPlan);
    }

    /**
     * Verify the plan in the banner message at the top of the page
     *
     * @param selectedPlan the selected plan during the free-trial was bought
     * @return true if the billing date,duration and the price are correct
     */
    public boolean verifyBannerMessagePlan(String selectedPlan) throws ParseException {
        if (selectedPlan.contains("per")) {
            selectedPlan = selectedPlan.replace("per", "/");
            selectedPlan = selectedPlan.replaceAll(" ", "");
        }
        return StringHelper.containsIgnoreCase(waitForElementVisible(UPDATE_MEMBERSHIP_BANNER_MESSAGE_LOCATOR).getText(), selectedPlan);
    }

    /**
     * Verify the date on the Cancel Confirmation dialog is correct
     *
     * @param origin_access_plan enum of the Origin Access plan type
     * @return true if the listed expiration date is correct based on the type
     * of plan given
     * @throws ParseException
     */
    public boolean verifyCancelDialogDate(ORIGIN_ACCESS_PLAN origin_access_plan) throws ParseException {
        String startDate = new SimpleDateFormat("MM/dd/yyyy").format(new Date());
        return verifyBannerMessageDate(waitForElementVisible(CancelMembership.getCancelMembershipBodyText()).getText(), startDate, origin_access_plan);
    }

    /**
     * Verifies the date in the cancel membership banner message with start date
     * as the current date
     *
     * @param origin_access_plan type of plan selected by user
     * @return true if the banner message has expected billing date
     * @throws ParseException
     */
    public boolean verifyCancelBannerMessageDate(ORIGIN_ACCESS_PLAN origin_access_plan) throws ParseException {
        String startDate = new SimpleDateFormat("MM/dd/yyyy").format(new Date());
        return verifyCancelBannerMessageDate(startDate, origin_access_plan);
    }

    /**
     * Verifies the date in the cancel membership banner message with a given
     * start date
     *
     * @param startDate is when the membership was bought
     * @param origin_access_plan type of plan selected by user
     * @return true if the banner message has expected billing date
     * @throws ParseException
     */
    public boolean verifyCancelBannerMessageDate(String startDate, ORIGIN_ACCESS_PLAN origin_access_plan) throws ParseException {
        return verifyBannerMessageDate(waitForElementVisible(CANCEL_MEMBERSHIP_SUCCESS_BANNER_LOCATOR).getText(), startDate, origin_access_plan);
    }

    /**
     * Verifies the date in the update membership banner message with a given
     * start date
     *
     * @param startDate is when the membership was bought
     * @param origin_access_plan type of plan selected by user
     * @return true if the banner message has expected billing date
     * @throws ParseException
     */
    public boolean verifyUpdatePlanBannerMessageDate(String startDate, ORIGIN_ACCESS_PLAN origin_access_plan) throws ParseException {
        return verifyBannerMessageDate(waitForElementVisible(UPDATE_MEMBERSHIP_BANNER_MESSAGE_LOCATOR).getText(), startDate, origin_access_plan);
    }

    /**
     * Verifies the date in the update membership banner message with start date
     * as the current date
     *
     * @param origin_access_plan type of plan selected by user
     * @return true if the banner message has expected billing date
     * @throws ParseException
     */
    public boolean verifyUpdatePlanBannerMessageDate(ORIGIN_ACCESS_PLAN origin_access_plan) throws ParseException {
        String startDate = new SimpleDateFormat("MM/dd/yyyy").format(new Date());
        return verifyUpdatePlanBannerMessageDate(startDate, origin_access_plan);
    }

    /**
     * Verifies the billing date in banner message
     *
     * @param bannerText The text appeared in the banner message at the top
     * @param startDate The Start Date of the membership was bought
     * @param origin_access_plan The type of plan the user purchased/currently
     * own
     * @return true if the date matches according to the plan mentioned
     * @throws ParseException
     */
    public boolean verifyBannerMessageDate(String bannerText, String startDate, ORIGIN_ACCESS_PLAN origin_access_plan) throws ParseException {
        return StringHelper.containsIgnoreCase(bannerText, calculateBillingDate(startDate, origin_access_plan));
    }
}
