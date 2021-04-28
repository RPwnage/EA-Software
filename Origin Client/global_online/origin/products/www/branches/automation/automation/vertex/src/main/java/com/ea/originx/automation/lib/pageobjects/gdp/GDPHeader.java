package com.ea.originx.automation.lib.pageobjects.gdp;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.vx.originclient.resources.CountryInfo;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;


/**
 * Page object that represents the 'GDP Header' at the top of a Games Details Page (GDP).
 *
 * @author nvarthakavi
 */
public class GDPHeader extends EAXVxSiteTemplate {

    protected static final String PRODUCT_HERO_INFO_BLOCK_TITLE_KEYWORDS = "Included with";

    protected static final By GDP_HEADER_LOCATOR = By.cssSelector("#storecontent origin-store-gdp-header");
    protected static final By PRODUCT_HERO_LOCATOR = By.cssSelector("otkex-product-hero");

    //Background
    protected static final By PRODUCT_HERO_BACKGROUND_LOCATOR = By.cssSelector(".otkex-product-hero-background-wrapper");

    //Game Logo & Game Title
    protected static final By PRODUCT_HERO_LOGO_LOCATOR = By.cssSelector(".otkex-product-hero-logo");
    protected static final By PRODUCT_HERO_TITLE_LOCATOR = By.cssSelector(".otkex-product-hero-title");

    //Description
    protected static final By PRODUCT_HERO_LEFT_RAIL_HEADER_LOCATOR = By.cssSelector(".otkex-product-hero-leftrail-header");
    protected static final By PRODUCT_HERO_LEFT_RAIL_DESCRIPTION_LOCATOR = By.cssSelector(".otkex-product-hero-leftrail-description");
    protected static final By PRODUCT_HERO_LEFT_RAIL_READ_MORE_LOCATOR = By.cssSelector(".otkex-product-hero-leftrail-readmore");

    //Game Rating
    protected static final By PRODUCT_HERO_ENTITLEMENT_RATING_LOCATOR = By.cssSelector(".otkex-rating img");

    //Info Block
    protected static final By PRODUCT_HERO_INFO_BLOCK_LOCATOR = By.cssSelector("otkex-infoblock");
    protected static final By PRODUCT_HERO_INFO_BLOCK_ICON_LOCATOR = By.cssSelector("otkex-infoblock .otkicon");
    protected static final By PRODUCT_HERO_INFO_BLOCK_TITLE_LOCATOR = By.cssSelector("otkex-infoblock .otktitle-4");
    protected static final By PRODUCT_HERO_INFO_BLOCK_TITLE_LOGO_LOCATOR = By.cssSelector("otkex-infoblock .otktitle-4 img");

