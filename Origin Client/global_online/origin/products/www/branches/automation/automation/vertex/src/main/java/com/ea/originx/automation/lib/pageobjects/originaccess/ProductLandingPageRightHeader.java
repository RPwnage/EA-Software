package com.ea.originx.automation.lib.pageobjects.originaccess;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

import java.util.Arrays;
import java.util.List;

import static com.ea.originx.automation.lib.pageobjects.originaccess.ProductLandingPageHeader.PRODUCT_LANDING_PAGE_HEADER_CSS;

/**
 * Represents the right comparison box header on the 'Product Landing' page of a pre-order game.
 *
 * @author caleung
 */
public class ProductLandingPageRightHeader extends EAXVxSiteTemplate {

    protected static final By RIGHT_COMPARISON_BOX_LOCATOR = By.cssSelector(PRODUCT_LANDING_PAGE_HEADER_CSS + " .origin-store-access-productlanding-comparison div:nth-child(2)");
    protected static final String RIGHT_COMPARISON_BOX_TEMPLATE = PRODUCT_LANDING_PAGE_HEADER_CSS + " .origin-store-access-productlanding-comparison div:nth-child(2)";
    protected static final String LOGOS_LOCATOR = RIGHT_COMPARISON_BOX_TEMPLATE + " .origin-store-access-productlanding-comparison-box-logos";

    // left column logos
    protected static final String LEFT_COLUMN_LOGOS_LOCATOR = LOGOS_LOCATOR + " div:nth-child(1)";
    protected static final By GAME_LOGO_LOCATOR = By.cssSelector(LEFT_COLUMN_LOGOS_LOCATOR + " .origin-store-access-productlanding-comparison-gamelogo img");
    protected static final By GAME_PRICE_LOCATOR = By.cssSelector(LEFT_COLUMN_LOGOS_LOCATOR + " span.origin-store-access-productlanding-comparison-price");
    protected static final By DISCOUNT_PERCENTAGE_LOCATOR = By.cssSelector(LEFT_COLUMN_LOGOS_LOCATOR + " span.origin-store-access-productlanding-comparison-discount");

    // right column logos
    protected static final String RIGHT_COLUMN_LOGOS_LOCATOR = LOGOS_LOCATOR + " div:nth-child(3)";
    protected static final By ORIGIN_ACCESS_LOGO_LOCATOR = By.cssSelector(RIGHT_COLUMN_LOGOS_LOCATOR + " .origin-store-access-productlanding-comparison-gamelogo");
    protected static final By ORIGIN_ACCESS_PRICE_LOCATOR = By.cssSelector(RIGHT_COLUMN_LOGOS_LOCATOR + " .origin-store-access-productlanding-comparison-price");

