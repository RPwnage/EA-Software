package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Review changes to your membership' dialog
 * 
 * @author mdobre
 */
public class ReviewChangesMembershipDialog extends Dialog{
    
    private static final String REVIEW_CHANGES_MEMBERSHIP_CSS = "origin-dialog-content-access-review-change";
    private static final By REVIEW_CHANGES_MEMBERSHIP_LOCATOR = By.cssSelector(REVIEW_CHANGES_MEMBERSHIP_CSS);
    private static final By REVIEW_CHANGES_CURRENT_MEMBERSHIP_LOCATOR = By.cssSelector(REVIEW_CHANGES_MEMBERSHIP_CSS + " .origin-oa-old-plan");
    private static final By REVIEW_CHANGES_NEW_MEMBERSHIP_LOCATOR = By.cssSelector(REVIEW_CHANGES_MEMBERSHIP_CSS + " .origin-oa-new-plan");
    private static final By REVIEW_CHANGES_CURRENT_MEMBERSHIP_PRICE_LOCATOR = By.cssSelector(".origin-oa-sub-plan-price");
    private static final By CLOSE_LOCATOR = By.cssSelector(".otkmodal-close");
    private static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(REVIEW_CHANGES_MEMBERSHIP_CSS + " .otkbtn-primary");
    private static final By CONFIRM_CHANGE_LOCATOR = By.cssSelector(REVIEW_CHANGES_MEMBERSHIP_CSS + " button");
    private static final String CURRENT_MEMBERSHIP_TEXT = "YOUR CURRENT MEMBERSHIP";
    
    /**
     * Constructor
     * 
     * @param driver Selenium WebDriver
     */
    public ReviewChangesMembershipDialog(WebDriver driver) {
        super(driver, REVIEW_CHANGES_MEMBERSHIP_LOCATOR, CLOSE_LOCATOR);
    }
    
    /**
     * Clicks the 'Close' CTA
     */
    public void clickCloseCTA() {
        waitForElementClickable(waitForElementVisible(CLOSE_BUTTON_LOCATOR)).click();
    }
    
    /** 
     * Clicks the 'Confirm Change' CTA
     */
    public void clickConfirmChangeCTA() {
        waitForElementClickable(waitForElementVisible(CONFIRM_CHANGE_LOCATOR)).click();
    }

    /**
     * Verifies the current plan is monthly and contains expected text
     *
     * @return true if the plan type is monthly and contains expected text,
     * false otherwise
     */
    public boolean verifyCurrentMonthlyMembershipPlan() {
        boolean dataPlan = waitForElementPresent(REVIEW_CHANGES_CURRENT_MEMBERSHIP_LOCATOR).getAttribute("data-plan").equals("monthly");
        boolean dataPlanText = StringHelper.containsIgnoreCase(waitForElementPresent(REVIEW_CHANGES_CURRENT_MEMBERSHIP_LOCATOR).getText(), CURRENT_MEMBERSHIP_TEXT);
        return dataPlan && dataPlanText;
    }

    /**
     * Verifies the current plan is yearly and contains expected text
     *
     * @return true if the plan type is yearly and contains expected text, false
     * otherwise
     */
    public boolean verifyCurrentYearlyMembershipPlan() {
        boolean dataPlan = waitForElementPresent(REVIEW_CHANGES_CURRENT_MEMBERSHIP_LOCATOR).getAttribute("data-plan").equals("yearly");
        boolean dataPlanText = StringHelper.containsIgnoreCase(waitForElementPresent(REVIEW_CHANGES_CURRENT_MEMBERSHIP_LOCATOR).getText(), CURRENT_MEMBERSHIP_TEXT);
        return dataPlan && dataPlanText;
    }

    /**
     * Gets the price of the current membership plan
     *
     * @return the price as String
     */
    public String getCurrentMembershipPlanPrice() {
        double price = StringHelper.extractNumberFromText(waitForElementPresent(REVIEW_CHANGES_CURRENT_MEMBERSHIP_LOCATOR).findElement(REVIEW_CHANGES_CURRENT_MEMBERSHIP_PRICE_LOCATOR).getText());
        return StringHelper.formatDoubleToPriceAsString(price);
    }

    /**
     * Gets the price of the new membership plan
     *
     * @return the price as String
     */
    public String getNewMembershipPlanPrice() {
        double price = StringHelper.extractNumberFromText(waitForElementPresent(REVIEW_CHANGES_NEW_MEMBERSHIP_LOCATOR).findElement(REVIEW_CHANGES_CURRENT_MEMBERSHIP_PRICE_LOCATOR).getText());
        return StringHelper.formatDoubleToPriceAsString(price);
    }

    /**
     * Verifies current billing plan is yearly
     *
     * @return true if the billing plan is yearly, false otherwise
     */
    public boolean verifyCurrentMembershipBillingPlanYearlyText() {
        return StringHelper.containsIgnoreCase(waitForElementPresent(REVIEW_CHANGES_CURRENT_MEMBERSHIP_LOCATOR).findElement(REVIEW_CHANGES_CURRENT_MEMBERSHIP_PRICE_LOCATOR).getText(), "year");
    }

    /**
     * Verifies current billing plan is monthly
     *
     * @return true if the billing plan is monthly, false otherwise
     */
    public boolean verifyCurrentMembershipBillingPlanMonthlyText() {
        return StringHelper.containsIgnoreCase(waitForElementPresent(REVIEW_CHANGES_CURRENT_MEMBERSHIP_LOCATOR).findElement(REVIEW_CHANGES_CURRENT_MEMBERSHIP_PRICE_LOCATOR).getText(), "month");
    }
}