    //Legal Block
    protected static final By PRODUCT_HERO_LEGAL_BLOCK_EA_USER_AGREEMENT = By.cssSelector(".origin-storegamelegal-eula");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public GDPHeader(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify 'GDP Header' (i.e. GDP page) reached.
     *
     * @return true if the 'GDP Header' is present on the page, false otherwise
     */
    public boolean verifyGDPHeaderReached() {
        return waitIsElementPresent(GDP_HEADER_LOCATOR);
    }

    /**
     * Get 'Offer ID' for the entitlement on the current PDP page.
     */
    public String getGameName() {
        return waitForElementPresent(PRODUCT_HERO_LOCATOR).getAttribute("game-name");
    }

    /**
     * Get the 'GDP' entitlement rating
     *
     * @return the entitlement rating name
     */
    public String getGDPEntitlementRating() {
        return waitForElementPresent(PRODUCT_HERO_ENTITLEMENT_RATING_LOCATOR).getAttribute("alt");
    }

    /**
     * Get the 'GDP' infoblock icon web element
     *
     * @return the infoblock icon web element
     */
    public WebElement getGDPInfoBlockIcon() {
        return waitForElementPresent(PRODUCT_HERO_INFO_BLOCK_ICON_LOCATOR);
    }

    /**
     * Get the 'GDP' infoblock title web element
     *
     * @return the infoblock title web element
     */
    public WebElement getGDPInfoBlockTitle() {
        return waitForElementPresent(PRODUCT_HERO_INFO_BLOCK_TITLE_LOCATOR);
    }

    /**
     * Get the 'GDP' infoblock title text
     *
     * @return the infoblock title text
     */
    public String getGDPInfoBlockTitleText() {
        return getGDPInfoBlockTitle().getText();
    }

    /**
     * Verify the Background Image is visible
     *
     * @return true if the background image is visible
     */
    public boolean verifyHeroBackgroundVisible() {
        return waitIsElementVisible(PRODUCT_HERO_BACKGROUND_LOCATOR);
    }

    /**
     * Verify the 'Product Hero Game Title/Game Logo' is visible.
     *
     * @return true if the game title/game logo is visible, false otherwise
     */
    public boolean verifyGameTitleOrLogoVisible() {
        return waitIsElementVisible(waitForAnyElementVisible(PRODUCT_HERO_LOGO_LOCATOR, PRODUCT_HERO_TITLE_LOCATOR));
    }

    /**
     * Verify the 'Product Hero Left Rail Header' is visible
     *
     * @return true if the header is visible, false otherwise
     */
    public boolean verifyProductHeroLeftRailHeaderVisible() {
        return waitIsElementVisible(PRODUCT_HERO_LEFT_RAIL_HEADER_LOCATOR);
    }

    /**
     * Verifies the Rating is correctly localized based on the country given
     *
     * @param country the country localization for a specific rating
     * @return true if correctly localized, false otherwise
     */
    public boolean verifyGDPEntitlementRating(CountryInfo.CountryEnum country) {
        return StringHelper.containsIgnoreCase(getGDPEntitlementRating(), country.getCountryGameRating());
    }

    /**
     * Verify the 'Product Hero Left Rail Description' is visible
     *
     * @return true if the description is visible, false otherwise
     */
    public boolean verifyProductHeroLeftRailDescriptionVisible() {
        return waitIsElementVisible(PRODUCT_HERO_LEFT_RAIL_DESCRIPTION_LOCATOR);
    }

    /**
     * Verify the 'Product Hero Left Rail Read More' Link is visible
     *
     * @return true if the read more link is visible, false otherwise
     */
    public boolean verifyProductHeroLeftRailReadMoreVisible() {
        return waitIsElementVisible(PRODUCT_HERO_LEFT_RAIL_READ_MORE_LOCATOR);
    }

    /**
     * Verify the 'Product Hero Info Block Icon' is visible
     *
     * @return true if the icon is visible, false otherwise
     */
    public boolean verifyProductHeroInfoBlockIconVisible() {
        return waitIsElementVisible(PRODUCT_HERO_INFO_BLOCK_ICON_LOCATOR);
    }

    /**
     * Verify GDP Header game title matches the expected title.
     *
     * @param gameTitle String to match
     * @return true if title matches, false otherwise
     */
    public boolean verifyGameTitle(String gameTitle) {
        return StringHelper.removeHighASCIICharacters(getGameName()).equalsIgnoreCase(StringHelper.removeHighASCIICharacters(gameTitle));
    }

    /**
     * Verify GDP Header game title matches the expected title.
     *
     * @param gameTitles List of Strings to match
     * @return true if any of the strings is a match, false otherwise
     */
    public boolean verifyGameTitle(List<String> gameTitles) {
        for (String gameTitle : gameTitles) {
            if (!StringHelper.containsIgnoreCase(getGameName(), gameTitle)) {
                return false;
            }
        }
        return true;
    }

    /**
     * Verify GDP Header infoblock title is to the right of the icon and matches the expected title.
     *
     * @return true if title is to the right and matches expected text, false otherwise
     */
    public boolean verifyProductHeroInfoBlockTitle(String subscriptionType) {
        return verifyInfoBlockTitleIsRightOfIcon()
                && verifyInfoBlockTitleAndSubscriptionLogo(subscriptionType);
    }

    /**
     * Verify GDP Header infoblock title matches the expected title and
     * subscription logo
     *
     * @return true if title contains expected keywords and subscription logo,
     * false otherwise
     */
    public boolean verifyInfoBlockTitleAndSubscriptionLogo(String subscriptonType) {
        boolean isSubscriptionLogoPresent = waitForElementVisible(PRODUCT_HERO_INFO_BLOCK_TITLE_LOGO_LOCATOR).getAttribute("src").contains(subscriptonType);
        boolean isSubscriptionLogoTitle = StringHelper.containsIgnoreCase(getGDPInfoBlockTitleText(), PRODUCT_HERO_INFO_BLOCK_TITLE_KEYWORDS);
        String logoUrl = waitForElementVisible(PRODUCT_HERO_INFO_BLOCK_TITLE_LOGO_LOCATOR).getAttribute("src");

        return isSubscriptionLogoTitle && verifyUrlResponseCode(logoUrl) && isSubscriptionLogoPresent;
    }

    /**
     * Verify GDP Header infoblock title is to the right of the icon.
     *
     * @return true if title is to the right, false otherwise
     */
    public boolean verifyInfoBlockTitleIsRightOfIcon() {
        return getGDPInfoBlockTitle().getLocation().getX() > getGDPInfoBlockIcon().getLocation().getX();
    }

    /**
     * Click GDP Header EA User Agreement link.
     */
    public void clickEAUserAgreement() {
        waitForElementClickable(PRODUCT_HERO_LEGAL_BLOCK_EA_USER_AGREEMENT).click();
    }
}