    protected static final By CTA_LOCATOR = By.cssSelector(RIGHT_COMPARISON_BOX_TEMPLATE + " .origin-store-access-productlanding-comparison-box-cta .origin-telemetry-savesubscribe-cta .origin-cta-primary .otkbtn .otkbtn-primary-text");
    protected static final String[] CTA_EXPECTED_KEYWORDS = { "join", "pre-order" };
    protected static final String LIST_ITEM_TEMPLATE = RIGHT_COMPARISON_BOX_TEMPLATE + " ul li:nth-child(%s) span b";
    protected static final By GAME_NAME_LOCATOR = By.cssSelector(String.format(LIST_ITEM_TEMPLATE, "1"));

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ProductLandingPageRightHeader(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify right comparison box is present.
     *
     * @return true if right comparison box is visible, false otherwise
     */
    public boolean verifyRightComparisonBoxVisible() {
        return waitIsElementVisible(RIGHT_COMPARISON_BOX_LOCATOR);
    }

    /**
     * Verify the right comparison box game logo is visible.
     *
     * @return true if the right comparison box game logo is visible, false otherwise
     */
    public boolean verifyGameLogoVisible() {
        return waitIsElementVisible(GAME_LOGO_LOCATOR);
    }

    /**
     * Verify the right comparison box Origin Access logo is visible.
     *
     * @return true if the right comparison box Origin Access logo is visible, false otherwise
     */
    public boolean verifyOriginAccessLogoVisible() {
        return waitIsElementVisible(ORIGIN_ACCESS_LOGO_LOCATOR);
    }

    /**
     * Verify the price after Origin Access discount is correct for an Origin
     * Access user for a game.
     *
     * @param expectedPrice expected price
     * @return true if price is correct, false otherwise
     */
    public boolean verifyPriceAfterDiscountIsCorrect(String expectedPrice) {
        try {
            String price = waitForElementVisible(GAME_PRICE_LOCATOR).getText();
            return StringHelper.containsIgnoreCase(price, expectedPrice);
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify discount percentage for the game is visible.
     *
     * @return true if discount percentage is visible, false otherwise
     */
    public boolean verifyGameDiscountPercentageVisible() {
        return waitIsElementVisible(DISCOUNT_PERCENTAGE_LOCATOR);
    }

    /**
     * Get the discount percentage in the right comparison box for the game.
     *
     * @return discount percentage as a double
     */
    public double getDiscountPercentage() {
        return StringHelper.extractNumberFromText(waitForElementVisible(DISCOUNT_PERCENTAGE_LOCATOR).getText());
    }

    /**
     * Verify the Origin Access price is visible.
     *
     * @return true if price is visible, false otherwise
     */
    public boolean verifyOriginAccessPriceVisible() {
        return waitIsElementVisible(ORIGIN_ACCESS_PRICE_LOCATOR);
    }

    /**
     * Click on right comparison box CTA.
     */
    public void clickOnCTA() {
        waitForElementClickable(CTA_LOCATOR).click();
    }

    /**
     * Verify the right comparison box CTA is visible.
     *
     * @return true if right comparison box CTA is visible, false otherwise
     */
    public boolean verifyCTAVisible() {
        return waitIsElementVisible(CTA_LOCATOR);
    }

    /**
     * Get the text of the right comparison box CTA.
     *
     * @return text of the right comparison box CTA as a String
     */
    public String getCTAText() {
        return driver.findElement(CTA_LOCATOR).getText();
    }

    /**
     * Verify the right comparison box CTA text is as expected.
     *
     * @return true if the right comparison box CTA text is expected
     */
    public boolean verifyCTATextIsCorrect() {
        return StringHelper.containsIgnoreCase(getCTAText(), CTA_EXPECTED_KEYWORDS);
    }

    /**
     * Get the Origin Access price in the right comparison box.
     *
     * @return Origin Access price as a double
     */
    public double getOriginAccessPrice() {
        return StringHelper.extractNumberFromText(waitForElementVisible(ORIGIN_ACCESS_PRICE_LOCATOR).getText());
    }

    /**
     * Get the right comparison box first bullet point, which is the pre-order game name.
     *
     * @return name of the pre-order game name
     */
    public String getGameName() {
        return waitForElementVisible(GAME_NAME_LOCATOR).getText();
    }

    /**
     * Verify the pre-order game name in the right comparison box is correct.
     *
     * @param expectedName Expected game name
     * @return true if price is correct, false otherwise
     */
    public boolean verifyGameNameCorrect(String expectedName) {
        try {
            String gameName = getGameName();
            return StringHelper.containsIgnoreCase(gameName, expectedName);
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify benefits are visible in the right comparison box.
     *
     * @return true if all benefits are visible, false otherwise
     */
    public boolean verifyBenefitsVisible() {
        List<WebElement> rightComparisonBoxBenefits = Arrays.asList(driver.findElement(By.cssSelector(String.format(LIST_ITEM_TEMPLATE, "2"))),
                driver.findElement(By.cssSelector(String.format(LIST_ITEM_TEMPLATE, "3"))), driver.findElement(By.cssSelector(String.format(LIST_ITEM_TEMPLATE, "4"))));
        return rightComparisonBoxBenefits.stream()
                .allMatch(b -> waitIsElementVisible(b));
    }
}