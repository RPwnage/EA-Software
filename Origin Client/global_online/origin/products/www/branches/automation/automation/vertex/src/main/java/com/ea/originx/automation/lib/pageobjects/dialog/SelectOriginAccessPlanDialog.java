package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.originx.automation.lib.resources.OriginClientData;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Dialog that appears to select an Origin Access plan.
 *
 * @author nvarthakavi
 */
public class SelectOriginAccessPlanDialog extends Dialog {

    private static final String DIALOG_CSS = "origin-dialog-content-access-change-membership";
    private static final By DIALOG_LOCATOR = By.cssSelector(DIALOG_CSS);
    private static final By STEP_LOCATOR = By.cssSelector(".origin-store-access-interstitial-header h5");
    private static final By TITLE_LOCATOR = By.cssSelector(".otkmodal-header h4");
    private static final By DESCRIPTION_LOCATOR = By.cssSelector(".origin-store-access-interstitial-desc");
    private static final By NEXT_BUTTON_LOCATOR = By.cssSelector(DIALOG_CSS + " button.otkbtn.otkbtn-primary");
    private static final By FREE_TRIAL_DESCRIPTION_LOCATOR = By.cssSelector(".origin-store-access-interstitial-desc");

    private static final String PLAN_LOCATOR = DIALOG_CSS + " li[data-plan='%s']";
    private static final String SELECTED_PLAN_LOCATOR = "origin-dialog-content-access-change-membership li.origin-oa-change-plan.selected";
    private static final String PRICE_LOCATOR = ".origin-store-access-interstitial-offer-content .origin-store-access-interstitial-offer-price";
    private static final String DURATION_LOCATOR = ".origin-store-access-interstitial-offer-content .origin-store-access-interstitial-offer-duration";
    private static final String BEST_VALUE_TEXT_LOCATOR = ".origin-oa-sub-plan-value-wrap div.origin-oa-best-plan-value";
    private static final String[] EXPECTED_CANCEL_ANYTIME_DESCRIPTION_KEYWORDS = {"cancel", "anytime"};
    private static final String[] EXPECTED_TRIAL_DURATION_DESCRIPTION_KEYWORDS = {"free", Integer.toString(OriginClientData.ORIGIN_ACCESS_TRIAL_LENGTH)};
    private static final String[] EXPECTED_STEP_KEYWORDS = { "step", "1", "member", "Origin", "Access" };
    private static final String[] EXPECTED_TITLE_KEYWORDS = { "Origin", "Access", "plan", "select" };
    private static final String[] EXPECTED_DESCRIPTION_KEYWORDS = { "Origin", "Access", "subscriber", "discount", "purchase" };
    private static final String[] EXPECTED_BEST_VALUE_KEYWORDS = { "Best", "Value" };

    public enum ORIGIN_ACCESS_PLAN {
        MONTHLY_PLAN,
        YEARLY_PLAN
    }

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public SelectOriginAccessPlanDialog(WebDriver driver) {
        super(driver, DIALOG_LOCATOR, TITLE_LOCATOR);
    }

    /**
     * Click the 'Next' button to start the payment page.
     */
    public void clickNext() {
        waitForElementClickable(waitForElementVisible(NEXT_BUTTON_LOCATOR)).click();
    }
    
    /**
     * Wait for the animation to complete (The plan name and the next button takes a while to load).
     */
    public void waitForYearlyPlanAndStartTrialButtonToLoad() {
        waitForAnimationComplete(By.cssSelector(String.format(PLAN_LOCATOR, OriginClientData.ORIGIN_ACCESS_PREMIER_YEARLY_PLAN)), 10);
        waitForAnimationComplete(NEXT_BUTTON_LOCATOR);
    }

    /**
     * Checks to see if the 'Download' dialog is visible.
     *
     * @return true if the download dialog is visible, false otherwise
     */
    public boolean isDialogVisible() {
        return waitIsElementVisible(TITLE_LOCATOR);
    }

    /**
     * Verify the free trial description matches the given expected keywords.
     *
     * @param expectedKeywords keywords to search
     * @return true if the text contains expected keywords, false otherwise
     */
    public boolean verifyFreeTrialDescriptionMatches(String[] expectedKeywords) {
        return StringHelper.containsIgnoreCase(waitForElementVisible(FREE_TRIAL_DESCRIPTION_LOCATOR).getText(), expectedKeywords);
    }

