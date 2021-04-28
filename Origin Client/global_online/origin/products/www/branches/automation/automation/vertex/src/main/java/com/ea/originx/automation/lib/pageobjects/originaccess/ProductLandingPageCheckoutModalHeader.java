package com.ea.originx.automation.lib.pageobjects.originaccess;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import java.util.Arrays;
import java.util.List;

/**
 * Represents the checkout modal header for the 'Product Landing' page pre-order checkout.
 *
 * @author caleung
 */
public class ProductLandingPageCheckoutModalHeader extends EAXVxSiteTemplate {

    protected static final String HEADER_TEMPLATE = ".checkout-content .otkmodal-header";
    protected static final By HEADER_LOCATOR = By.cssSelector(HEADER_TEMPLATE);
    protected static final String STEP_TEMPLATE = HEADER_TEMPLATE + " .checkout-steps li";
    protected static final By STEP_LOCATOR = By.cssSelector(STEP_TEMPLATE);
    protected static final String STEP_NUMBER_TEMPLATE = STEP_TEMPLATE + " .step-number";
    protected static final By CHECKOUT_MODAL_STEP_SUBHEADER = By.cssSelector(HEADER_TEMPLATE + " .checkout-steps li .step-subheader");
    protected static final String CHECKOUT_MODAL_ACTIVE_STEP_TEMPLATE = HEADER_TEMPLATE + " .checkout-steps .active-step";
    protected static final By CHECKOUT_MODAL_ACTIVE_STEP_LOCATOR = By.cssSelector(CHECKOUT_MODAL_ACTIVE_STEP_TEMPLATE + " .step-header");
    protected static final By CHECKOUT_MODAL_ACTIVE_STEP_NUM_LOCATOR = By.cssSelector(CHECKOUT_MODAL_ACTIVE_STEP_TEMPLATE + " .step-number");

    // Step 1
    protected static final String[] ENTER_PAYMENT_STEP_EXPECTED_KEYWORDS = { "enter", "payment" };

    // Step 2
    protected static final String[] JOIN_ACCESS_STEP_EXPECTED_KEYWORDS = { "join", "Origin", "Access" };
    protected static final String[] JOIN_ACCESS_SUBHEADER_BEFORE_TAX_EXPECTED_KEYWORDS = { OriginClientData.ORIGIN_ACCESS_MONTHLY_PLAN_PRICE };
    protected static final String ORIGIN_ACCESS_AFTER_TAX_PRICE = StringHelper.formatDoubleToPriceAsString(
            StringHelper.extractNumberFromText(OriginClientData.ORIGIN_ACCESS_MONTHLY_PLAN_PRICE) + (StringHelper.extractNumberFromText(OriginClientData.ORIGIN_ACCESS_MONTHLY_PLAN_PRICE) * 0.05));
    protected static final String[] JOIN_ACCESS_SUBHEADER_AFTER_TAX_EXPECTED_KEYWORDS = { ORIGIN_ACCESS_AFTER_TAX_PRICE };
    protected static final String[] JOIN_ACCESS_SUBHEADER_AFTER_PAYMENT_EXPECTED_KEYWORDS = { "membership", "started" };

