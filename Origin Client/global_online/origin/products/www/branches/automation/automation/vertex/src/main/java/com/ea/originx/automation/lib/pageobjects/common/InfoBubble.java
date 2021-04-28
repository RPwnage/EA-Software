package com.ea.originx.automation.lib.pageobjects.common;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the info bubble that appear on the free games page and on the
 * house page when you hover over the Get It Now button
 *
 * @author nvarthakavi
 */
public class InfoBubble extends EAXVxSiteTemplate {

    protected static final String INFO_BUBBLE_CSS = "div.origin-infobubble-content.otkc.otkc-small";
    protected static final String GAME_RATING_CSS = INFO_BUBBLE_CSS + " > div.origin-storegamerating.otkc-small";
    protected static final String GAME_LEGAL_CSS = INFO_BUBBLE_CSS + " > div.origin-storegamelegal";

    protected static final By INFO_BUBBLE_LOCATOR = By.cssSelector(INFO_BUBBLE_CSS);

    protected static final By GAME_RATING_LOGO_LOCATOR = By.cssSelector(GAME_RATING_CSS
            + " > a.origin-storegamerating-logo > img");

    protected static final By RATING_DESCRIPTOR_LOCATOR = By.cssSelector(GAME_RATING_CSS
            + " > div.origin-storegamerating-details > ul.origin-storegamerating-descriptors > li.origin-storegamerating-descriptor");

    protected static final By AVAILABILITY_DATE_LOCATOR = By.cssSelector(GAME_LEGAL_CSS
            + " > p.otkc-small.origin-storegamelegal-trial");

    protected static final By EULA_LOCATOR = By.cssSelector(GAME_LEGAL_CSS
            + " > div.origin-storegamelegal-eula");

    protected static final By TNC_LOCATOR = By.cssSelector(GAME_LEGAL_CSS
            + " > div.origin-storegamelegal-terms");

    protected static final By OTH_LOCATOR = By.cssSelector(GAME_LEGAL_CSS
            + " > div.origin-storegamelegal-oth");

    protected static final String[] EXPECTED_LIMITED_TIME_ONLY_AVAILABILITY_TIME_KEYWORDS = {"limited", "time","only"};

    protected static final String[] EXPECTED_AVAILABILITY_TIME_LENGTH_KEYWORDS = {"hours", "game", "time"};

    protected static final String[] EXPECTED_EULA_KEYWORDS = {"User", "Agreement"};

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public InfoBubble(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify if the 'Info Bubble' is visible
     *
     * @return true if element is visible, false otherwise
     */
    public boolean isInfoBubbleVisible() {
        return isElementVisible(INFO_BUBBLE_LOCATOR);
    }

    /**
     * To get 'Game Rating'
     *
     * @return the game rating of the entitlement or null if not found
     */
    public String getGameRating() {
        try {
            WebElement gameRating = waitForElementVisible(GAME_RATING_LOGO_LOCATOR);
            return gameRating.getAttribute("alt");
        } catch (TimeoutException e) {
            _log.error("Could not find Game Rating");
            return null;
        }
    }

    /**
     * Verify if the 'Game Rating' Logo is visible
     *
     * @return true if element is visible, false otherwise
     */
    public boolean isGameRatingLogoVisible() {
        return isElementVisible(GAME_RATING_LOGO_LOCATOR);
    }

    /**
     * Verify if the 'Game Rating' matches the local 'Game Rating' category
     *
     * @return true if element matches the condition , false otherwise
     */
    public boolean verifyGameRatingDescriptorMatchesLocale() {
        return waitForElementVisible(GAME_RATING_LOGO_LOCATOR).getAttribute("ng-src").contains("ESRB");
    }

    /**
     * Verify if the Rating Descriptors are visible
     *
     * @return true if the rating descriptors are visible, false otherwise
     */
    public boolean isRatingDescriptorsVisible() {
        return isElementVisible(RATING_DESCRIPTOR_LOCATOR);
    }

    /**
     * Verify if the 'Availability Date' is visible
     *
     * @return true if the date is visible, false otherwise
     */
    public boolean isAvailabilityDateVisible() {
        return isElementVisible(AVAILABILITY_DATE_LOCATOR);
    }

    /**
     * Verify if the 'EULA' is visible
     *
     * @return true if 'EULA' link is visible, false otherwise
     */
    public boolean isEULAVisible() {
        try {
            String eulaText = waitForElementVisible(EULA_LOCATOR).getText();
            return StringHelper.containsIgnoreCase(eulaText, EXPECTED_EULA_KEYWORDS);
        } catch (TimeoutException e) {
            _log.error("Unable to fine EULA text");
            return false;
        }
    }

    /**
     * Verify if the 'Terms and Conditions' is visible
     *
     * @return true if 'Terms and Conditions' link is visible, false otherwise
     */
    public boolean isTermsAndConditionsVisible() {
        try {
            return StringHelper.containsIgnoreCase(waitForElementVisible(TNC_LOCATOR).getText(), "Terms and Conditions");
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify if the 'Availability Date' say 'Limited Time Only'
     *
     * @return true if the date matches the keywords, false otherwise
     */
    public boolean isAvailabilityDateLimitedTime() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(AVAILABILITY_DATE_LOCATOR).getText(),EXPECTED_LIMITED_TIME_ONLY_AVAILABILITY_TIME_KEYWORDS);
    }

    /**
     * Verify if the 'Availability Date' say 'Limited Time Only'
     *
     * @return true if the date matches the keywords, false otherwise
     */
    public boolean isAvailabilityDateLength() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(AVAILABILITY_DATE_LOCATOR).getText(),EXPECTED_AVAILABILITY_TIME_LENGTH_KEYWORDS);
    }

    /**
     * Verify if the 'OTH' Link is visible
     *
     * @return true if 'OTH' Link is visible, false otherwise
     */
    public boolean isOTHLinkVisible() {
        try {
            return StringHelper.containsIgnoreCase(waitForElementVisible(OTH_LOCATOR).getText(), "On The House");
        } catch (TimeoutException e) {
            return false;
        }
    }
}
