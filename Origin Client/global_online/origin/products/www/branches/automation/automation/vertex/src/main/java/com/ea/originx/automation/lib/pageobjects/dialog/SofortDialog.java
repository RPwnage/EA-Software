package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the 'Sofort' dialog for the checkout flow through Sofort.
 * In the browser, Sofort opens in a new tab, but the elements remain
 * the same.
 *
 * See https://confluence.ea.com/display/EBI/Integration+Payment+Information#IntegrationPaymentInformation-SofortBanking
 * for doing this manually
 *
 * Note: Currently only works on integration
 *
 * @author jmittertreiner
 */
public class SofortDialog extends Dialog {
    private static final By DIALOG_LOCATOR = By.cssSelector("body .sofort-header");
    private static final By COUNTRY_SELECTOR_LOCATOR = By.cssSelector("#SelectCountryPage");
    private static final By BANK_INPUT_LOCATOR = By.cssSelector("input[name='data[BankCode][search]']");
    private static final By NEXT_BUTTON_LOCATOR = By.cssSelector(".button-right.primary");
    private static final By USER_ID_FIELD_LOCATOR = By.cssSelector("#BackendFormLOGINNAMEUSERID");
    private static final By PIN_FIELD_LOCATOR = By.cssSelector("#BackendFormUSERPIN");
    private static final By ACCOUNT_SELECT_LOCATOR = By.cssSelector(".account-selector > label[for='account-3']");
    private static final By CONFIRMATION_CODE_FIELD_LOCATOR = By.cssSelector("#BackendFormTan");
    private static final By PRICE_LOCATOR = By.cssSelector(".amount.js-toggle-details");
    private static final By PRODUCT_NAME_LOCATOR = By.cssSelector(".more .reason");
    private static final By PAYMENT_DETAILS_EXPAND_CLIENT_LOCATOR = By.cssSelector(".main .sidebar.js-sidebar");
    private static final By PAYMENT_DETAILS_EXPAND_CHROME_LOCATOR = By.cssSelector(".main .sidebar.js-sidebar .amount.js-toggle-details");
    private static final By CLOSE_PAYMENT_DETAILS_DROPDOWN = By.cssSelector(".sidebar-top");
    private static final By ACCEPT_COOKIES_BUTTON_LOCATOR = By.cssSelector(".cookie-text .js-hide-cookie-banner.button[data-cookie-decission='1']");
    
    private String sofortUsername = "fakeUsername";
    private String sofortPassword = "fakePin";
    private String sofortConfirmationCode = "12345";
    private String oldPage ;
    
    public SofortDialog(WebDriver driver) {
        super(driver, DIALOG_LOCATOR);
        focusDialog();
    }
    
    /**
     * Completes the 'Sofort' dialog and returns to the main Origin window.
     */
    public void completePurchase() {
        selectBank();
        fillInInfo();
        selectAccount();
        enterConfirmationCode();
        driver.switchTo().window(oldPage);
    }
    
    /**
     * Opens the Payment Details dropdown and returns the entitlement price.
     *
     * @return A String representing the price of the game
     */
    public String getPrice(boolean isClient) {
        openPaymentDetails(isClient);
        String price = StringHelper.normalizeNumberString(waitForElementVisible(PRICE_LOCATOR).getText());
        closePaymentDetails();
        return price;
    }
    
    /**
     * Opens the Payment Details dropdown and returns the name of the entitlement
     *
     * @return The name of the product
     */
    public String getProductName(boolean isClient) {
        openPaymentDetails(isClient);
        // Element contains the text "734800179899 Battlefield 4" we don't want the first word
        String details = waitForElementVisible(PRODUCT_NAME_LOCATOR).getText().split(" ", 2)[1].trim();
        closePaymentDetails();
        return details;
    }
    
    /**
     * Switches the driver to the window and frame containing the dialog.
     */
    public void focusDialog() {
        oldPage = driver.getWindowHandle();
        waitForPageThatMatches(".*sofort.*");
        waitForInAnyFrame(COUNTRY_SELECTOR_LOCATOR);
    }
    
    /**
     * Selects the demo bank from the select menu.
     */
    public void selectBank() {
        waitForElementVisible(BANK_INPUT_LOCATOR).sendKeys("00000");
        clickNextButton();
    }
    
    /**
     * Fills in the user ID and PIN fields.
     */
    public void fillInInfo() {
        // You can put anything in the fields as long as it's at least 5 characters long
        waitForElementVisible(USER_ID_FIELD_LOCATOR).sendKeys(sofortUsername);
        waitForElementVisible(PIN_FIELD_LOCATOR).sendKeys(sofortPassword);
        clickNextButton();
    }
    
    /**
     * Selects an account to purchase from.
     */
    public void selectAccount() {
        setCheckbox(ACCOUNT_SELECT_LOCATOR, true);
        clickNextButton();
    }
    
    /**
     * Enters the confirmation code.
     */
    public void enterConfirmationCode() {
        waitForElementClickable(CONFIRMATION_CODE_FIELD_LOCATOR).sendKeys(sofortConfirmationCode);
        clickNextButton();
    }
    
    /**
     * Clicks 'Next' button
     */
    public void clickNextButton(){
        sleep(5000); // waiting for button animtion to complete
        waitForElementClickable(NEXT_BUTTON_LOCATOR).click();
    }

    /**
     * Click Accept + continue button
     */
    public void acceptCookies() {
        waitForElementClickable(ACCEPT_COOKIES_BUTTON_LOCATOR).click();
    }
    
    /**
     * Expands the payment details menu.
     */
    private void openPaymentDetails(boolean isClient) {
        WebElement expander;
        if (isClient) {
            expander = waitForElementPresent(PAYMENT_DETAILS_EXPAND_CLIENT_LOCATOR);
            if(!expander.getAttribute("class").contains("up")) {
                expander.click();
            }
        } else {
            expander = waitForElementPresent(PAYMENT_DETAILS_EXPAND_CHROME_LOCATOR);
            if(!expander.getAttribute("class").contains("open")) {
                expander.click();
            }
        }
    }
    
    /**
     * Retracts the payment details menu
     */
    private void closePaymentDetails(){
        if(waitIsElementVisible(CLOSE_PAYMENT_DETAILS_DROPDOWN)){
            waitForElementClickable(CLOSE_PAYMENT_DETAILS_DROPDOWN).click();
            sleep(1000); // wait for dropdown to retract
        }
    }
}