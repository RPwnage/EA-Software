package com.ea.originx.automation.lib.pageobjects.login;

import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Password Recovery' page.
 *
 * @author mkalaivanan
 */
public class PasswordRecoveryPage extends EAXVxSiteTemplate {

    protected static final String PASSWORD_RECOVERY_FORM_CSS = "#resetpasswordformrequest";
    protected static final String PASSWORD_RECOVERY_PANEL_CSS = PASSWORD_RECOVERY_FORM_CSS + " #panel-reset-password-request";
    protected static final By EMAIL_ID_LOCATOR = By.cssSelector("#email");
    protected static final By CAPTCHA_LOCATOR = By.cssSelector("#recaptcha_response_field");
    protected static final By SEND_BUTTON_LOCATOR = By.cssSelector("#sendBtn");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public PasswordRecoveryPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Wait for the 'Password Recovery' page to load.
     */
    public void waitForPasswordRecoveryPageToLoad() {
        boolean isClient = OriginClient.getInstance(driver).isQtClient(driver);
        if (isClient) {
            waitForPageToLoad();
            Waits.waitForWindowThatHasElement(driver, SEND_BUTTON_LOCATOR);
        } else {
            Waits.waitForPageThatMatches(driver, "^https://signin.int.ea.com/p/originX/resetPassword.*", 110);
        }

        waitForElementVisible(SEND_BUTTON_LOCATOR);
    }

    /**
     * Verify 'Password Recovery' page has been reached.
     *
     * @return true if the 'Send' button is visible, false otherwise
     */
    public boolean verifyPasswordRecoveryPageReached() {
        return waitIsElementVisible(SEND_BUTTON_LOCATOR);
    }

    /**
     * Click the 'Send' button.
     */
    public void clickSendButton() {
        forceClickElement(waitForElementPresent(SEND_BUTTON_LOCATOR));
    }

    /**
     * Enter the given email address into the 'Email Address' field.
     *
     * @param emailAddress Email address which requires password to be reset
     */
    public void enterEmailAddress(String emailAddress) {
        waitForElementVisible(EMAIL_ID_LOCATOR).sendKeys(emailAddress);
    }

    /**
     * Enter the 'Captcha'.
     *
     * @param captcha 'Captcha' to be entered
     */
    public void enterCaptcha(String captcha) {
        waitForElementVisible(CAPTCHA_LOCATOR).sendKeys(captcha);
    }
}