    /**
     * Verify the free trial description indicates it can be cancelled anytime.
     *
     * @return true if the text indicates it can be cancelled anytime, false otherwise
     */
    public boolean verifyCancelAnytimeDescription() {
        return verifyFreeTrialDescriptionMatches(EXPECTED_CANCEL_ANYTIME_DESCRIPTION_KEYWORDS);
    }

    /**
     * Verify the free trial description indicates it provides Origin Access benefits to the user.
     *
     * @return true if the text indicates that it provides Origin Access benefits to the user,
     * false otherwise
     */
    public boolean verifyFreeTrialDurationDescription() {
        return verifyFreeTrialDescriptionMatches(EXPECTED_TRIAL_DURATION_DESCRIPTION_KEYWORDS);
    }

    /**
     * Get the selected Origin Access plan.
     *
     * @return The selected plan as a String
     */
    public String getSelectedPlan() {
        WebElement parentLocator = waitForElementVisible(By.cssSelector(SELECTED_PLAN_LOCATOR));
        return parentLocator.findElement(By.cssSelector(PRICE_LOCATOR)).getText();
    }

    /**
     * Return the selected plan's duration.
     *
     * @return The selected plan's duration as a String
     */
    public String getSelectedPlanDuration() {
        WebElement parentLocator = waitForElementVisible(By.cssSelector(SELECTED_PLAN_LOCATOR));
        return parentLocator.findElement(By.cssSelector(DURATION_LOCATOR)).getText();
    }

    /**
     * Verify 'Best Value' text is visible
     *
     * @return true if best value is visible, false otherwise
     */
    public boolean isBestValueTextVisible(){
        WebElement parentLocator = waitForElementVisible(By.cssSelector(SELECTED_PLAN_LOCATOR));
        String bestValueText = parentLocator.findElement(By.cssSelector(BEST_VALUE_TEXT_LOCATOR)).getText();
        return StringHelper.containsAnyIgnoreCase(bestValueText, EXPECTED_BEST_VALUE_KEYWORDS);
    }

    /**
     * Select the given plan.
     *
     * @param plan The plan to be selected
     */
    public void selectPlan(ORIGIN_ACCESS_PLAN plan) {
        switch (plan) {
            case MONTHLY_PLAN:
                waitForElementClickable(By.cssSelector(String.format(PLAN_LOCATOR, OriginClientData.ORIGIN_ACCESS_PREMIER_MONTHLY_PLAN))).click();
                break;
            case YEARLY_PLAN:
                waitForElementClickable(By.cssSelector(String.format(PLAN_LOCATOR, OriginClientData.ORIGIN_ACCESS_PREMIER_YEARLY_PLAN))).click();
                break;
            default:
                throw new RuntimeException("Error unexpected plan" + plan);
        }
    }
    
    /**
     * Verify the monthly plan is visible.
     *
     * @return true if monthly plan is visible, false otherwise
     */
    public boolean isMonthlyPlanVisible() {
        return waitIsElementVisible(By.cssSelector(String.format(PLAN_LOCATOR, OriginClientData.ORIGIN_ACCESS_PREMIER_MONTHLY_PLAN)));
    }

    /**
     * Verify the monthly plan is selected.
     *
     * @return true if monthly plan class name contains 'selected', false
     * otherwise
     */
    public boolean isMonthlyPlanSelected() {
        WebElement monthlyPlan = waitForElementVisible(By.cssSelector(String.format(PLAN_LOCATOR, OriginClientData.ORIGIN_ACCESS_PREMIER_MONTHLY_PLAN)));
        String monthlyPlanClassName = monthlyPlan.getAttribute("class");
        return monthlyPlanClassName.contains("selected") && isPlanHighlighted(monthlyPlan);
    }
    
    /**
     * Verify the yearly plan is visible.
     *
     * @return true if yearly plan is visible, false otherwise
     */
    public boolean isYearlyPlanVisible() {
        return waitIsElementVisible(By.cssSelector(String.format(PLAN_LOCATOR, OriginClientData.ORIGIN_ACCESS_PREMIER_YEARLY_PLAN)));
    }
    
