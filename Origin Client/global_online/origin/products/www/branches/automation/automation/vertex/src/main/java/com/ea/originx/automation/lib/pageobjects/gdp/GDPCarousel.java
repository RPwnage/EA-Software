package com.ea.originx.automation.lib.pageobjects.gdp;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import static com.ea.originx.automation.lib.resources.OriginClientData.PRIMARY_CTA_BUTTON_COLOUR;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.support.Color;

/**
 * Page Object that represents a carousel placed in the GDP
 *
 * @author mdobre
 */
public class GDPCarousel extends EAXVxSiteTemplate {

    protected static final String GDP_CAROUSEL_TEMPLATE = "origin-store-pdp-section-wrapper[title-str='%s']";
    protected static final String GDP_CAROUSEL_TILES_CSS = GDP_CAROUSEL_TEMPLATE + " section li.origin-store-gdp-offercarousel-tile";
    protected static final String GDP_CAROUSEL_TILES_TITLES_CSS = GDP_CAROUSEL_TILES_CSS + " otkex-hometile";
    protected static final String GDP_CAROUSEL_LEFT_ARROW_CSS = GDP_CAROUSEL_TEMPLATE + " .origin-store-carousel-product-controls-left";
    protected static final String GDP_CAROUSEL_RIGHT_ARROW_CSS = GDP_CAROUSEL_TEMPLATE + " .origin-store-carousel-product-controls-right";
    protected static final String GDP_CAROUSEL_CLICKABLE_TILE_TMPL = ".origin-store-gdp-offercarousel-tile otkex-hometile[tile-header='%s'] div";
    private static final String GDP_CAROUSEL_INDICATORS_CSS = GDP_CAROUSEL_TEMPLATE + " .origin-store-carousel-product-indicator";

    private static final String CTA_EXPECTED_TEXT = "origin-cta-primary";

    protected final String title;
    protected final By carouselLocator;
    protected final By imageTilesLocator;
    protected final By imageTitlesLocator;
    protected final By leftArrowLocator;
    protected final By rightArrowLocator;
    protected final By listIndicators;

    public GDPCarousel(WebDriver driver, String title) {
        super(driver);
        this.title = title;
        this.carouselLocator = By.cssSelector(String.format(GDP_CAROUSEL_TEMPLATE, title));
        this.imageTilesLocator = By.cssSelector(String.format(GDP_CAROUSEL_TILES_CSS, title));
        this.imageTitlesLocator = By.cssSelector(String.format(GDP_CAROUSEL_TILES_TITLES_CSS, title));
        this.leftArrowLocator = By.cssSelector(String.format(GDP_CAROUSEL_LEFT_ARROW_CSS, title));
        this.rightArrowLocator = By.cssSelector(String.format(GDP_CAROUSEL_RIGHT_ARROW_CSS, title));
        this.listIndicators = By.cssSelector(String.format(GDP_CAROUSEL_INDICATORS_CSS, title));
    }

    /**
     * Scrolls into view the carousel
     */
    public void scrolltoGDPCarousel() {
        scrollElementToCentre(waitForElementPresent(carouselLocator));
    }

    /**
     * Waits for the carousel to load
     */
    public void waitForCarouselToLoad() {
        waitIsElementVisible(carouselLocator, 4);
    }

    /**
     * Gets all the indicators in a list
     *
     * @return the list with all the indicators
     */
    public List<WebElement> getAllIndicators() {
        return driver.findElements(listIndicators);
    }
    
    /**
     * Gets a list with all tiles
     * 
     * @return a list, representing all the images from the tiles
     */
    public List<WebElement> getAllTitles() {
        return driver.findElements(imageTitlesLocator);
    }

    /**
     * Gets the number of tiles from the carousel
     *
     * @return an int, representing the tiles from the carousel
     */
    public int getNumberOfTiles() {
        return driver.findElements(imageTilesLocator).size();
    }
    
    /**
     * Gets the title of a specific tile
     * @param numberOfTile the tile that will be inspected
     * 
     * @return a String, representing the tile's title 
     */
    public String getTitleOfTile(int numberOfTile) {
        return getAllTitles().get(numberOfTile).getAttribute("tile-header");
    }
    
    /**
     * Clicks on a specific tile
     * @param numberOfTile the tile that will be clicked
     */
    public void clickOnTile(int numberOfTile) {
        driver.findElements(imageTilesLocator).get(numberOfTile).click();
    }
    
    /**
     * Get selector for an entitlement 
     * 
     * @param entitlementName the name of the entitlement
     * @return the selector as By, representing the entitlement
     */
    public By getExpectedCarouselTile(String entitlementName) {
        return By.cssSelector(String.format(GDP_CAROUSEL_CLICKABLE_TILE_TMPL, entitlementName));
    }

    /**
     * Verifies the edition titles do not have duplicates
     *
     * @return true if thre are no duplicates, false otherwise
     */
    public boolean verifyUniqueEditionTitlesOnTiles() {
        List<String> titles = new ArrayList<>();
        int numberOfTiles = getNumberOfTiles();
        for (int i = 0; i < numberOfTiles; i++) {
            titles.add(getTitleOfTile(i));
        }
        Set<String> titlesSet = new HashSet<String>(titles);
        return (titles.size() == titlesSet.size());
    }

    /**
     * Verify there are no CTAs on the tiles by checking the HTML
     * of the element itself and all its children
     *
     * @return true if there are no CTAs, false otherwise
     */
    public boolean verifyNoCTADisplayed() {
        int numberOfTiles = getNumberOfTiles();
        for (int i = 0; i < numberOfTiles; i++) {
            String childElementContent = getAllTitles().get(i).getAttribute("innerHTML");
            if (StringHelper.containsIgnoreCase(childElementContent, CTA_EXPECTED_TEXT)) {
                return false;
            }
        }
        return true;
    }

    /**
     * Verifies if an indicator is checked
     *
     * @param indicatorNumber the number of the indicator which should be
     * verified
     * @return true if the indicator is checked, false otherwise
     */
    public boolean verifyIndicatorChecked(int indicatorNumber) {
        return  getAllIndicators().get(indicatorNumber - 1).getAttribute("class").contains("active");
    }

    /**
     * Clicks on the left arrow from the carousel
     */
    public void clickOnLeftArrow() {
        waitForElementClickable(leftArrowLocator).click();
        waitForAnimationComplete(listIndicators);
    }

    /**
     * Clicks on the right arrow from the carousel
     */
    public void clickOnRightArrow() {
        waitForElementClickable(rightArrowLocator).click();
        waitForAnimationComplete(listIndicators);
    }
    
    /**
     * Click on a specific carousel tile of an entitlement
     * 
     * @param entitlementName the name of the entitlement
     */
    public void clickOnSpecificTile(String entitlementName) {
        scrollElementIntoView(driver.findElement(getExpectedCarouselTile(entitlementName)));
        waitForElementClickable(getExpectedCarouselTile(entitlementName)).click();
    }
}