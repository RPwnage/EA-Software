package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Class for handling the Cancellation Survey, which appears after confirming
 * the cancellation of an Origin Access subscription
 *
 * @author glivingstone
 */
public class CancellationSurveyDialog extends Dialog {

    private static final String CANCELLATION_SURVEY_CSS = "origin-survey-wrapper";
    private static final By CANCELLATION_SURVEY_LOCATOR  = By.cssSelector(CANCELLATION_SURVEY_CSS);
    private static final By COMPLETE_CANCELLATION_BUTTON_LOCATOR = By.cssSelector(CANCELLATION_SURVEY_CSS + " .otkmodal-footer .otkbtn-primary");
    
    // Header
    private static final By CANCELLATION_TITLE_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-header .otkmodal-title");
    private final String EXPECTED_TITLE_TEXT = "One last thing before you go";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public CancellationSurveyDialog(WebDriver driver) {
        super(driver, CANCELLATION_SURVEY_LOCATOR, CANCELLATION_TITLE_LOCATOR);
    }
    
    /**
     * Verify the title exists with the correct text.
     *
     * @return true if the title is visible, and the text matches the expected
     * text
     */
    public boolean verifyTitleExists() {
        return StringHelper.containsIgnoreCase(waitForElementPresent(CANCELLATION_TITLE_LOCATOR).getText(), EXPECTED_TITLE_TEXT);
    }
    
    /**
     * Clicks the 'Complete Cancellation' CTA
     */
    public void clickCompleteCancellationCTA() {
        waitForElementClickable(waitForElementVisible(COMPLETE_CANCELLATION_BUTTON_LOCATOR)).click();
    }
}
