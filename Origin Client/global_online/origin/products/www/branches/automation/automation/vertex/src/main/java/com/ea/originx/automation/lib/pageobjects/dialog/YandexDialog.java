package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Yandex' dialog for the checkout flow through Yandex.
 * In the browser, Yandex opens in a new tab, but the elements remain
 * the same.
 *
 * @author caleung
 */
public class YandexDialog extends Dialog {
    private static final By DIALOG_LOCATOR = By.cssSelector("body.b-page");
    private static final By CREDIT_CARD_NUMBER_FIELD_LOCATOR = By.cssSelector("input[name='skr_card-number']");
    private static final By MONTH_FIELD_LOCATOR = By.cssSelector("input[name='skr_month'");
    private static final By YEAR_FIELD_LOCATOR = By.cssSelector("input[name='skr_year'");
    private static final By CSV_FIELD_LOCATOR = By.cssSelector("input[name='skr_cardCvc']");
    private static final By PAY_BUTTON_LOCATOR = By.cssSelector("div.island__section.island__section_theme_top-border-no-ident.island__section_name_submit-row.clearfix > button");
    private static final By BACK_TO_SHOP_BUTTON_LOCATOR = By.cssSelector("div.payment-success__back-control-link a.payment-success__back-control");

    private String oldPage;
    private String creditCardNumber = "4444 4444 4444 4448";

    public YandexDialog(WebDriver driver) {
        super(driver, DIALOG_LOCATOR);
        focusDialog();
    }

    /**
     * Verify Yandex dialog is loaded.
     */
    public boolean verifyYandexDialogVisible() {
        waitForElementVisible(CREDIT_CARD_NUMBER_FIELD_LOCATOR);
        return super.isOpen() && isElementVisible(CREDIT_CARD_NUMBER_FIELD_LOCATOR);
    }

    /**
     * Switches the driver to the window and frame containing the dialog.
     */
    public void focusDialog() {
        oldPage = driver.getWindowHandle();
        waitForPageThatMatches(".*yandex.*");
        Waits.sleep(5000); // wait because of timing issues
        waitForInAnyFrame(CREDIT_CARD_NUMBER_FIELD_LOCATOR);
    }

    /**
     * Clicks 'Pay' button
     */
    public void clickPayButton(){
        sleep(1000); // waiting for button animation to complete
        waitForElementClickable(PAY_BUTTON_LOCATOR).click();
    }

    /**
     * Fills in the credit card information.
     */
    public void fillInInfo() {
        // You can put anything in the fields as long as it's at least 5 characters long
        enterCreditCardNumber(creditCardNumber);
        enterCreditCardExpirationMonth(OriginClientData.CREDIT_CARD_EXPIRATION_MONTH);
        enterCreditCardExpirationYear(OriginClientData.CREDIT_CARD_EXPIRATION_YEAR);
        enterCSV(OriginClientData.CSV_CODE);
        clickPayButton();
    }

    /**
     * Enters the given credit card number.
     * @param creditCardNumber The credit card number to enter into the credit card number field.
     */
    public void enterCreditCardNumber(String creditCardNumber) {
        waitForElementVisible(CREDIT_CARD_NUMBER_FIELD_LOCATOR).sendKeys(creditCardNumber);
    }

    /**
     * Enters the given credit card expiration month.
     * @param expirationMonth The month the credit card is expiring.
     */
    public void enterCreditCardExpirationMonth(String expirationMonth) {
        waitForElementVisible(MONTH_FIELD_LOCATOR).sendKeys(expirationMonth);
    }

    /**
     * Enters the given credit card expiration year.
     * @param expirationYear The year the credit card is expiring.
     */
    public void enterCreditCardExpirationYear(String expirationYear) {
        waitForElementVisible(YEAR_FIELD_LOCATOR).sendKeys(expirationYear);
    }

    /**
     * Enters the given CSV code.
     * @param csv The CSV code
     */
    public void enterCSV(String csv) {
        waitForElementVisible(CSV_FIELD_LOCATOR).sendKeys(csv);
    }

    /**
     * Complete purchase flow.
     */
    public void completePurchase() {
        fillInInfo();
        clickBackToShopButton();
        driver.switchTo().window(oldPage);
    }

    /**
     * Click the 'Back to Shop' button that shows up after entering payment information.
     */
    public void clickBackToShopButton() {
        waitForElementClickable(BACK_TO_SHOP_BUTTON_LOCATOR).click();
    }
}