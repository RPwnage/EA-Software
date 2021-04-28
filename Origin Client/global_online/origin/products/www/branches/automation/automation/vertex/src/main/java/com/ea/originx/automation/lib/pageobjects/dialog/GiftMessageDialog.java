package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the dialog which prompts for a gift message when sending a gift.
 *
 * @author jmittertreiner
 */
public class GiftMessageDialog extends Dialog {

    private static final By DIALOG_LOCATOR = By.cssSelector("div.origin-store-gift-custommessage-rightcolumn");
    private static final By TITLE_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-header .otkmodal-title");
    private static final String[] TITLE_KEYWORDS = {"custom", "message"};

    // Information
    private static final String LEFT_COLUMN_CSS = ".origin-store-gift-custommessage-leftcolumn";
    private static final By DIALOG_MESSAGE_HEADER_LOCATOR = By.cssSelector(LEFT_COLUMN_CSS + " > .origin-store-gift-custommessage-messageheader");
    private static final String EXPECTED_DIALOG_MESSAGE_HEADER_TMPL = "sending %s to %s";
    private static final By PACK_ART_LOCATOR = By.cssSelector(".origin-store-gift-custommessage-rightcolumn img[src*='.jpg']");
    private static final By ERROR_MESSAGE_LOCATOR = By.cssSelector(LEFT_COLUMN_CSS + " > .otkinput-errormsg");
    private static final String[] EXPECTED_MISSING_SENDER_NAME_ERROR_MESSAGE_KEYWORDS = {"name","required"};

    // Text Entry Boxes
    private static final By SENDER_NAME_LOCATOR = By.cssSelector("#origin-store-gift-custommessage-yourname");

    private static final By MESSAGE_LOCATOR = By.cssSelector("div.otktextarea > textarea");
    private static final By CHARACTER_REMAINING_LOCATOR = By.cssSelector("p[ng-bind=charsRemaining]");
    public static final int MESSAGE_BOX_LIMIT = 256;

    // Buttons
    private static final By BACK_BUTTON_LOCATOR = By.cssSelector("span.origin-store-gift-custommessage-backbtnlabel");
    private static final By NEXT_BUTTON_LOCATOR = By.cssSelector("div.otkmodal-footer > button");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public GiftMessageDialog(WebDriver driver) {
        super(driver, DIALOG_LOCATOR, TITLE_LOCATOR);
    }

    /**
     * Wait for the dialog to appear, then wait an extra second for the dialog's
     * animation to complete.
     *
     * @return true if the dialog becomes visible
     */
    @Override
    public boolean waitIsVisible() {
        boolean opened = super.waitIsVisible();
        sleep(1000); // Wait for Animations to Complete
        return opened;
    }

    /**
     * Get the dialog message header text String.
     *
     * @return Message header, or throw TimeoutException if not visible
     */
    public String getDialogMessageHeader() {
        return waitForElementVisible(DIALOG_MESSAGE_HEADER_LOCATOR).getText();
    }

    /**
     * Click the dialog message header.
     */
    public void clickDialogMessageHeader() {
        waitForElementVisible(DIALOG_MESSAGE_HEADER_LOCATOR).click();
    }

    /**
     * Verify dialog message header against gift and recipient.
     *
     * @param entitlementName Name of gifting entitlement
     * @param recipientName Name of recipient of gift
     * @return true if message header shows the entitlement is a gift to the
     * recipient, false otherwise
     */
    public boolean verifyDialogMessageHeader(String entitlementName, String recipientName) {
        String expectedMessageHeader = String.format(EXPECTED_DIALOG_MESSAGE_HEADER_TMPL, entitlementName, recipientName);
        return StringHelper.containsIgnoreCase(getDialogMessageHeader(), expectedMessageHeader);
    }

    /**
     * Get error message text if any.
     *
     * @return error message text, or throw TimeoutException if error message if
     * not visible
     */
    public String getErrorMessage() {
        return waitForElementVisible(ERROR_MESSAGE_LOCATOR).getText();
    }

    /**
     * Verify 'Missing Sender Name' error message is visible.
     *
     * @return true if visible, false if there is a different error, or throw
     * TimeoutException if no error message visible
     */
    public boolean verifyMissingSenderNameErrorMessage() {
        return StringHelper.containsIgnoreCase(getErrorMessage(), EXPECTED_MISSING_SENDER_NAME_ERROR_MESSAGE_KEYWORDS);
    }

