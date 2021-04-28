package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import java.util.List;
import java.util.stream.Stream;

/**
 * Represents the 'Media Carousel' that appears on different store pages.
 *
 * @author esdecastro
 */
public class MediaCarousel extends EAXVxSiteTemplate {

    private static final By CAROUSEL_SLIDE_AREA_LOCATOR = By.cssSelector(".origin-carouselimage-slidearea.origin-store-carousel-carousel-slidearea");
    private static final By CAROUSEL_RIGHT_ARROW_LOCATOR = By.cssSelector(".origin-store-carousel-product-controls-right");
    private static final By CAROUSEL_LEFT_ARROW_LOCATOR = By.cssSelector(".origin-store-carousel-product-controls-left");
    private static final By POSITION_SELECTORS_LOCATOR = By.cssSelector("origin-store-pdp-media-carousel > div > ol.origin-store-carousel-product-indicators-container > li.origin-store-carousel-product-indicator");
    private static final By PILLS_LOCATOR = By.cssSelector(".origin-store-carousel-product-indicators-container>.origin-store-carousel-product-indicator");
    private static final By MEDIA_ITEM_LOCATOR = By.cssSelector("div.origin-carouselimage-container.origin-store-carousel-carousel-container > ul > li");
    private static final By IMAGE_ITEM_LOCATOR = By.cssSelector("div.origin-carouselimage-container.origin-store-carousel-carousel-container > ul > li > img");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public MediaCarousel(WebDriver driver) {
        super(driver);
    }

    /**
     * Clicks the i'th item (0 based) in the Screenshots and Videos Carousel.
     */
    public void openItemOnCarousel(int i) {
        //Need to differentiate the flow between if the Position Selectors are present or not
        // (ie. if the GDP has less than 2 images/videos)
        if (waitIsElementPresent(POSITION_SELECTORS_LOCATOR)) {
            // Wait for at least one position selector to be visible before we start
            waitForElementClickable(POSITION_SELECTORS_LOCATOR);
            WebElement carouselItem = getCarouselWebElement(i);
            // Wait for the scroll animation and click
            /*
             Move the carousel to the image.

             On the default screen size, there are 2 images per position, if you're
             here wondering why this doesn't work for you or are trying to get this
             working on mobile, this is probably why.
             */
            final int carouselPosition = i / 2;
            final List<WebElement> positionSelectors = driver.findElements(POSITION_SELECTORS_LOCATOR);
            waitForElementClickable(positionSelectors.get(carouselPosition)).click();
            // Wait for the scroll animation and click
            waitForElementClickable(carouselItem).click();
        } else {
            waitForElementPresent(CAROUSEL_SLIDE_AREA_LOCATOR);
            waitForElementClickable(getCarouselWebElement(i)).click();
        }
    }

    /**
     * Gets all non-empty items from the media carousel.
     *
     * @return All the non-empty items on the media carousel
     */
    private Stream<WebElement> getAllMediaOnCarousel() {
        final List<WebElement> allElements = driver.findElements(MEDIA_ITEM_LOCATOR);
        /*
         There are 4 types of elements on the carousel
        - Video elements
        - Empty placeholder video elements
        - Image elements
        - Empty placeholder image elements
        The video elements have a div, then a img tag with an src to youtube. The picture
        elements just have a dev > img with an src. The placeholder elements have the same
        structure, but don't have an src

        We want to return all the non empty elements. Since we don't want to have
        to check every element for non emptiness (the check is expensive and
        we'll probably only want the first or so elements), we return a lazy stream.
         */
        return allElements.stream()
                .filter(e -> !StringHelper.nullOrEmpty(e.findElement(By.cssSelector("img")).getAttribute("src")));

    }

    /**
     * To get the carouselWebElement at position i.
     *
     * @param i Position
     * @return The WebElement of the carousel item at position i
     */
    public WebElement getCarouselWebElement(int i) {
        final WebElement carouselItem = getAllMediaOnCarousel()
                .skip(i)
                .findFirst()
                .orElseThrow(() -> new RuntimeException("Cannot select media at"
                        + " position " + i + ". There are only "
                        + getAllMediaOnCarousel().count() + " positions"));
        return carouselItem;
    }

    /**
     * To get the i'th item (0 based) in the Screenshots and Videos Carousel's
     * Video-Id(source).
     *
     * @param i The position of the carousel item
     * @return The video-id/source-id of the carousel item
     */
    public String getVideoID(int i) {
        final WebElement carouselItem = getCarouselWebElement(i);
        return carouselItem.getAttribute("video-id");
    }

    /**
     * Verifies that the content is visible for a given carousel item
     *
     * @param position position of the carousel item
     * @return true if visible, false otherwise
     */
    public boolean verifyMediaModalContentVisible(int position) {
        return getCarouselWebElement(position).isDisplayed();
    }

    /**
     * Verifies that the media carousel has slided to the next page
     *
     * @return true if the page has slid, false otherwise
     */
    public boolean verifyPageSlide() {
        return !waitForElementVisible(CAROUSEL_SLIDE_AREA_LOCATOR).getAttribute("style").isEmpty();
    }

    /**
     * Clicks the right arrow on the carousel
     */
    public void clickRightArrow() {
        WebElement we = waitForAnyElementVisible(CAROUSEL_RIGHT_ARROW_LOCATOR);
        scrollToElement(we);
        we.click();
        sleep(500); // Wait for slide animation to complete
    }

    /**
     * Clicks the left arrow on the carousel
     */
    public void clickLeftArrow() {
        WebElement we = waitForAnyElementVisible(CAROUSEL_LEFT_ARROW_LOCATOR);
        scrollToElement(we);
        we.click();
        sleep(500); // Wait for slide animation to complete
    }

    /**
     * Gets the number of pages that the carousel has
     *
     * @return the number of pages the carousel has
     */
    private int getNumberOfPages() {
        int pages = driver.findElements(PILLS_LOCATOR).size();
        if(pages < 2){
            return 1;
        }
        return pages;
    }


    /**
     * Clicks the first image item by scrolling right through the carousel until an image item is visible.
     */
    public void clickFirstImageItem() {
        WebElement firstItem = waitForElementPresent(IMAGE_ITEM_LOCATOR);
        if(firstItem != null) {
            for (int i = 0; i < getNumberOfPages(); i++) {
                if (firstItem.isDisplayed()) {
                    firstItem.click();
                    return;
                }
                clickRightArrow();
            }
        }
        _log.error("Could not find the item within the media carousel");
    }

}
