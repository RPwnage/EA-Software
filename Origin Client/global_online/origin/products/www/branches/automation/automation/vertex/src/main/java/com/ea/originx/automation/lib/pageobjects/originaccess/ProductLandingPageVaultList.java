package com.ea.originx.automation.lib.pageobjects.originaccess;

import static com.ea.originx.automation.lib.pageobjects.originaccess.ProductLandingPageFooter.FOOTER_LOCATOR;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the vault list on the product landing page of a pre-order game
 *
 * @author caleung
 */
public class ProductLandingPageVaultList extends EAXVxSiteTemplate {

    protected static final String VAULT_LIST_TEMPLATE = "#storecontent origin-store-access-landing-vaultlist";
    protected static final By VAULT_LIST_LOCATOR = By.cssSelector(VAULT_LIST_TEMPLATE);
    protected static final By VAULT_LIST_SECTION_LOCATOR = By.cssSelector(VAULT_LIST_TEMPLATE + " section");
    protected static final By VAULT_LIST_GAMES_LOCATOR = By.cssSelector(VAULT_LIST_TEMPLATE + " .origin-store-access-landing-vaultlist "
                                   + "div.origin-store-access-landing-vaultlist-games div.origin-store-access-landing-vaultlist-game"); 
    protected static final By VAULT_LIST_HEADER_LOCATOR = By.cssSelector("div.origin-store-access-landing-vaultlist-content .otktitle-page");
    protected static final By VAULT_LIST_SHOW_MORE_BTN_LOCATOR = By.cssSelector(VAULT_LIST_TEMPLATE + " section button");
    protected static final By VAULT_LIST_BULLETS_LOCATOR =  By.cssSelector(" div.origin-store-access-landing-vaultlist-content ng-transclude ul li span");
    private static String SUBSCRIPTION_VAULT_GAME_FACET_GROUP = "subscriptionGroup:vault-games";
    
    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ProductLandingPageVaultList(WebDriver driver) {
        super(driver);
    }
    
    /**
     * Scroll to the 'Show More' button
     */
    public void scrollToShowMoreBtn() {
        scrollToElement(waitForElementPresent(VAULT_LIST_SHOW_MORE_BTN_LOCATOR));
    }
    
    /**
     * Verify if the bullet points are visible
     * @return true if the bullets are visible, false otherwise
     */
    public boolean verifyBulletPointsVisible() {
        List<WebElement> bulletPoints = driver.findElements(VAULT_LIST_BULLETS_LOCATOR);
        return bulletPoints.stream()
                .allMatch(b -> waitIsElementVisible(b));
    }
    
    /**
     * Scroll to the 'Vault List'
     */
    public void scrollToVaultList() {
        scrollToElement(waitForElementPresent(VAULT_LIST_SECTION_LOCATOR));
    }
    
    /**
     * Verify the 'Show More' button is visible
     * @return true if 'Show More' button is visible, false otherwise.
     */
    public boolean verifyShowMoreBtnVisible() {
        return waitIsElementVisible(VAULT_LIST_SHOW_MORE_BTN_LOCATOR);
    }

    /**
     * Verify the header of the vault list is visible
     * @return true if the header is visible
     */
    public boolean verifyHeaderVisible() {
        return waitIsElementVisible(VAULT_LIST_HEADER_LOCATOR);
    }
    /**
     * Click on the 'Show More' button
     */
    public void clickOnShowMoreBtn() {
        waitForElementClickable(VAULT_LIST_SHOW_MORE_BTN_LOCATOR).click();
    }
   
    /**
     * Get the games from the vault list
     * @return the number of games
     */
    public int getNumberOfGames() {
        List<WebElement> games = driver.findElements(VAULT_LIST_GAMES_LOCATOR);
        return games.size();
    }

    /**
     * Verify there are only vault games shown in the vault list
     *
     * @return true if only vault games are shown in the vault list, false otherwise
     */
    public boolean verifyOnlyVaultGamesInVaultList() {
        return waitForElementVisible(VAULT_LIST_LOCATOR).getAttribute("facet").equals(SUBSCRIPTION_VAULT_GAME_FACET_GROUP);
    }
}
