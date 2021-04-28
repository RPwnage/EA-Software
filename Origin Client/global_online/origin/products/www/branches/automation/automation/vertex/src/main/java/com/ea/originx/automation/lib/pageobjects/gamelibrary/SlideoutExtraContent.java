package com.ea.originx.automation.lib.pageobjects.gamelibrary;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.apache.logging.log4j.LogManager;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * A page object that represents the extra content for base game.
 *
 * @author rchoi
 */
public class SlideoutExtraContent extends EAXVxSiteTemplate {

    protected static final By EXPANSIONS_HEADER_LOCATOR = By.cssSelector(".content origin-gamelibrary-ogd-expansions h4.otktitle-2.origin-ogd-expansions-sectiontitle");
    protected static final String[] HEADER_NAMES = {"Expansions", "Add-Ons"};

    // Extra Content Tile CTA
    protected static final String BASEGAME_EXPANSION_CSS_TMPL = "div[offer-id='%s']";
    protected static final String BASEGAME_EXTRA_CONTENT_BUTTON_CSS_TMPL = BASEGAME_EXPANSION_CSS_TMPL + " .otkbtn.otkbtn-primary";
    protected static final String BASEGAME_EXTRA_CONTENT_VIOLATOR_TEXT_LOCATOR = BASEGAME_EXPANSION_CSS_TMPL + "otktitle-4 origin-gamecard-violator-text";

    // Extra Content Tile Description Area
    protected static final String TILE_SECONDARY_COL_TMPL = BASEGAME_EXPANSION_CSS_TMPL + " .origin-gamecard-secondarycol";
    protected static final String TILE_SHORT_DESCRIPTION_TMPL = TILE_SECONDARY_COL_TMPL + " strong[ng-bind-html*='shortDescription']";
    protected static final String TILE_MEDIUM_DESCRIPTION_TMPL = TILE_SECONDARY_COL_TMPL + " p[ng-if*='mediumDescription']";
    protected static final String TILE_LONG_DESCRIPTION_TMPL = TILE_SECONDARY_COL_TMPL + " div.origin-gamecard-long-description-container";
    protected static final By READ_MORE_LOCATOR = By.cssSelector("span.origin-gamecard-readmore");
    protected static final String GAMECARD_VIOLATOR_TEXT_TMPL = BASEGAME_EXPANSION_CSS_TMPL + " div.origin-gamecard-violator-text:not(.origin-gamecard-violator-debugtext)";
    private static final org.apache.logging.log4j.Logger _log = LogManager.getLogger(SlideoutExtraContent.class);

    public SlideoutExtraContent(WebDriver driver) {
        super(driver);
    }

    /**
     * Click the primary button for extra content.
     */
    public void clickExtraContentButton(String offerID) {
        WebElement extraContent = waitForElementVisible(By.cssSelector(String.format(BASEGAME_EXTRA_CONTENT_BUTTON_CSS_TMPL, offerID)));
        scrollToElement(extraContent);
        waitForElementClickable(extraContent).click();
    }

    /**
     * Verify if extra content for base game exists.
     *
     * @return true if extra content for base game exists,
     * false otherwise
     */
    public boolean verifyBaseGameExtraContent(String offerID) {
        return waitIsElementVisible(By.cssSelector(String.format(BASEGAME_EXPANSION_CSS_TMPL, offerID)));
    }

    /**
     * Verify if extra content for base game is unlocked after
     * purchasing.
     *
     * @return true if 'Unlocked' text for the violator is found,
     * false otherwise
     */
    public boolean verifyExtraContentIsUnlocked(String offerID) {
        return StringHelper.containsIgnoreCase(waitForElementVisible(By.cssSelector(String.format(BASEGAME_EXTRA_CONTENT_VIOLATOR_TEXT_LOCATOR, offerID))).getText(), "Unlocked");
    }

    /**
     * Verify if extra content for base game is downloadable after
     * purchasing.
     *
     * @return true if 'download' text for the button is found,
     * false otherwise
     */
    public boolean verifyExtraContentIsDownloadable(String offerID) {
        return StringHelper.containsIgnoreCase(waitForElementVisible(By.cssSelector(String.format(BASEGAME_EXTRA_CONTENT_BUTTON_CSS_TMPL, offerID))).getText(), "download");
    }