    /**
     * Verify if error message is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean isErrorMessageVisible() {
        return waitIsElementVisible(ERROR_MESSAGE_LOCATOR, 2);
    }

    /**
     * Clear the 'Sender Name' input field.
     */
    public void clearSenderName() {
        waitForElementVisible(SENDER_NAME_LOCATOR).clear();
    }

    /**
     * Clears then enter a given String at the sender name input field.
     *
     * @param name The name to enter
     */
    public void enterSenderName(String name) {
        WebElement senderNameField = waitForElementVisible(SENDER_NAME_LOCATOR);
        senderNameField.clear();
        senderNameField.sendKeys(name);
    }

    /**
     * Get the sender name present inside the 'Sender' box.
     *
     * @return The name inside the sender box
     */
    public String getSenderName() {
        return waitForElementPresent(SENDER_NAME_LOCATOR).getAttribute("value");
    }

    /**
     * Verify the sender name matches the given string.
     *
     * @param expectedName The name you expect to be in name box
     * @return true if the name is the same, false otherwise
     */
    public boolean verifySenderName(String expectedName) {
        return getSenderName().equals(expectedName);
    }

    /**
     * Verify an error message stating a name is required is visible.
     *
     * @return true if the error message stating a name is required is visible,
     * false otherwise
     */
    public boolean verifyFromErrorMessageVisible() {
        return waitIsElementVisible(ERROR_MESSAGE_LOCATOR);
    }

    /**
     * Clears then enter a message string at the message input field.
     *
     * @param message The message to enter
     */
    public void enterMessage(String message) {
        WebElement messageField = waitForElementVisible(MESSAGE_LOCATOR);
        messageField.clear();
        messageField.sendKeys(message);
    }

    /**
     * Clear the 'Message' input field.
     */
    public void clearMessage() {
        waitForElementVisible(MESSAGE_LOCATOR).clear();
    }

    /**
     * Get the message in the 'Message' box.
     *
     * @return The message contained in the message box
     */
    public String getMessage() {
        return waitForElementPresent(MESSAGE_LOCATOR).getAttribute("value");
    }

    /**
     * Click on the custom message box.
     */
    public void clickMessageBox() {
        waitForElementPresent(MESSAGE_LOCATOR).click();
    }

    /**
     * Verify the 'Next' button is enabled.
     *
     * @return true if the next button is enabled, false otherwise
     */
    public boolean isNextEnabled() {
        return waitForElementPresent(NEXT_BUTTON_LOCATOR).isEnabled();
    }

    /**
     * Click the 'Next' button to proceed.
     */
    public void clickNext() {
        waitForElementClickable(NEXT_BUTTON_LOCATOR).click();
    }

    /**
     * Click the 'Back' button.
     */
    public void clickBack() {
        waitForElementClickable(BACK_BUTTON_LOCATOR).click();
        sleep(1000); // Wait for close animation to complete
    }

    /**
     * Verify the title contains expected keywords.
     *
     * @return true if the title contains the keywords, false otherwise
     */
    public boolean verifyTitleText() {
        return verifyTitleContainsIgnoreCase(TITLE_KEYWORDS);
    }

    /**
     * Verify the 'Back' button is visible.
     *
     * @return true if the 'Back' button is visible, false otherwise
     */
    public boolean verifyBackButtonExists() {
        return waitIsElementVisible(BACK_BUTTON_LOCATOR);
    }

    /**
     * Verify the packart is visible.
     *
     * @return true if the packart is visible, false otherwise
     */
    public boolean verifyPackArtExists() {
        return waitIsElementVisible(PACK_ART_LOCATOR);
    }

    /**
     * Verify the message header contains a given String.
     *
     * @param expected The string we expect to be in the message header
     * @return true if the header contains the String, false otherwise
     */
    public boolean verifyMessageHeaderContains(String expected) {
        return waitForElementVisible(DIALOG_MESSAGE_HEADER_LOCATOR).getText().contains(expected);
    }

    /**
     * Verify the 'Character Remaining' count matches the given number.
     *
     * @param number The number we expect to be remaining
     * @return true if the remaining count matches, false otherwise
     */
    public boolean verifyCharacterRemainingCount(int number) {
        String charRemainingString = waitForElementVisible(CHARACTER_REMAINING_LOCATOR).getText();
        int charsRemaining = (int) StringHelper.extractNumberFromText(charRemainingString);
        return charsRemaining == number;
    }
}