     /**
     * Verify the yearly plan is selected.
     *
     * @return true if yearly plan class name contains 'selected', false
     * otherwise
     */
    public boolean isYearlyPlanSelected() {
        WebElement yearlyPlan = waitForElementVisible(By.cssSelector(String.format(PLAN_LOCATOR, OriginClientData.ORIGIN_ACCESS_PREMIER_YEARLY_PLAN)));
        String yearlyPlanClassName = yearlyPlan.getAttribute("class");
        return yearlyPlanClassName.contains("selected") && isPlanHighlighted(yearlyPlan);
    }

    /**
     * Verify both the monthly and yearly plans are visible.
     *
     * @return true if monthly and yearly plans are visible, false otherwise
     */
    public boolean verifyMonthlyAndYearlyPlansVisible() {
        return isMonthlyPlanVisible() && isYearlyPlanVisible();
    }

    //////////////////////////////////////////////////////////////////////////
    // Subscribe and Save 'Select Origin Access Plan' dialog
    // The subscribe and save 'Select Origin Access Plan' dialog contains
    // a step header, which the normal dialog doesn't have.
    //////////////////////////////////////////////////////////////////////////
    /**
     * Verify that a step is displayed in the header.
     *
     * @return true if step is displayed, false otherwise
     */
    private boolean isStepTextVisible() {
        return waitIsElementVisible(STEP_LOCATOR);
    }

    /**
     * Verify that the title text is displayed in the header.
     *
     * @return true if title text is displayed, false otherwise
     */
    private boolean isTitleVisible() {
        return waitIsElementVisible(TITLE_LOCATOR);
    }

    /**
     * Verify that the title text and step text is displayed in the header.
     *
     * @return true if both are displayed, false otherwise
     */
    public boolean areHeaderElementsVisible() {
        return isTitleVisible() && isStepTextVisible();
    }

    /**
     * Verify that the step text is correct.
     *
     * @return true if step text is as expected, false otherwise
     */
    private boolean isStepTextCorrect() {
        return StringHelper.containsAnyIgnoreCase(waitForElementVisible(STEP_LOCATOR).getText(), EXPECTED_STEP_KEYWORDS);
    }

    /**
     * Verify that the header text is correct.
     *
     * @return true if header text is as expected, false otherwise
     */
    private boolean isHeaderTextCorrect() {
        return StringHelper.containsAnyIgnoreCase(waitForElementVisible(TITLE_LOCATOR).getText(), EXPECTED_TITLE_KEYWORDS);
    }

    /**
     * Verify that the header element text are correct.
     *
     * @return true if header element text are correct, false otherwise
     */
    public boolean isHeaderElementTextCorrect() {
        return isStepTextCorrect() && isHeaderTextCorrect();
    }

    /**
     * Verify description is displayed.
     *
     * @return true if description is visible, false otherwise
     */
    public boolean isDescriptionVisible() {
        return waitIsElementVisible(DESCRIPTION_LOCATOR);
    }

    /**
     * Get description text.
     *
     * @return The description text
     */
    public String getDescription() {
        return waitForElementVisible(DESCRIPTION_LOCATOR).getText();
    }

    /**
     * Verify description is correct and contains the expected entitlement name.
     *
     * @param entitlementName name of entitlement
     * @return true if description is as expected and contains the entitlement name, false otherwise
     */
    public boolean isDescriptionCorrect(String entitlementName) {
        return StringHelper.containsIgnoreCase(getDescription(), entitlementName) &&
                StringHelper.containsAnyIgnoreCase(getDescription(), EXPECTED_DESCRIPTION_KEYWORDS);
    }

    /**
     * Verify the 'Next' button is visible.
     *
     * @return true if 'next' button is visible, false otherwise
     */
    public boolean isNextButtonVisible() {
        return waitIsElementVisible(NEXT_BUTTON_LOCATOR);
    }
    
    /**
     * Verify the selected plan is highlighted with green
     * 
     * @param plan to check for
     * @return true if the plan is highlighted, false otherwise
     */
    public boolean isPlanHighlighted(WebElement plan) {
        return getColorOfElement(plan, "border-top-color").equalsIgnoreCase(OriginClientData.ORIGIN_ACCESS_SELECTED_PLAN_BORDER_COLOR);
    }
}