    /**
     * Verify if extra content for base game can be 'Play on Origin' after
     * purchasing, for browser use
     *
     * @return true if 'Play On Origin' text for the button is found,
     * false otherwise
     */
    public boolean verifyExtraContentCanBePlayOnOrigin(String offerID) {
        return StringHelper.containsIgnoreCase(waitForElementVisible(By.cssSelector(String.format(BASEGAME_EXTRA_CONTENT_BUTTON_CSS_TMPL, offerID))).getText(), "Play on Origin");
    }
    
    /**
     * Verify the 'Extra Content' tab is open and loaded.
     *
     * @return true if the 'Extra Content' tab is open,
     * false otherwise
     */
    public boolean isExtraContentOpen() {
        WebElement header = waitForElementVisible(EXPANSIONS_HEADER_LOCATOR);
        return StringHelper.containsAnyIgnoreCase(header.getText(), HEADER_NAMES);
    }

    /**
     * Verify the given expansion or addon has a short description.
     *
     * @param offerID The offer ID of the add-on or expansion
     * @return true if there is a short description that is visible,
     * false otherwise
     */
    public boolean verifyHasShortDescription(String offerID) {
        WebElement description;
        try {
            description = waitForElementPresent(By.cssSelector(String.format(TILE_SHORT_DESCRIPTION_TMPL, offerID)), 2);
        } catch (TimeoutException e) {
            return false;
        }
        return description.isDisplayed();
    }

    /**
     * Verify the given expansion or addon has a medium description
     *
     * @param offerID The offer ID of the add-on or expansion
     * @return true if there is a medium description that is visible,
     * false otherwise
     */
    public boolean verifyHasMediumDescription(String offerID) {
        WebElement description;
        try {
            description = waitForElementPresent(By.cssSelector(String.format(TILE_MEDIUM_DESCRIPTION_TMPL, offerID)), 2);
        } catch (TimeoutException e) {
            return false;
        }
        return description.isDisplayed() && description.getText().length() > 0;
    }

    /**
     * Verifies the medium description is at least some length. There is no
     * restriction on how long a medium description could be.
     *
     * @param offerID The offer ID of the add-on or expansion to check the medium
     * description
     * @param minLength Minimum length for the description
     * @return true if the medium description is at least minLength characters
     * long, false otherwise
     */
    public boolean verifyMediumDescriptionLengthAtLeast(String offerID, int minLength) {
        WebElement description;
        try {
            description = waitForElementPresent(By.cssSelector(String.format(TILE_MEDIUM_DESCRIPTION_TMPL, offerID)));
        } catch (TimeoutException e) {
            return false;
        }
        return description.getText().length() > minLength;
    }

    /**
     * Verify there is no 'Read More' link on the 'Medium Description'.
     *
     * @param offerID The offer ID of the add-on or expansion
     * @return true if there is a short description that is visible, false
     * otherwise
     */
    public boolean verifyNoReadMoreMedium(String offerID) {
        WebElement description = waitForElementPresent(By.cssSelector(String.format(TILE_MEDIUM_DESCRIPTION_TMPL, offerID)));
        return !waitIsChildElementVisible(description, READ_MORE_LOCATOR, 2);
    }

    /**
     * Verify the given expansion or add-on has a long description.
     *
     * @param offerID The offer ID of the addon or expansion
     * @return true if there is a long description that is visible,
     * false otherwise
     */
    public boolean verifyHasLongDescription(String offerID) {
        WebElement description;
        try {
            description = waitForElementPresent(By.cssSelector(String.format(TILE_LONG_DESCRIPTION_TMPL, offerID)), 2);
        } catch (TimeoutException e) {
            return false;
        }
        return description.isDisplayed();
    }

    /**
     * Verify extra content is installed.
     *
     * @param offerID The offer ID of the add-on or expansion
     * @return true if extra content is installed, false otherwise
     */
    public boolean verifyExtraContentIsInstalled(String offerID) {
        WebElement element = waitForElementVisible(By.cssSelector(String.format(GAMECARD_VIOLATOR_TEXT_TMPL, offerID)));
        hoverOnElement(element);
        return StringHelper.containsIgnoreCase(element.getText(), "Installed");
    }
}