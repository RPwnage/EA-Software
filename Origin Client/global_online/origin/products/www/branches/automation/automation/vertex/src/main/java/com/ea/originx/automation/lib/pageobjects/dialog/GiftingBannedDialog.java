package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Gifting Banned' dialog.
 *
 * @author cvanichsarn
 */
public class GiftingBannedDialog extends Dialog {
    
    private static final By DIALOG_LOCATOR = By.cssSelector("origin-dialog-errormodal");
    private static final By HEADER_LOCATOR = By.cssSelector(".origin-dialog-content-errormodal-title");
    private static final String BODY_CSS = ".origin-dialog-content-errormodal-text";
    private static final By BODY_LOCATOR = By.cssSelector(BODY_CSS);
    
    private static final String[] EXPECTED_BODY_TEXT_KEYWORDS = {"banned","suspended","ineligible","gift"};
    private static final By HELP_LINK_LOCATOR = By.cssSelector(BODY_CSS + " a.external-link");
    private static final String HELP_LINK_TEXT = "How To Address Your Banned or Suspended Account";
    private static final String HELP_LINK_URL_REGEX = "https.*help.ea.com.*information-about-banned-or-suspended-accounts";
    private static final By HELP_EMAIL_LOCATOR = By.cssSelector(BODY_CSS + " a:nth-child(2)");
    private static final String HELP_EMAIL_TEXT = "lets_talk@ea.com";
    private static final String HELP_EMAIL_LINK = "mailto:lets_talk@ea.com";
    
    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public GiftingBannedDialog(WebDriver driver) {
        super(driver, DIALOG_LOCATOR);
    }
    
    /**
     * Wait for the dialog to appear, then wait an extra second for the dialog's
     * animation to complete.
     *
     * @return true if the dialog becomes visible, false otherwise
     */
    @Override
    public boolean waitIsVisible() {
        boolean opened = super.waitIsVisible();
        sleep(1000); // Wait for Animations to Complete
        return opened;
    }
    
    /**
     * Retrieve the text in the body of the dialog.
     * 
     * @return The text obtained from the body of the dialog
     */
    public String getMessage() {
        return waitForElementVisible(BODY_LOCATOR).getText();
    }
    
    /**
     * Checks if the dialog has the correct text, link text and email text.
     * 
     * @return true if all three are true, false otherwise
     */
    public boolean verifyDialogIsAccurate() {
        return verifyDialogText() && verifyHelpLinkText() && verifyHelpEmailText();
    }
    
    /**
     * Verifies that the text in the body of the dialog contains certain keywords.
     * 
     * @return true if the text contains the keywords, false otherwise
     */
    public boolean verifyDialogText() {
        return StringHelper.containsIgnoreCase(getMessage(), EXPECTED_BODY_TEXT_KEYWORDS);
    }
    
    /**
     * Clicks the link to the help page for banned users.
     */
    public void clickHelpLink() {
        waitForElementClickable(HELP_LINK_LOCATOR).click();
    }
    
    /**
     * Verifies that the text for the link to the 'Help' page is accurate.
     * 
     * @return true if the text is accurate, false otherwise
     */
    public boolean verifyHelpLinkText() {
        String helpLinkText = driver.findElement(HELP_LINK_LOCATOR).getText();
        return helpLinkText.equals(HELP_LINK_TEXT);
    }
    
    /**
     * Verify that the URL for the 'Help' link is accurate.
     * 
     * @return true if the URL is accurate, false otherwise
     */
    public boolean verifyHelpLinkUrl() {
        String helpLinkURL = driver.findElement(HELP_LINK_LOCATOR).getAttribute("href");
        return helpLinkURL.matches(HELP_LINK_URL_REGEX);
    }
    
    /**
     * Return the URL for the 'Help' link.
     * 
     * @return the URL as a String
     */
    public String getHelpLinkUrl() {
        return driver.findElement(HELP_LINK_LOCATOR).getAttribute("href");
    }
    
    /**
     * Click on the email link.
     */
    public void clickHelpEmail() {
        waitForElementClickable(HELP_EMAIL_LOCATOR).click();
    }
    
    /**
     * Verify that the text for the email is accurate.
     * 
     * @return true if the text for the email is accurate, false otherwise
     */
    public boolean verifyHelpEmailText() {
        String helpEmailText = driver.findElement(HELP_EMAIL_LOCATOR).getText();
        return helpEmailText.equals(HELP_EMAIL_TEXT);
    }
    
    /**
     * Verify that the link for the email is accurate.
     * 
     * @return true if the link is accurate, false otherwise
     */
    public boolean verifyHelpEmailLink() {
        String helpEmailLink = driver.findElement(HELP_EMAIL_LOCATOR).getAttribute("href");
        return helpEmailLink.equals(HELP_EMAIL_LINK);
    }
}