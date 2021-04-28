package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Cancellation Confirmed' dialog
 *
 * @author cdeaconu
 */
public class CancellationConfirmed extends Dialog{
    
    private static final String CANCELLATION_CONFIRMED_DIALOG_CSS = "origin-dialog-content-access-canceled";
    private static final By CANCELLATION_CONFIRMED_DIALOG_LOCATOR = By.cssSelector(CANCELLATION_CONFIRMED_DIALOG_CSS);
    private static final By CLOSE_LOCATOR = By.cssSelector(".otkmodal-close");
    private static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(CANCELLATION_CONFIRMED_DIALOG_CSS + " .otkbtn-primary");
    private static final By REJOINING_IS_EASY_LINK_LOCATOR = By.cssSelector(CANCELLATION_CONFIRMED_DIALOG_CSS +" .otkmodal-body a");
    private static final String[] CANCELLATION_CONFIRMED_KEYWORDS = {"cancelled", "membership"};

    // Body
    private final By SUB_HEADER_LOCATOR = By.cssSelector(".otkmodal-body > p > strong");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public CancellationConfirmed(WebDriver driver) {
        super(driver, CANCELLATION_CONFIRMED_DIALOG_LOCATOR, CLOSE_LOCATOR);
    }
    
    /**
     * Verify the 'Cancellation Confirmed' dialog exists.
     *
     * @return true if the dialog exists and visible, false otherwise
     */
    public boolean isDialogVisible() {
        return waitIsElementVisible(CANCELLATION_CONFIRMED_DIALOG_LOCATOR);
    }

    /**
     * Clicks the 'Close' CTA
     */
    public void clickCloseCTA() {
        waitForElementClickable(waitForElementVisible(CLOSE_BUTTON_LOCATOR)).click();
    }

    /**
     * Verifies there is a confirmation message that the subscription has been
     * canceled
     *
     * @return true if the keywords are contained within the text, false
     * otherwise
     */
    public boolean verifyConfirmationMessage() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(SUB_HEADER_LOCATOR).getText(), CANCELLATION_CONFIRMED_KEYWORDS);
    }
    
    /**
     * Clicks the 'Rejoining is easy' link
     */
    public void clickRejoiningIsEasyLink() {
        waitForElementClickable(waitForElementVisible(REJOINING_IS_EASY_LINK_LOCATOR)).click();
    }
}
