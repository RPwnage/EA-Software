package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.NoSuchElementException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object that represents a 'Demo and Beta' page tile.
 *
 * @author nvarthakavi
 */
public class DemoTile extends EAXVxSiteTemplate {

    private final WebElement rootElement;

    // The following css selectors are for child elements within the rootElement
    protected static final By GET_IT_NOW_BUTTON_LOCATOR = By.cssSelector("origin-cta-directacquisition .origin-cta-primary .otkbtn.otkbtn-primary");
    protected static final String TILE_OFFERID_CSS = "origin-store-game-tile .origin-storegametile-overlaywrapper" +
            " .origin-storegametile-overlay .origin-storegametile-overlaycontent .origin-storegametile-cta";
    protected static final String TILE_OFFERID_CSS_TMPL = TILE_OFFERID_CSS + "[offer-id='%s']";

    /**
     * Constructor
     *
     * @param driver      Selenium WebDriver
     * @param rootElement Root element of this Trial tile
     */
    public DemoTile(WebDriver driver, WebElement rootElement) {
        super(driver);
        this.rootElement = rootElement;
    }

    /**
     * Get the offer ID of a 'Demo Tile'.
     *
     * @return Offer ID of current 'Demo and Beta' tile
     */
    public String getOfferId() {
        return rootElement.getAttribute("data-telemetry-label-offerid");
    }

    /**
     * Get the entitlement name of a 'Demo Tile'.
     *
     * @return Entitlement/display name of current 'Demo and Beta' tile.
     */
    public String getDemoEntitlementName() {
        return rootElement.getAttribute("display-name");
    }

    /**
     * Get a specific 'Demo and Beta' tile WebElement given the offer ID.
     *
     * @param offerId Entitlement offer ID
     * @return 'Demo Tile' WebElement for the given offer ID on this 'Demo and Beta' page
     * tab, or throw NoSuchElementException
     */
    private WebElement getDemoAndBetaTileElement(String offerId) {
        scrollToElement(waitForElementVisible(By.cssSelector(String.format(TILE_OFFERID_CSS_TMPL, offerId)))); //added this scroll as the element is hidden
        return driver.findElement(By.cssSelector(String.format(TILE_OFFERID_CSS_TMPL, offerId)));
    }

    /**
     * Get a specific 'Demo and Beta' tile given the offer ID.
     *
     * @param offerId Entitlement offer ID
     * @return 'Demo Tile' for the given offer ID on this 'Demo and Beta' page, or throw
     * NoSuchElementException
     */
    public DemoTile getDemoAndBetaTile(String offerId) {
        return new DemoTile(driver, getDemoAndBetaTileElement(offerId));
    }
    /**
     * Verify the offer ID of a 'Demo Tile' matches the given offer ID.
     *
     * @param offerId Expected offer ID
     * @return true if this 'Demo and Beta' tile has matching offer ID, false otherwise
     */
    public boolean verifyOfferId(String offerId) {
        return getOfferId().equalsIgnoreCase(offerId);
    }

    /**
     * Get the child element given it's locator.
     *
     * @param childLocator Child locator within rootElement of this 'Demo and Beta' tile
     * @return Child WebElement if found, or throw NoSuchElementException
     */
    private WebElement getChildElement(By childLocator) {
        return rootElement.findElement(childLocator);
    }

    /**
     * Get the child element's text given it's locator.
     *
     * @param childLocator Child locator within rootElement of this 'Demo and Beta' tile.
     * @return Child WebElement text if found, or null if not found
     */
    private String getChildElementText(By childLocator) {
        try {
            return rootElement.findElement(childLocator).getAttribute("textContent");
        } catch (NoSuchElementException e) {
            return null;
        }
    }

    /**
     * Click child WebElement as specified by the child locator.
     *
     * @param childLocator Child locator within rootElement of this 'Demo and Beta' tile
     */
    private void clickChildElement(By childLocator) {
        waitForElementClickable(getChildElement(childLocator)).click();
    }

    /**
     * Verify the child element is visible given it's locator.
     *
     * @param childLocator Child locator within rootElement of this 'Demo and Beta' tile
     * @return true if childElement is visible, false otherwise
     */
    private boolean verifyChildElementVisible(By childLocator) {
        return waitIsElementVisible(getChildElement(childLocator));
    }

    /**
     * Scroll to child WebElement and hover on it.
     *
     * @param childLocator Child locator within rootElement of this 'Demo and Beta' tile
     */
    private void scrollToAndHoverOnChildElement(By childLocator) {
        String offerId = getOfferId();
        WebElement childElement = waitForElementPresent(By.cssSelector(String.format(TILE_OFFERID_CSS_TMPL, offerId)));
        scrollToElement(childElement);
        sleep(500);
        hoverOnElement(childElement);
        sleep(500);
    }

    /**
     * Click on the 'Get It Now' button in the 'Demo and Beta' tile.
     */
    public void clickGetItNowButton() {
        clickChildElement(GET_IT_NOW_BUTTON_LOCATOR);
    }

    /**
     * Hover on the 'Get It Now' button in the 'Demo and Beta' tile.
     */
    public void hoverOnGetItNowButton() {
        scrollToAndHoverOnChildElement(GET_IT_NOW_BUTTON_LOCATOR);
    }
}