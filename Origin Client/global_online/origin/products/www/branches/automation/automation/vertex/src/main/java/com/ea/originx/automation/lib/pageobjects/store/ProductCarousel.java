package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object that represents the 'Product Carousel' on the 'Store' page.
 * A single row of products which may be browsed.
 *
 * @author glivingstone
 */
public class ProductCarousel extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(ProductCarousel.class);

    public ProductCarousel(WebDriver driver) {
        super(driver);
    }

    protected static final String PRODUCT_CAROUSEL = "#content #storecontent .origin-store-carousel-product-carousel";
    protected static final String CAROUSEL_TITLE = PRODUCT_CAROUSEL + " .otktitle-2";
    protected static final String CONTROLS = PRODUCT_CAROUSEL + " .origin-store-carousel-product-controls";
    protected static final String LEFT_CONTROL = CONTROLS + " .origin-store-carousel-product-controls-left";
    protected static final String RIGHT_CONTROL = CONTROLS + " .origin-store-carousel-product-controls-right";
    protected static final String CAROUSEL_TILE = PRODUCT_CAROUSEL + " .origin-gametile-item";
    protected static final String CAROUSEL_PRICE = CAROUSEL_TILE + " .origin-storeprice .otkprice";
    protected static final String CAROUSEL_PRICE_CHILD = ".otkprice";
    protected static final String CAROUSEL_IMAGE = CAROUSEL_TILE + " .origin-storegametile-posterart";
    protected static final String INNER_HTML_ATTRIBUTE = "innerHTML";
    /**
     * Navigate to the left on the 'Product Carousel'.
     */
    public void navigateLeft() {
        waitForElementClickable(By.cssSelector(LEFT_CONTROL)).click();
    }

    /**
     * Navigate to the right on the 'Product Carousel'.
     */
    public void navigateRight() {
        waitForElementClickable(By.cssSelector(RIGHT_CONTROL)).click();
    }

    /**
     * Click on the first non-free item in the 'Product Carousel'. We need to find
     * the first non free one, because clicking on a free one will initiate a
     * download and EULA, rather than go to a PDP.
     *
     * @return An integer of the position in the list you are clicking on
     * (beginning at 0)
     */
    public int clickFirstNonFree() {
        List<WebElement> allTiles = waitForAllElementsVisible(By.cssSelector(CAROUSEL_TILE));
        int index = -1;

        for (WebElement tile : allTiles) {
            List<WebElement> price = tile.findElements(By.cssSelector(CAROUSEL_PRICE_CHILD));
            if (!price.isEmpty() && !price.get(0).getAttribute(INNER_HTML_ATTRIBUTE).contains("$0.00")) {
                _log.debug(price.get(0).getAttribute(INNER_HTML_ATTRIBUTE));
                tile.click();
                index = allTiles.indexOf(tile);
                break;
            }
        }
        return index;
    }

    /**
     * Verify the 'Product Carousel' exists on the page.
     *
     * @return true if the product carousel exists, false otherwise
     */
    public boolean verifyProductCarousel() {
        return !waitForAllElementsVisible(By.cssSelector(PRODUCT_CAROUSEL)).isEmpty();
    }

    /**
     * Verify the title of the carousel matches what is expected.
     *
     * @param expectedTitle A string of the title we expect to appear.
     * @return true if the title on the page contains the expected text, false
     * otherwise
     */
    public boolean verifyTitleExists(String expectedTitle) {
        WebElement title = waitForElementPresent(By.cssSelector(CAROUSEL_TITLE));
        return title.getText().contains(expectedTitle);
    }

    /**
     * Verify the prices and images displayed on all the tiles in the carousel.
     *
     * @return true if all the items in the carousel have both prices and
     * images, false if any tile is missing any of those items
     */
    public boolean verifyItemsHavePricesAndImages() {
        return verifyItemsHavePrices() && verifyItemsHaveImages();
    }

    /**
     * Verify the prices exist on all the tiles in the pack
     *
     * @return true if all the items in the pack have prices, false if any of
     * the prices are missing.
     */
    public boolean verifyItemsHavePrices() {
        List<WebElement> allPrices = waitForAllElementsVisible(By.cssSelector(CAROUSEL_PRICE));
        boolean allHavePrices = true;

        for (WebElement price : allPrices) {
            _log.debug(price.getAttribute(INNER_HTML_ATTRIBUTE));
            if (!price.getAttribute(INNER_HTML_ATTRIBUTE).contains("$")) {
                allHavePrices = false;
                break;
            }
        }
        return allHavePrices && !allPrices.isEmpty();
    }

    /**
     * Verify the images exist on all the tiles in the pack.
     *
     * @return true if all the items in the pack have prices, false if any of
     * the prices are missing.
     */
    public boolean verifyItemsHaveImages() {
        List<WebElement> allImages = waitForAllElementsVisible(By.cssSelector(CAROUSEL_IMAGE));
        boolean allHaveImages = true;

        for (WebElement image : allImages) {
            _log.debug(image.getAttribute("src"));
            if (image.getAttribute("src").equals("") || image.getAttribute("src") == null) {
                allHaveImages = false;
                break;
            }
        }
        return allHaveImages && !allImages.isEmpty();
    }
}