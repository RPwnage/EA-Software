package com.ea.originx.automation.lib.pageobjects.common;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import static com.ea.originx.automation.lib.resources.OriginClientData.PRIMARY_CTA_BUTTON_COLOUR;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.support.Color;

/**
 * The representation of the carousels that appear on the 'My Home' page
 *
 * @author mdobre
 */
public class MyHomeCarousel extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(MyHomeCarousel.class);

    private static final String HOME_CAROUSEL_TEMPLATE = "origin-home-discovery-bucket[title-str='%s']";
    private static final String IMAGE_TILES_CSS = HOME_CAROUSEL_TEMPLATE + " .origin-tile-image";

    // Navigation
    private static final String INDICATORS_CSS = HOME_CAROUSEL_TEMPLATE + " .origin-store-carousel-product-indicators-container";
    private static final String RIGHT_ARROW_CSS = HOME_CAROUSEL_TEMPLATE + " .origin-home-carousel-control-item .otkicon-rightarrow";
    private static final String LEFT_ARROW_CSS = HOME_CAROUSEL_TEMPLATE + " .origin-home-carousel-control-item .otkicon-leftarrow";
    private static final String LIST_INDICATORS_CSS = INDICATORS_CSS + " li";

    protected final String title;
    protected final By carouselLocator;
    protected final By indicatorsLocator;
    protected final By imageTilesLocator;
    protected final By rightArrowLocator;
    protected final By leftArrowLocator;
    protected final By listIndicators;

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public MyHomeCarousel(WebDriver driver, String title) {
        super(driver);
        this.title = title;
        this.carouselLocator = By.cssSelector(String.format(HOME_CAROUSEL_TEMPLATE, title));
        this.indicatorsLocator = By.cssSelector(String.format(INDICATORS_CSS, title));
        this.imageTilesLocator = By.cssSelector(String.format(IMAGE_TILES_CSS, title));
        this.rightArrowLocator = By.cssSelector(String.format(RIGHT_ARROW_CSS, title));
        this.leftArrowLocator = By.cssSelector(String.format(LEFT_ARROW_CSS, title));
        this.listIndicators = By.cssSelector(String.format(LIST_INDICATORS_CSS, title));
    }

    /**
     * Verifies if carousels with more items exists by checking if the carousel
     * with the given title exists
     *
     * @return true if the carousel exists, false otherwise
     */
    public boolean verifyCarouselWithBiggerSizeExists() {

        return waitIsElementVisible(carouselLocator);
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
     * Verifies tiles have images attached
     *
     * @return true if images exists on all tiles of the carousel, false
     * otherwise
     */
    public boolean verifyTilesHaveImages() {

        List<WebElement> allImages = driver.findElements(imageTilesLocator);
        boolean allHaveImages = true;

        for (WebElement image : allImages) {
            boolean isUrlInsideImage = image.getAttribute("style").contains("url");
            String url = null;
            if (isUrlInsideImage) {
                int urlEndingPosition = image.getAttribute("style").toString().lastIndexOf("url(");
                url = image.getAttribute("style").toString().substring(urlEndingPosition);
            }
            if (image.getAttribute("style").equals("") || image.getAttribute("style") == null || url == null) {
                allHaveImages = false;
                break;
            }
        }
        return allHaveImages && !allImages.isEmpty();
    }

    /**
     * Verifies if an indicator is checked by looking into the color
     *
     * @param indicatorNumber the number of the indicator which should be
     * verified
     * @return true if the indicator is checked, false otherwise
     */
    public boolean verifyIndicatorChecked(int indicatorNumber) {
        return Color.fromString(waitForElementVisible(getAllIndicators().get(indicatorNumber - 1)).getCssValue("background-color")).asHex().equals(PRIMARY_CTA_BUTTON_COLOUR);
    }

    /**
     * Clicks on the given indicator
     *
     * @param indicatorNumber the indicator which should be clicked
     */
    public void clickOnIndicator(int indicatorNumber) {
        waitForElementClickable(getAllIndicators().get(indicatorNumber - 1)).click();
    }

    /**
     * Clicks on the right arrow from the carousel
     */
    public void clickOnRightArrow() {
        waitForElementClickable(rightArrowLocator).click();
        waitForAnimationComplete(indicatorsLocator);
    }

    /**
     * Clicks on the left arrow from the carousel
     */
    public void clickOnLeftArrow() {
        waitForElementClickable(leftArrowLocator).click();
        waitForAnimationComplete(indicatorsLocator);
    }
}
