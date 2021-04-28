package com.ea.originx.automation.lib.pageobjects.gamelibrary;

import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.util.List;
import com.ea.vx.originclient.client.OriginClient;
import org.openqa.selenium.By;
import org.openqa.selenium.Keys;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Tests the dropdown filter menu in the 'Game Library'.
 *
 * @author rmcneil
 */
public class FilterMenu extends EAXVxSiteTemplate {

    protected static final String FILTER_PILL_TITLE_XPATH = "//origin-filter-pill//li//a[contains(text(),'%s')]";
    protected static final By FILTER_PILL_MULTIPLE_TITLE_LOCATOR = By.cssSelector("origin-filter-pill-multiple li.origin-filterbar-item a");
    protected static final String FILTER_MENU_CSS = "ul.origin-filter-list";
    protected static final String FILTER_MENU_ITEM_CSS_TMPL = FILTER_MENU_CSS + " origin-gamelibrary-filter-item[filter-id='%s'] origin-filter-option li";
    protected static final String FILTER_ITEM_DISABLED_CSS_TMPL = FILTER_MENU_ITEM_CSS_TMPL + ".origin-filter-item-isdisabled";
    protected static final By FILTER_MENU_LOCATOR = By.cssSelector(FILTER_MENU_CSS);
    protected static final By CLEAR_FILTER_LOCATOR = By.cssSelector(FILTER_MENU_CSS + " origin-filter-link li");
    protected static final By FILTER_INPUT_LOCATOR = By.cssSelector(".origin-filter-searchfield-input");
    protected static final By FILTER_PILLS_LOCATOR = By.cssSelector("origin-filter-pill > li.origin-filterbar-item-isactive");
    protected static final By FILTER_COUNT_LOCATOR = By.cssSelector("span[ng-if='activeFiltersCount']");
    protected static final By FILTER_TEXT_CLEAR_BUTTON_LOCATOR = By.cssSelector(".origin-filter-item .otkinput-filter .origin-filter-dismiss-icon");
    protected static final By FILTER_PILL_BUBBLE_LOCATOR = By.cssSelector("ul.origin-filterbar-container li.origin-filterbar-item");
    protected static final By FILTER_PILL_TEXT_LOCATOR = By.cssSelector("a.origin-filterbar-item-label");
    protected static final By FILTER_PILL_CLOSE_CIRCLE_LOCATOR = By.cssSelector("i.origin-filterbar-item-icon");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public FilterMenu(WebDriver driver) {
        super(driver);
    }

    /**
     * Enters the given text into the 'Filter' box.
     *
     * @param text The text we want to enter.
     */
    public void enterFilterText(String text) {
        WebElement SearchBox = waitForElementClickable(FILTER_INPUT_LOCATOR);
        SearchBox.clear();
        SearchBox.sendKeys(text);
        SearchBox.sendKeys(Keys.RETURN);
        waitForAngularHttpComplete();
    }

    /**
     * Verifies if the filter text is the game that was searched for
     *
     * @param filterName the game name
     * @return true if the filter text matches the game title, false otherwise
     */
    public boolean verifyFilterTextMatchesGame(String filterName) {
        return waitIsElementVisible(By.xpath(String.format(FILTER_PILL_TITLE_XPATH, filterName)));
    }

    /**
     * Clicks the 'Clear All' link to clear all existing filters.
     */
    public void clearAllFilters() {
        waitForElementClickable(CLEAR_FILTER_LOCATOR).click();
    }

    /**
     * Verifies there are no active filters.
     *
     * @return true if the filter menu is visible, false otherwise
     */
    public boolean isFilterMenuOpen() {
        return waitIsElementVisible(FILTER_MENU_LOCATOR);
    }

    /**
     * Set the filter to 'Installed' menu.
     */
    public void setInstalled() {
        clickMenu("isInstalled");
    }

    /**
     * Set the filter to 'Favorite' menu.
     */
    public void setFavorites() {
        clickMenu("isFavorite");
    }

    /**
     * Set the filter to 'Downloading' menu.
     */
    public void setDownloading() {
        clickMenu("isDownloading");
    }

    /**
     * Set the filter to 'Origin Access Vault games' menu.
     */
    public void setOriginAccessVaultGames() {
        clickMenu("isSubscription");
    }

    /**
     * Set the filter to 'Purchased Games' menu.
     */
    public void setPurchasedGames() {
        clickMenu("isPurchasedGame");
    }

    /**
     * Set the filter to 'Mac Game' menu.
     */
    public void setMacGame() {
        clickMenu("isMacGame");
    }

    /**
     * Set the filter to 'PC Game' menu.
     */
    public void setPCGame() {
        clickMenu("isPcGame");
    }

    /**
     * Click 'menuName' (to Filter games or clear the filter).
     *
     * @param menuName menu(filter-id) to click
     */
    private void clickMenu(String menuName) {
        By menuLocator = By.cssSelector(String.format(FILTER_MENU_ITEM_CSS_TMPL, menuName));
        WebElement menu = waitForElementVisible(menuLocator, 15); // Occassional timeout with default 5 secs. Increase to 15 secs
        waitForElementClickable(menuLocator, 15).click();
    }

    /**
     * Click 'menuName' (to Filter games or clear the filter).
     *
     * @param menuName menu(filter-id) to click
     */
    private void clickFilterBubble(String menuName) {
        By menuLocator = By.cssSelector(String.format(FILTER_MENU_ITEM_CSS_TMPL, menuName));
        WebElement menu = waitForElementVisible(menuLocator, 15); // Occassional timeout with default 5 secs. Increase to 15 secs
        scrollElementToCentre(menu);
        waitForElementClickable(menuLocator, 15).click();
    }

