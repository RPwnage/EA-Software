package com.ea.originx.automation.lib.pageobjects.gdp;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.vx.pages.helpers.EnclosedElementCriteria;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object that represents the information sections on the GDP.
 * These sections appear below the GDP Hero.
 *
 * @author tdhillon
 */
public class GDPSections extends EAXVxSiteTemplate {

    public static final String DESCRIPTION_NAVBAR_LABEL = "DESCRIPTION";
    public static final String SYSTEM_REQUIREMENTS_NAVBAR_LABEL = "SYSTEM REQUIREMENTS";

    private static final By SYSTEM_REQUIREMENTS_SECTION_LOCATOR = By.cssSelector("origin-store-pdp-systemrequirements-wrapper section");
    private static final By DESCRIPTION_SECTION_LOCATOR = By.cssSelector("origin-store-gdp-description-wrapper section");

    private static final By MEDIA_LOCATOR = By.cssSelector("origin-store-pdp-media-carousel div.origin-carouselimage-container ul li");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public GDPSections(WebDriver driver) {
        super(driver);
    }

    private WebElement getSystemRequirementSection() {
        return waitForElementVisible(SYSTEM_REQUIREMENTS_SECTION_LOCATOR);
    }

    private WebElement getDescriptionSection() {
        return waitForElementVisible(DESCRIPTION_SECTION_LOCATOR);
    }

    /**
     * Verify that 'System Requirements' Section is in Viewport
     *
     * @return true if 'System Requirements' Section is in Viewport
     */
    public boolean verifySystemRequirementsSectionVisible() throws Exception {
        return rectangleElementWithinViewPort(getSystemRequirementSection(), EnclosedElementCriteria.TOP_LEFT);
    }

    /**
     * Verify that 'Description' Section is in Viewport
     *
     * @return true if 'Description' Section is in Viewport
     */
    public boolean verifyDescriptionSectionVisible() throws Exception {
        return rectangleElementWithinViewPort(getDescriptionSection(), EnclosedElementCriteria.TOP_LEFT);
    }

    //////////////////////////////////////////////
    // Media Carousel
    //////////////////////////////////////////////

    /**
     * Verifies that the Media Module is visible
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyMediaModuleVisible(){
        return waitIsElementVisible(MEDIA_LOCATOR);
    }
}
