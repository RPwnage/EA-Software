package com.ea.originx.automation.lib.pageobjects.originaccess;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import java.util.List;

/**
 * Represents the 'Product Landing' page footer of a pre-order game.
 *
 * @author caleung
 */
public class ProductLandingPageFooter extends EAXVxSiteTemplate {

    protected static final String FOOTER_LOCATOR_TEMPLATE = "#storecontent origin-store-access-landing-footer .origin-store-access-landing-footer .origin-store-access-landing-footer-content";
    protected static final By FOOTER_LOCATOR = By.cssSelector(FOOTER_LOCATOR_TEMPLATE);
    protected static final By ORIGIN_ACCESS_LOGO_LOCATOR = By.cssSelector(FOOTER_LOCATOR_TEMPLATE + " .origin-store-access-landing-footer-logo");
    protected static final By HEADER_LOCATOR = By.cssSelector(FOOTER_LOCATOR_TEMPLATE + " .otktitle-page");
    protected static final By BULLET_POINT_LOCATOR = By.cssSelector("ng-transclude .origin-store-access-list li span");
    protected static final By CTA_LOCATOR = By.cssSelector(FOOTER_LOCATOR_TEMPLATE + " origin-store-access-cta .origin-cta-primary .otkbtn .otkbtn-primary-text");
    protected static final By LEGAL_TEXT_LOCATOR = By.cssSelector(FOOTER_LOCATOR_TEMPLATE + " .otktitle-4");
    protected static final By PC_ONLY_TEXT_LOCATOR = By.cssSelector(FOOTER_LOCATOR_TEMPLATE + " .otktitle-5");

    protected static final String[] EXPECTED_HEADER_KEYWORDS = { "first", "Origin", "Access"};
    protected static final String[] EXPECTED_CTA_KEYWORDS = { "join", "pre-order" };
    protected static final String[] PC_ONLY_TEXT_EXPECTED = { "only", "pc" };


    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ProductLandingPageFooter(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the Origin Access logo is displayed.
     *
     * @return true if Origin Access logo is displayed, false otherwise
     */
    public boolean verifyOriginAccessLogoDisplayed() {
        return waitIsElementVisible(ORIGIN_ACCESS_LOGO_LOCATOR);
    }

    /**
     * Scroll to the product landing page footer.
     */
    public void scrollToProductLandingPageFooter() {
        scrollToElement(waitForElementClickable(FOOTER_LOCATOR));
    }

    /**
     * Verify header is displayed.
     *
     * @return true if header is displayed, false otherwise
     */
    public boolean verifyHeaderDisplayed() {
        return waitIsElementVisible(HEADER_LOCATOR);
    }

    /**
     * Get header text.
     *
     * @return Header text
     */
    public String getFooterHeaderText() {
        return waitForElementVisible(HEADER_LOCATOR).getText();
    }

    /**
     * Verify header text is correct.
     *
     * @param entitlementName Name of entitlement
     * @return true if header text contains is correct, false otherwise
     */
    public boolean verifyHeaderTextCorrect(String entitlementName) {
        return StringHelper.containsAnyIgnoreCase(getFooterHeaderText(), EXPECTED_HEADER_KEYWORDS) &&
                StringHelper.containsIgnoreCase(getFooterHeaderText(), entitlementName);
    }

    /**
     * Verify all bullet points are visible.
     *
     * @return true if all bullet points are visible, false otherwise
     */
    public boolean verifyAllBulletPointsVisible() {
        List<WebElement> bulletPoints = driver.findElements(BULLET_POINT_LOCATOR);
        return bulletPoints.stream()
                .allMatch(b -> waitIsElementVisible(b));
    }

    /**
     * Get the text of the footer CTA.
     *
     * @return text of the footer CTA as a String
     */
    public String getFooterCTAText() {
        return driver.findElement(CTA_LOCATOR).getText();
    }

    /**
     * Verify the footer CTA text is as expected.
     *
     * @param expectedPrice Expected total price
     * @return true if the footer CTA text contains expectedText,
     * false otherwise
     */
    public boolean verifyFooterCTATextIsCorrect(String expectedPrice) {
        return StringHelper.containsAnyIgnoreCase(getFooterCTAText(), EXPECTED_CTA_KEYWORDS) &&
                StringHelper.containsIgnoreCase(getFooterCTAText(), expectedPrice);
    }

    /**
     * Verify footer legal text is displayed.
     *
     * @return true if the footer legal text is displayed, false otherwise
     */
    public boolean verifyFooterLegalTextDisplayed() {
        return waitIsElementVisible(LEGAL_TEXT_LOCATOR);
    }

    /**
     * Verify 'Only for PC' text is displayed.
     *
     * @return true if the footer 'Only for PC' text is displayed, false otherwise
     */
    public boolean verifyFooterOnlyPCTextDisplayed() {
        return waitIsElementVisible(PC_ONLY_TEXT_LOCATOR);
    }

    /**
     * Verify 'Only for PC' text is correct.
     *
     * @return true if text is correct, false otherwise
     */
    public boolean verifyFooterOnlyPCTextCorrect() {
        return StringHelper.containsAnyIgnoreCase(waitForElementVisible(PC_ONLY_TEXT_LOCATOR).getText(), PC_ONLY_TEXT_EXPECTED);
    }

    /**
     * Click on the footer CTA.
     */
    public void clickOnFooterCTA() {
        waitForElementClickable(CTA_LOCATOR).click();
    }
}