    /**
     * Assuming client is maximised
     * Checks if a given filter pill is visible.
     *
     * @param filterName The text we want to enter
     * @return true if the filter pill is visible, false otherwise
     */
    public boolean verifyFilterPillOpen(String filterName) {
        return waitIsElementVisible(By.xpath(String.format(FILTER_PILL_TITLE_XPATH, filterName)));
    }

    /**
     * Checks if the 'Purchased Games' filter pill is visible.
     *
     * @param text The text to search
     * @return true if the filter pill is visible, false otherwise
     */
    public boolean isFilterPillFilterByTextVisible(String text) {
        return verifyFilterPillOpen("Filter by: " + text);
    }

    /**
     * Assuming client is maximised
     * Checks if Premade filter "filterName" filter pill is visible.
     *
     * @return true if the filter pill is visible, false otherwise
     */
    public boolean verifyFilterPillMultipleVisible(String filterName) {
        boolean isFilterPillVisible = false;
        List<WebElement> filterBubbles = driver.findElements(FILTER_PILL_MULTIPLE_TITLE_LOCATOR);
        for (WebElement filterBubble : filterBubbles) {
            if (filterBubble.getText().equals(filterName)) {
                isFilterPillVisible = true;
                break;
            }
        }
        return isFilterPillVisible;
    }

    /**
     * Checks if 'PC' filter pill is visible
     *
     * @return true if 'PC' filter pill is visible
     */
    public boolean verifyPCFilterPillVisible() {
        return verifyFilterPillMultipleVisible("PC");
    }

    /**
     * Checks if 'Mac' filter pill is visible
     *
     * @return true if 'Mac' filter pill is visible
     */
    public boolean verifyMacFilterPillVisible() {
        return verifyFilterPillMultipleVisible("Mac");
    }

    /**
     * Checks if 'Installed' filter pill is visible
     *
     * @return true if 'Installed' filter pill is visible
     */
    public boolean verifyInstalledFilterPillVisible() {
        return verifyFilterPillMultipleVisible("Installed");
    }

    /**
     * Checks if 'Purchased games' filter pill is visible
     *
     * @return true if 'Purchased games' filter pill is visible
     */
    public boolean verifyPurchasedFilterPillVisible() {
        return verifyFilterPillMultipleVisible("Purchased games");
    }

    /**
     * Checks if 'Origin Access Vault games' filter pill is visible
     *
     * @return true if 'Origin Access Vault games' filter pill is visible
     */
    public boolean verifyOriginAccessVaultGamesFilterPillVisible() {
        return verifyFilterPillMultipleVisible("Origin Access Vault games");
    }

    /**
     * Verifies there are no active filters.
     *
     * @return true if no filter pills are visible, false otherwise
     */
    public boolean verifyNoActiveFilters() {
        List<WebElement> filterPills = driver.findElements(FILTER_PILLS_LOCATOR);
        return filterPills.isEmpty();
    }

    /**
     * Gets the number of currently applied filters.
     *
     * @return The number of currently applied filters
     */
    public int getFilterCount() {
        return Integer.parseInt(waitForElementVisible(FILTER_COUNT_LOCATOR).getText());
    }

    /**
     * Clear the 'Filter By Text' field by clicking the cross button.
     */
    public void clearTextField() {
        waitForElementClickable(FILTER_TEXT_CLEAR_BUTTON_LOCATOR).click();
    }

    /**
     * Verify the TextField is cleared.
     *
     * @return true if the TextField is cleared
     */
    public boolean verifyFilterTextFieldIsCleared() {
        return waitForElementVisible(FILTER_INPUT_LOCATOR).getText().contains("");
    }

    /**
     * Assuming client is maximised
     * Clear the 'Filter' pill bubble by clicking the 'X' Button.
     *
     * @param filter the text/name of the filter to be removed
     */
    public void clearFilterPillBubble(String filter) {
        List<WebElement> rootElements = driver.findElements(FILTER_PILL_BUBBLE_LOCATOR);

        for (WebElement rootElement : rootElements) {
            if (rootElement.findElement(FILTER_PILL_TEXT_LOCATOR).getText().contains(filter)) {
                waitForElementClickable(rootElement.findElement(FILTER_PILL_CLOSE_CIRCLE_LOCATOR)).click();
                break;
            }
        }
    }

    /**
     * Checks if the Premade Filter is Disabled
     *
     * @param menuName menu(filter-id)
     * @return true if Premade Filter is Disabled
     */
    public boolean verifyPremadeFilterDisabled(String menuName) {
        return waitIsElementVisible(By.cssSelector(String.format(FILTER_ITEM_DISABLED_CSS_TMPL, menuName)) , 15);
    }

    /**
     * Verifies 'Favorite' Filter is disabled
     *
     * @return true if Favorite filter is disabled
     */
    public boolean verifyFavoriteFilterDisabled() {
        return verifyPremadeFilterDisabled("isFavorite");
    }

    /**
     * Clear the 'Installed' filter pill bubble by clicking the 'X' Button.
     */
    public void clearFilterPillBubbleInstalled() {
        clearFilterPillBubble("Installed");
    }

    /**
     * Clear the 'PC' filter pill bubble by clicking the 'X' button.
     */
    public void clearFilterPillBubblePC() {
        clearFilterPillBubble("PC");
    }

    /**
     * Clear the 'Mac' filter pill bubble by clicking the 'X' button.
     */
    public void clearFilterPillBubbleMac() {
        clearFilterPillBubble("Mac");
    }
}
