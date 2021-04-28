package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import org.openqa.selenium.By;
import org.openqa.selenium.NoSuchElementException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object that represents each tile on the 'Free Games Trials' page and 'Play First Trials' page.
 *
 * @author nvarthakavi
 */
public class TrialTile extends EAXVxSiteTemplate {

    private final WebElement rootElement;

    // The following css selectors are for child elements within the rootElement
    private static final By ADD_TO_GAME_LIBRARY_LOCATOR = By.cssSelector(".origin-storecta origin-cta-directacquisition .origin-cta-primary .otkbtn.otkbtn-primary");
    private static final String ADD_TO_GAME_LIBRARY_TEXT = "Add to Game Library";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     * @param rootElement Root element of this 'Trial' tile
     */
    public TrialTile(WebDriver driver, WebElement rootElement) {
        super(driver);
        this.rootElement = rootElement;
    }

    /**
     * Get the offer ID of the current 'Trial' tile.
     *
     * @return Offer ID of the current 'Trial' tile
     */
    public String getOfferId() {
        return rootElement.getAttribute("offer-id");
    }

    /**
     * Get the entitlement name of the current 'Trial' tile.
     *
     * @return Entitlement/Display name of current 'Trial' tile
     */
    public String getTrialEntitlementName() {
        return rootElement.getAttribute("display-name");
    }

    /**
     * Verify the offer ID of the current 'Trial' tile matches the given offer ID.
     *
     * @param offerId Expected offerId
     * @return true if this Trial tile has matching offer ID, false otherwise
     */
    public boolean verifyOfferId(String offerId) {
        return getOfferId().equalsIgnoreCase(offerId);
    }

    /**
     * Get the child element, given it's locator.
     *
     * @param childLocator Child locator within rootElement of this 'Trial' tile
     * @return Child WebElement if found, or throw NoSuchElementException
     */
    private WebElement getChildElement(By childLocator) {
        return rootElement.findElement(childLocator);
    }

    /**
     * Get the child element's text, given it's locator.
     *
     * @param childLocator Child locator within rootElement of this 'Trial' tile
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
     * @param childLocator Child locator within rootElement of this 'Trial' tile
     */
    private void clickChildElement(By childLocator) {
        waitForElementClickable(getChildElement(childLocator)).click();
    }

    /**
     * Verify the child element is visible, given it's locator.
     *
     * @param childLocator Child locator within rootElement of this 'Trial' tile
     * @return true if childElement is visible, false otherwise
     */
    private boolean verifyChildElementVisible(By childLocator) {
        return waitIsElementVisible(getChildElement(childLocator));
    }

    /**
     * Verifies that the 'Add to Game Library' CTA exists on the 'Trial' tile.
     *
     * @return true if it exists, false otherwise
     */
    public boolean verifyAddToGameLibraryCTAExists() {
        WebElement addToGameLibraryCTA;
        try {
            addToGameLibraryCTA = this.getChildElement(ADD_TO_GAME_LIBRARY_LOCATOR);
        } catch (NoSuchElementException e) {
            return false;
        }
        boolean isCTAVisible = addToGameLibraryCTA.isDisplayed();
        // Ensure that the text has changed from 'Learn More' to 'Add to Game Library'
        boolean isCTATextCorrect = addToGameLibraryCTA.getText().equals(ADD_TO_GAME_LIBRARY_TEXT);
        return isCTAVisible && isCTATextCorrect;
    }

    /**
     * Scroll to child WebElement and hover on it.
     *
     * @param childLocator Child locator within rootElement of this 'Trial' tile
     */
    private void scrollToAndHoverOnChildElement(By childLocator) {
        WebElement childElement = waitForElementVisible(getChildElement(childLocator));
        scrollToElement(childElement);
        sleep(500);
        hoverOnElement(childElement);
        sleep(500);
    }

    /**
     * Click on the 'Get It Now' button in the 'Trial' tile.
     */
    public void clickGetItNowButton() {
        clickChildElement(ADD_TO_GAME_LIBRARY_LOCATOR);
    }

    /**
     * Hover on the 'Get It Now' button in the 'Trial' tile.
     */
    public void hoverOnGetItNowButton() {
        scrollToAndHoverOnChildElement(ADD_TO_GAME_LIBRARY_LOCATOR);
    }

    /**
     * Get the 'Trial' tile non-trial entitlement name.
     *
     * @return The entitlement name of the tile
     */
    public String getEntitlementName() {
        return getTrialEntitlementName().replace("(Trial)", "").trim();
    }

    /**
     * Get the 'Trial' tile process name.
     *
     * @return The process name of the tile
     */
    public String getProcessName() {
        String processName = EntitlementInfoHelper.getProcessName(getEntitlementName());
        processName = processName.replace(".exe", "Trial.exe");
        return processName;
    }
}