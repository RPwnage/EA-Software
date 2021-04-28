package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * The 'Hero Carousel' that appears at the top of the store page, below the MOTD.
 *
 * @author glivingstone
 */
public class HeroCarousel extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(HeroCarousel.class);

    protected final String HERO_CAROUSEL = "#storewrapper .origin-store-carousel-hero";

    protected final String INDICATORS_CONTAINER = HERO_CAROUSEL + " .origin-store-carousel-hero-indicators-container";
    protected final String INDICATORS = INDICATORS_CONTAINER + " .origin-store-carousel-hero-indicator";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public HeroCarousel(WebDriver driver) {
        super(driver);
    }

    /**
     * Select the given indicator in the list of carousel buttons on the bottom
     * of the carousel.
     *
     * @param select The banner indicator to select, starting at 0
     */
    public void selectIndicator(int select) {
        List<WebElement> buttons = waitForAllElementsVisible(By.cssSelector(INDICATORS));
        for (final WebElement button : buttons) {
            _log.debug(button.getAttribute("class"));
        }
        buttons.get(select - 1).click();
    }

    /**
     * Verify the 'Hero Carousel' exists on the page.
     *
     * @return true if the 'Hero Carousel' is found, false otherwise
     */
    public boolean verifyHeroCarousel() {
        return !waitForAllElementsVisible(By.cssSelector(HERO_CAROUSEL)).isEmpty();
    }

    /**
     * Verify the banner indicator is in the expected location.
     *
     * @param expected The expected location of the banner indicator, starting
     * at 0
     * @return true if the banner is at the expected location, false otherwise
     */
    public boolean verifyBannerIndicatorLocation(int expected) {
        List<WebElement> buttons = waitForAllElementsVisible(By.cssSelector(INDICATORS));
        for (WebElement button : buttons) {
            if (button.getAttribute("class").contains("active")) {
                return expected - 1 == buttons.indexOf(button);
            }
        }
        return false;
    }
}