    // Step 3
    protected static final String[] GET_ENTITLEMENT_STEP_EXPECTED_KEYWORDS = { "get" };
    protected static final String[] GET_ENTITLEMENT_SUBHEADER_EXPECTED_KEYWORDS = { "10%", "Origin", "Access", "discount" };

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ProductLandingPageCheckoutModalHeader(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the checkout modal header is visible.
     *
     * @return true if checkout modal header is visible, false otherwise
     */
    public boolean verifyCheckoutModalHeaderVisible() {
        return waitIsElementVisible(HEADER_LOCATOR);
    }

    /**
     * Get step 1 text ('Enter your payment info').
     *
     * @return Step 1 text
     */
    private String getStepOneText() {
        return driver.findElements(STEP_LOCATOR).get(0).getText();
    }

    /**
     * Get Step two text ('Join Origin Access').
     *
     * @return Step two text
     */
    private String getStepTwoText() {
        return driver.findElements(STEP_LOCATOR).get(1).getText();
    }

    /**
     * Get step two sub-header text (Price of Access before tax, price
     * of Access after tax, 'Your membership has started').
     *
     * @return Step two sub-header text
     */
    private String getStepTwoSubHeaderText() {
        return driver.findElements(CHECKOUT_MODAL_STEP_SUBHEADER).get(0).getText();
    }

    /**
     * Get step three text ('Get (entitlement name)').
     *
     * @return Step three text
     */
    private String getStepThreeText() {
        return driver.findElements(STEP_LOCATOR).get(2).getText();
    }

    /**
     * Get step three sub-header text ('with 10% Origin Access discount').
     *
     * @return Step three sub-header text
     */
    private String getStepThreeSubHeaderText() {
        return driver.findElements(CHECKOUT_MODAL_STEP_SUBHEADER).get(1).getText();
    }

    /**
     * Verify the three step headers text are correct and in the correct order.
     *
     * @param entitlementName Name of pre-order game
     * @return true if all three are correct, false otherwise
     */
    public boolean verifyStepsTextCorrect(String entitlementName) {
        List<String[]> expectedText = Arrays.asList(ENTER_PAYMENT_STEP_EXPECTED_KEYWORDS, JOIN_ACCESS_STEP_EXPECTED_KEYWORDS, GET_ENTITLEMENT_STEP_EXPECTED_KEYWORDS);
        List<String> headerText = Arrays.asList(getStepOneText(), getStepTwoText(), getStepThreeText());
        return headerText.stream()
                .allMatch(b -> StringHelper.containsAnyIgnoreCase(b, expectedText.get(headerText.indexOf(b))))
                && StringHelper.containsIgnoreCase(getStepThreeText(), entitlementName);
    }

    /**
     * Verify step two subheader text is correct after purchasing
     * Origin Access (step 2).
     *
     * @return true if is correct, false otherwise
     */
    public boolean verifyStepTwoSubheaderAfterOAPurchaseCorrect() {
        return StringHelper.containsAnyIgnoreCase(getStepTwoSubHeaderText(), JOIN_ACCESS_SUBHEADER_AFTER_PAYMENT_EXPECTED_KEYWORDS);
    }

    /**
     * Verify sub-header text are shown, are correct, and are in the correct order.
     *
     * @return true if sub-header text are shown, are correct, and are in the correct order,
     * false otherwise
     */
    public boolean verifyStepsSubTextCorrect(boolean afterTax) {
        List<String[]> expectedText;
        if (afterTax) {
            expectedText = Arrays.asList(JOIN_ACCESS_SUBHEADER_AFTER_TAX_EXPECTED_KEYWORDS, GET_ENTITLEMENT_SUBHEADER_EXPECTED_KEYWORDS);
        } else {
            expectedText = Arrays.asList(JOIN_ACCESS_SUBHEADER_BEFORE_TAX_EXPECTED_KEYWORDS, GET_ENTITLEMENT_SUBHEADER_EXPECTED_KEYWORDS);
        }
        List<String> subHeaderText = Arrays.asList(getStepTwoSubHeaderText(), getStepThreeSubHeaderText());
        return subHeaderText.stream()
                .allMatch(b -> StringHelper.containsAnyIgnoreCase(b, expectedText.get(subHeaderText.indexOf(b))));
    }

    /**
     * Get active step's header text.
     *
     * @return Active step's header text
     */
    public String getActiveStepHeaderText() {
        return driver.findElement(CHECKOUT_MODAL_ACTIVE_STEP_LOCATOR).getText();
    }

    /**
     * Get active step number.
     *
     * @return Active step number as a String
     */
    public String getActiveStepNumber() {
        return waitForElementVisible(CHECKOUT_MODAL_ACTIVE_STEP_NUM_LOCATOR).getText();
    }

    /**
     * Verify step one is active and is correct.
     *
     * @return true if step one is active and is correct, false otherwise
     */
    public boolean verifyStepOneActiveAndCorrect() {
        return verifyStepOneActive() && verifyStepOneHeaderExpected();
    }

    /**
     * Verify step two is active and is correct.
     *
     * @return true if step two is active and is correct, false otherwise
     */
    public boolean verifyStepTwoActiveAndCorrect() {
        return verifyStepTwoActive() && verifyStepTwoHeaderExpected();
    }

    /**
     * Verify step three is active and is correct.
     *
     * @param entitlementName Name of entitlement
     * @return true if step three is active, false otherwise
     */
    public boolean verifyStepThreeActiveAndCorrect(String entitlementName) {
        return verifyStepThreeActive() && verifyStepThreeHeaderExpected(entitlementName);
    }

    /**
     * Verify step one is active.
     *
     * @return true if step one is active, false otherwise
     */
    public boolean verifyStepOneActive() {
        return getActiveStepNumber().equals("1");
    }

    /**
     * Verify step two is active.
     *
     * @return true if step two is active, false otherwise
     */
    public boolean verifyStepTwoActive() {
        return getActiveStepNumber().equals("2");
    }

    /**
     * Verify step three is active.
     *
     * @return true if step three is active, false otherwise
     */
    public boolean verifyStepThreeActive() {
        return getActiveStepNumber().equals("3");
    }

    /**
     * Verify step one header contains expected keywords.
     *
     * @return true if step one header contains expected keywords, false otherwise
     */
    public boolean verifyStepOneHeaderExpected() {
        return StringHelper.containsAnyIgnoreCase(getStepOneText(), ENTER_PAYMENT_STEP_EXPECTED_KEYWORDS);
    }

    /**
     * Verify step two header contains expected keywords.
     *
     * @return true if step one header contains expected keywords, false otherwise
     */
    public boolean verifyStepTwoHeaderExpected() {
        return StringHelper.containsAnyIgnoreCase(getStepTwoText(), JOIN_ACCESS_STEP_EXPECTED_KEYWORDS);
    }

    /**
     * Verify step three header contains expected keywords and expected entitlement name.
     *
     * @return true if step one header contains expected keywords, false otherwise
     */
    public boolean verifyStepThreeHeaderExpected(String entitlementName) {
        return StringHelper.containsAnyIgnoreCase(getStepThreeText(), GET_ENTITLEMENT_STEP_EXPECTED_KEYWORDS) &&
                StringHelper.containsIgnoreCase(getActiveStepHeaderText(), entitlementName);
    }

    /**
     * Verify a step is completed (check symbol).
     *
     * @param stepNum Step number to check (only testing for steps 1 and 2 because cannot check for step 3 as dialog
     *                closes after step 3 completed)
     * @return true if given step is completed, false otherwise
     */
    private boolean verifyCompletedStep(int stepNum) {
        return driver.findElements(STEP_LOCATOR).get(stepNum - 1).getAttribute("class").equals("completed-step")
                && waitIsElementVisible(driver.findElements(By.cssSelector(STEP_NUMBER_TEMPLATE)).get(stepNum - 1).
                findElement(By.cssSelector(".otkicon-check")));
    }

    /**
     * Verify step one is completed (check symbol).
     *
     * @return true if step one is completed, false otherwise
     */
    public boolean verifyStepOneCompleted() {
        return verifyCompletedStep(1);
    }

    /**
     * Verify step two is completed (check symbol)
     *
     * @return true if step two is completed, false otherwise
     */
    public boolean verifyStepTwoCompleted() {
        return verifyCompletedStep(2);
    }

    /**
     * Verify step three is completed (check symbol).
     * Not currently used because step three doesn't show the check.
     *
     * @return true if step three is completed, false otherwise
     */
    public boolean verifyStepThreeCompleted() {
        return verifyCompletedStep(3);
    }

    /**
     * Verify a step is disabled.
     *
     * @param stepNum Step number to check
     * @return true if given step is disabled, false otherwise
     */
    private boolean verifyDisabledStep(int stepNum) {
        return driver.findElements(STEP_LOCATOR).get(stepNum - 1).getAttribute("class").equals("");
    }

    /**
     * Verify step two is disabled.
     *
     * @return true if step two is disabled, false otherwise
     */
    public boolean verifyStepTwoDisabled() {
        return verifyDisabledStep(2);
    }

    /**
     * Verify step three is disabled.
     *
     * @return true if step three is disabled, false otherwise
     */
    public boolean verifyStepThreeDisabled() {
        return verifyDisabledStep(3);
    }

    /**
     * Verify all headers are correct during step 3 (Step 1 and 2 are completed, Step 3 is active).
     *
     * @return true if all headers are as expected during step 3, false otherwise
     */
    public boolean verifyHeadersCorrectDuringStepThree() {
        return (verifyStepOneCompleted() && verifyStepTwoCompleted() && verifyStepThreeActive());
    }
}