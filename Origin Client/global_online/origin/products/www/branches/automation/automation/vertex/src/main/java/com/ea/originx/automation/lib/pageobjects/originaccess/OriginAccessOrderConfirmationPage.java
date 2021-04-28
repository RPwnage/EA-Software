package com.ea.originx.automation.lib.pageobjects.originaccess;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Origin Access Order Confirmation' page from the Subscribe and Save
 * checkout flow.
 *
 * @author caleung
 */
public class OriginAccessOrderConfirmationPage extends EAXVxSiteTemplate {

    protected static final String OA_CONFIRMATION_PAGE_TEMPLATE = ".checkout-content div.otkmodal-body.centered-order-confirmation.oa-confirmation div.oa-confirmation-body-wrapper";
    protected static final By OA_CONFIRMATION_LOCATOR = By.cssSelector(".checkout-content div.otkmodal-body.centered-order-confirmation.oa-confirmation");
    protected static final By OA_LOGO_LOCATOR = By.cssSelector(OA_CONFIRMATION_PAGE_TEMPLATE + " img");
    protected static final By DESCRIPTION_LOCATOR = By.cssSelector(OA_CONFIRMATION_PAGE_TEMPLATE + " p");
    protected static final By LEGAL_TEXT_LOCATOR = By.cssSelector("#oa-thankyou-legal-copy");
    protected static final By CLOSE_MODAL_LOCATOR = By.cssSelector("#origin-content origin-iframe-modal div div.otkmodal-dialog.otkmodal-lg div i");
    protected static final By NEXT_BTN_LOCATOR = By.cssSelector(OA_CONFIRMATION_PAGE_TEMPLATE + " .advance-to-stage");

    protected static final String[] DESCRIPTION_EXPECTED_TEXT = { "10%", "discount" };

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OriginAccessOrderConfirmationPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify 'Origin Access Order Confirmation page' reached.
     *
     * @return true if Origin Access Order Confirmation page is reached, false otherwise
     */
    public boolean verifyOAOrderConfirmationPageReached() {
        return waitIsElementVisible(OA_CONFIRMATION_LOCATOR);
    }

    /**
     * Verify the Origin Access logo is displayed.
     *
     * @return true if the Origin Access logo is displayed, false otherwise
     */
    public boolean verifyOALogoVisible() {
        return waitIsElementVisible(OA_LOGO_LOCATOR);
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
     * Verify the description is displayed.
     *
     * @return true if the description text is displayed, false otherwise
     */
    public boolean verifyDescriptionVisible() {
        return waitIsElementVisible(DESCRIPTION_LOCATOR);
    }

    /**
     * Verify the description text is as expected.
     *
     * @param entitlementName Name of the pre-order game
     * @return true if description text is as expected, false otherwise
     */
    public boolean verifyDescriptionCorrect(String entitlementName) {
        return StringHelper.containsAnyIgnoreCase(getDescription(), DESCRIPTION_EXPECTED_TEXT) &&
                StringHelper.containsAnyIgnoreCase(getDescription(), entitlementName);
    }

    /**
     * Verify the legal text is displayed.
     *
     * @return true if the legal text is displayed, false otherwise
     */
    public boolean verifyLegalTextDisplayed() {
        return waitIsElementVisible(LEGAL_TEXT_LOCATOR);
    }

    /**
     * Close the confirmation page.
     */
    public void closeConfirmationPage() {
        driver.switchTo().defaultContent();
        waitForElementVisible(CLOSE_MODAL_LOCATOR).click();
    }

    /**
     * Click the 'Next' button.
     */
    public void clickNextBtn() {
        waitForElementVisible(NEXT_BTN_LOCATOR).click();
    }
}