package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.DateHelper;
import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Covers are "Are You Sure" dialog when cancelling an Origin Access
 * Subscription
 *
 * @author glivingstone
 */
public class AreYouSureDialog extends Dialog {

    // Header and SubHeader
    private static final By ARE_YOU_SURE_TITLE_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-header .otkmodal-title");

    // Body
    private static final By SUB_HEADER_LOCATOR = By.cssSelector(".otkmodal-body > p > strong");
    private static final By BODY_TEXT_LOCATOR = By.cssSelector(".otkmodal-body > p > span");
    private final String[] SUBSCRIPTION_EFFECTIVE_MESSAGGE_KEYWORDS = {"cancellation", "effective", "current"};
    private final String[] END_OF_CYCLE_MESSAGE_KEYWORDS = {"end", "billing"};
    private final String[] REJOIN_ORIGIN_ACCESS_KEYWORDS = {"rejoin", "save"};

    // Footer, including buttons
    private static final By CONTINUE_BUTTON_LOCATOR = By.cssSelector("button[ng-click*=continue]");
    private static final By CLOSE_BUTTON_LOCATOR = By.cssSelector("button[ng-click*=cancel]");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public AreYouSureDialog(WebDriver driver) {
        super(driver, ARE_YOU_SURE_TITLE_LOCATOR, ARE_YOU_SURE_TITLE_LOCATOR);
    }

    /**
     * Verify the title exists and has the correct text in it
     *
     * @return true if the title is correct, false otherwise
     */
    public boolean verifyTitleExists() {
        return verifyTitleEquals("Are you sure you want to cancel?");
    }

    /**
     * Verify the sub-header exists
     *
     * @return true if the sub-header exists
     */
    public boolean verifySubHeaderExists() {
        return waitIsElementVisible(SUB_HEADER_LOCATOR);
    }

    /**
     * Verify the body exists
     *
     * @return true if the body exists
     */
    public boolean verifyBodyExists() {
        return waitIsElementVisible(BODY_TEXT_LOCATOR);
    }

    /**
     * Verify the end date is correct. It is expected to be a year from today
     * based on the subscription type
     *
     * @return True if the end date is a year from today, false otherwise
     */
    public boolean verifyEndDate() {
        String expectedDate = DateHelper.getDatePlusOneYearGMT();
        return StringHelper.containsIgnoreCase(waitForElementVisible(BODY_TEXT_LOCATOR).getText(), expectedDate);
    }

    /**
     * Verify the end date message contains a few keywords indicating that
     * subscription will end of the next billing date
     *
     * @return true if the keywords are contains within the text, false
     * otherwise
     */
    public boolean verifyEndDateMessage() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(BODY_TEXT_LOCATOR).getText(), END_OF_CYCLE_MESSAGE_KEYWORDS);
    }

    /**
     * Verify there is a message indicating that the user can rejoin at anytime
     *
     * @return true if the message indicates the user may rejoin at any time
     */
    public boolean verifyMayRejoinMessage() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(BODY_TEXT_LOCATOR).getAttribute("innerHTML"), REJOIN_ORIGIN_ACCESS_KEYWORDS);
    }

    /**
     * Verify there is a message indicating that the subscription is active
     * until the end of the paid period
     *
     * @return true if the keywords are contained within the text, false
     * otherwise
     */
    public boolean verifySubcriptionActiveEndPeriodMessage() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(BODY_TEXT_LOCATOR).getAttribute("innerHTML"), SUBSCRIPTION_EFFECTIVE_MESSAGGE_KEYWORDS);
    }

    /**
     * Verify the continue button exists on the dialog
     *
     * @return true if the continue button exists
     */
    public boolean verifyContinueButtonExists() {
        return waitIsElementVisible(CONTINUE_BUTTON_LOCATOR);
    }

    /**
     * Verify the close button exists on the dialog
     *
     * @return true if the close button exists
     */
    public boolean verifyCloseButtonExists() {
        return waitIsElementVisible(CLOSE_BUTTON_LOCATOR);
    }

    /**
     * Click the close button on the dialog
     */
    public void clickCloseButton() {
        waitForElementClickable(CLOSE_BUTTON_LOCATOR).click();
    }

    /**
     * Click the continue button on the dialog
     */
    public void clickContinueButton() {
        waitForElementClickable(CONTINUE_BUTTON_LOCATOR).click();
    }
}
