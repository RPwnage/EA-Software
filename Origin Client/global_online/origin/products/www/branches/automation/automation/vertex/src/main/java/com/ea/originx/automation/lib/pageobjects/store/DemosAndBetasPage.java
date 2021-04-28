package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import java.util.List;
import java.util.stream.Collectors;

/**
 * Page object that represents the 'Demos and Betas' page.
 *
 * @author nvarthakavi
 */
public class DemosAndBetasPage extends EAXVxSiteTemplate {

    protected static final By DEMOS_AND_BETA_PAGE_LOCATOR = By.cssSelector(".l-origin-store-row-content.flex-box");
    protected static final String DEMO_AND_BETA_CARD_CSS = "origin-store-game-tile .origin-storegametile";
    private static final By DEMO_AND_BETA_CARDS_LOCATOR = By.cssSelector(DEMO_AND_BETA_CARD_CSS);
    protected static final String DEMO_AND_BETA_CARD_CSS_TMPL = DEMO_AND_BETA_CARD_CSS + " .origin-storegametile-overlaywrapper" +
            " .origin-storegametile-overlay .origin-storegametile-overlaycontent .origin-storegametile-cta[offer-id='%s']";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public DemosAndBetasPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify if the 'Demos and Betas' page is displayed.
     *
     * @return true if page displayed, false otherwise
     */
    public boolean verifyDemosAndBetasPageReached() {
        return waitIsElementVisible(DEMOS_AND_BETA_PAGE_LOCATOR);
    }

    /**
     * Get all the 'Demos and Betas' on the page.
     *
     * @return A list WebElements of all the 'Demos and Betas' tiles on the page
     */
    private List<WebElement> getAllDemoAndBetaElements() {
        return waitForAllElementsVisible(DEMO_AND_BETA_CARDS_LOCATOR);
    }

    /**
     * Get a list of all the 'Demo and Beta' tiles on this 'On the House' page.
     *
     * @return List of all 'Demos and Betas' tiles on this 'On the House' page
     */
    public List<DemoTile> getAllDemoAndBetaTiles() {
        List<DemoTile> demoTiles = getAllDemoAndBetaElements().
                stream().map(webElement -> new DemoTile(driver, webElement)).collect(Collectors.toList());
        return demoTiles;
    }

    /**
     * Wait for all the tiles to load.
     *
     * @return true if the tiles are stabilized, false otherwise
     */
    public boolean waitForDemoTilesToLoad(){
        return waitForAnimationComplete(DEMO_AND_BETA_CARDS_LOCATOR);
    }

    /**
     * Get a specific 'Demo and Beta' tile given an offer ID.
     *
     * @param offerId Entitlement offer ID
     * @return TrialTile for the given offer ID on this 'Free Games Trials' page, or throw
     * NoSuchElementException
     */
    public DemoTile getDemoAndBetaTile(String offerId) {
        return new DemoTile(driver, getDemoAndBetaTileElement(offerId));
    }

    /**
     * Get a specific 'Demo and Beta' tile WebElement given an offer ID.
     *
     * @param offerId Entitlement offer ID
     * @return 'Free Games Trial' tile WebElement for the given offer ID on this 'Free Games Trials' page
     * tab, or throw NoSuchElementException
     */
    private WebElement getDemoAndBetaTileElement(String offerId) {
        waitForElementPresent(By.cssSelector(String.format(DEMO_AND_BETA_CARD_CSS_TMPL, offerId))); //added this scroll as the element is hidden
        return driver.findElement(By.cssSelector(String.format(DEMO_AND_BETA_CARD_CSS_TMPL, offerId)));
    }
}