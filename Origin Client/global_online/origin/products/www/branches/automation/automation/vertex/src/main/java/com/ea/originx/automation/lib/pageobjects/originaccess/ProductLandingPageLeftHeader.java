package com.ea.originx.automation.lib.pageobjects.originaccess;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

import java.util.List;

import static com.ea.originx.automation.lib.pageobjects.originaccess.ProductLandingPageHeader.PRODUCT_LANDING_PAGE_HEADER_CSS;

/**
 * Represents the left comparison box header on the 'Product Landing' page of a pre-order game.
 *
 * @author caleung
 */
public class ProductLandingPageLeftHeader extends EAXVxSiteTemplate {

    protected static final String LEFT_COMPARISON_BOX_TEMPLATE = PRODUCT_LANDING_PAGE_HEADER_CSS + " > div.origin-store-access-productlanding-comparison.origin-telemetry-store-access-productlanding-comparison > div:nth-child(1)";
    protected static final By LEFT_COMPARISON_BOX_LOCATOR = By.cssSelector(LEFT_COMPARISON_BOX_TEMPLATE);
    protected static final By GAME_LOGO_LOCATOR = By.cssSelector(LEFT_COMPARISON_BOX_TEMPLATE + " .origin-store-access-productlanding-comparison-box-logo .origin-store-access-productlanding-comparison-gamelogo img");
    protected static final By CTA_LOCATOR = By.cssSelector(LEFT_COMPARISON_BOX_TEMPLATE + " .origin-store-access-productlanding-comparison-box-cta origin-cta-purchase .origin-store-purchase-cta-btnwithdropdown div .otkbtn");
    protected static final By GAME_NAME_LOCATOR = By.cssSelector(LEFT_COMPARISON_BOX_TEMPLATE + " > .origin-store-access-list > li:nth-child(1) > span > b");
    protected static final By BENEFITS_LOCATOR = By.cssSelector(LEFT_COMPARISON_BOX_TEMPLATE + " > .origin-store-access-list > li.origin-store-access-item-disabled");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ProductLandingPageLeftHeader(WebDriver driver) {
        super(driver);
    }


    /**
     * Verify left comparison box is present.
     *
     * @return true if left comparison box is visible, false otherwise
     */
    public boolean verifyLeftComparisonBoxVisible() {
        return waitIsElementVisible(LEFT_COMPARISON_BOX_LOCATOR);
    }

    /**
     * Verify left comparison box game logo is shown.
     *
     * @return true if game logo is shown in left comparison box, false otherwise
     */
    public boolean verifyLeftComparisonBoxLogoVisible() {
        return waitIsElementVisible(GAME_LOGO_LOCATOR);
    }

    /**
     * Clicks the CTA in the left comparison box.
     */
    public void clickLeftComparisonBoxCTA() {
        waitForElementClickable(CTA_LOCATOR).click();
    }

    /**
     * Gets the CTA text of the left comparison box.
     *
     * @return the text of the left comparison box
     */
    public String getLeftComparisonBoxCTAText() {
        return driver.findElement(CTA_LOCATOR).getText();
    }

    /**
     * Gets the game name displayed in the left comparison box.
     *
     * @return Game name displayed in the left comparison box
     */
    public String getLeftComparisonBoxGameName() {
        return driver.findElement(GAME_NAME_LOCATOR).getText();
    }

    /**
     * Verify the left comparison box game name and passed expected game name are the same.
     *
     * @param expectedGameName Expected game name for the left comparison box CTA
     * @return true if left comparison box game name matches the expected game name
     */
    public boolean verifyLeftComparisonBoxGameNameIsCorrect(String expectedGameName) {
        return StringHelper.containsIgnoreCase(getLeftComparisonBoxGameName(), expectedGameName);
    }

    /**
     * Verify left comparison box CTA text and passed expected text are the same.
     *
     * @param expectedText Expected text for the left comparison box CTA
     * @return true if left comparison box CTA text matches the expected text
     */
    public boolean verifyLeftComparisonBoxCTATextIsCorrect(String expectedText) {
        return StringHelper.containsIgnoreCase(getLeftComparisonBoxCTAText(), expectedText);
    }

    /**
     * Verify the Origin Access benefits have strike through text.
     *
     * @return true if Origin Access benefits are displayed correctly, false otherwise
     */
    public boolean verifyOriginAccessBenefitIsCorrect() {
        List<WebElement> disabledBenefits= driver.findElements(BENEFITS_LOCATOR);
        return disabledBenefits.stream()
                .allMatch(b -> verifyTextIsStrikedThrough(b));
    }

    /**
     * Verify the Origin Access benefit text is striked through
     *
     * @param webElement WebElement of benefit text
     * @return true if text is striked through, false otherwise
     */
    public boolean verifyTextIsStrikedThrough(WebElement webElement) {
        try {
            String textDeco = webElement.getCssValue("text-decoration");
            return StringHelper.containsIgnoreCase(textDeco, "line-through");
        } catch (TimeoutException e) {
            return false;
        }
    }
}