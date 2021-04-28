package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.originx.automation.lib.pageobjects.template.QtDialog;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebDriverException;

/**
 * Represents the PayPal dialog for checkout flow through PayPal.
 * In the browser, PayPal opens in a new tab, but the element remain
 * the same.
 *
 * Note: Currently does not work on production
 *
 * @author jmittertreiner
 */
public class PayPalDialog extends Dialog {

    // Paypal information for not production environments
    private static final String PAYPAL_EMAIL = "originstoreCA@gmail.com";
    private static final String PAYPAL_PASSWORD = "QAtest1!";

    private static  String INPUT_LOCATOR = ".fieldWrapper ";
    private static  By EMAIL_LOCATOR = By.cssSelector(INPUT_LOCATOR + "#email");
    private static  By PASSWORD_LOCATOR = By.cssSelector(INPUT_LOCATOR + "#password");
    private static  By LOGIN_IN_LOCATOR = By.cssSelector("#btnLogin");
    private static  By CONTINUE_LOCATOR = By.cssSelector("#button");

    private String oldPage ;
    private QtPayPalDialog qtPayPalDialog = null;
    
    private class QtPayPalDialog extends QtDialog {

        /**
         * Constructor
         *
         * @param webDriver Selenium WebDriver
         */
        public QtPayPalDialog(WebDriver driver) {
            super(driver);
        }
    }
    
    public PayPalDialog(WebDriver driver) {
        super(driver,EMAIL_LOCATOR);
        qtPayPalDialog = new QtPayPalDialog(driver);
        focusDialog();
    }
    
    /**
     * Completes the PayPal purchase dialog and returns to the main Origin window.
     */
    public void loginAndCompletePurchase() {
        enterEmail(PAYPAL_EMAIL);
        enterPassword(PAYPAL_PASSWORD);
        clickLogin();
        clickContinue();
        driver.switchTo().window(oldPage);
    }

    /**
     * Switches the driver to the window and frame containing the dialog.
     */
    public void focusDialog() {
        oldPage = driver.getWindowHandle();
        waitForPageThatMatches(".*paypal.*", 30);
        try{
            waitForInAnyFrame(EMAIL_LOCATOR);
            // sometimes the PayPal UI load a different with a different interface
        }catch(TimeoutException e){
            INPUT_LOCATOR = "#loginBox ";
            EMAIL_LOCATOR = By.cssSelector(INPUT_LOCATOR + "#login_email");
            waitForInAnyFrame(EMAIL_LOCATOR);
            PASSWORD_LOCATOR = By.cssSelector(INPUT_LOCATOR + "#login_password");
            LOGIN_IN_LOCATOR = By.cssSelector(INPUT_LOCATOR + "#submitLogin");
            CONTINUE_LOCATOR = By.cssSelector("#continue_abovefold");
        }
    }
    
    /**
     * Enters the given email into the email field.
     *
     * @param email The email address to enter
     */
    public void enterEmail(String email) {
        waitForElementVisible(EMAIL_LOCATOR).sendKeys(email);
    }
    
    /**
     * Enters the given password into the password field.
     *
     * @param password The password to enter
     */
    public void enterPassword(String password) {
        waitForElementVisible(PASSWORD_LOCATOR).sendKeys(password);
    }

    /**
     * Clicks the 'Login' button and waits for the continue page to load.
     */
    public void clickLogin() {
        waitForElementClickable(LOGIN_IN_LOCATOR).click();
    }

    /**
     * Clicks the 'Continue' button (the dialog should close after this).
     */
    public void clickContinue() {
        // Button is visible and clickable but a loading overlay is blocking it
        for(int i = 0; i < 5; i++){
            try {
                waitForElementVisible(CONTINUE_LOCATOR, 5).click();
            } catch(WebDriverException e){
                sleep(1000);
            }
        }
    }
    
}