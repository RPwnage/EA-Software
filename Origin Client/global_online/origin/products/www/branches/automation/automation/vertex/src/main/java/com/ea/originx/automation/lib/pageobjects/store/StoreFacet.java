package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.pageobjects.common.SystemMessage;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.lang.invoke.MethodHandles;
import java.util.ArrayList;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.StaleElementReferenceException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object that represents 'Store Facets' (filter menus and filters/options)
 * displayed on the right of the 'Store - Browse Games' page.
 *
 * @author rmcneil
 * @author palui
 */
public class StoreFacet extends EAXVxSiteTemplate {

    protected static final String STORE_FACET_CSS = ".origin-store-facets";

    // For checking if all facets loaded
    protected static final String FILTER_PANEL_VISIBLE_CSS = STORE_FACET_CSS + " .l-origin-filterpanelcontainer-panel-visible";
    protected static final By FILTER_PANEL_VISIBLE_LOCATOR = By.cssSelector(FILTER_PANEL_VISIBLE_CSS);

    // Menus
    protected static final String FILTER_MENUS_CSS = STORE_FACET_CSS + " li[role='presentation']";
    protected static final String FILTER_MENU_CSS_TMPL = FILTER_MENUS_CSS + "[label='%s']";
    protected static final String FILTER_MENU_CHILD_OPTION_CSS_TMPL = FILTER_MENU_CSS_TMPL + " li[label='%s']";
    protected static final String FILTER_MENU_LINK_CSS_TMPL = FILTER_MENU_CSS_TMPL + " h5 span";
    protected static final String FILTER_MENU_DOWNARROW_CSS_TMPL = FILTER_MENU_CSS_TMPL + " i";
    protected static final String FILTER_MENU_COLLAPSED_CSS_TMPL = FILTER_MENU_CSS_TMPL + " ul.origin-filter-list.ng-hide[role='menu']";

    // For checking if options list of a menu is visible
    protected static final String FILTER_MENU_OPTIONS_UL_CSS_TMPL = FILTER_MENU_CSS_TMPL + " ul";

    // 'Clear All' link
    protected static final String FILTER_CLEAR_ALL_LINK_CSS = STORE_FACET_CSS + " .origin-filtergroup-clearall-link";
    protected static final String FILTER_CLEAR_ALL_LINK_ENABLED_CSS = FILTER_CLEAR_ALL_LINK_CSS + ":not(.disabled)";
    protected static final By FILTER_CLEAR_ALL_LINK_LOCATOR = By.cssSelector(FILTER_CLEAR_ALL_LINK_CSS);
    protected static final By FILTER_CLEAR_ALL_LINK_ENABLED_LOCATOR = By.cssSelector(FILTER_CLEAR_ALL_LINK_ENABLED_CSS);

    // Options
    protected static final String FILTER_OPTIONS_CSS = STORE_FACET_CSS + " li.origin-filter-item.origin-filter-item-option[role='presentation']";
    protected static final String FILTER_OPTION_CSS_TMPL = FILTER_OPTIONS_CSS + "[label='%s']";
    protected static final String FILTER_OPTION_CHECKED_CSS_TMPL = FILTER_OPTION_CSS_TMPL + ".origin-filter-item-isactive";
    protected static final String FILTER_OPTION_HIDDEN_CSS_TMPL = FILTER_OPTION_CSS_TMPL + ".ng-hide";
    protected static final String FILTER_OPTION_VISIBLE_CSS_TMPL = FILTER_OPTION_CSS_TMPL + ":not(.ng-hide)";
    protected static final String FILTER_OPTION_CHECK_CIRCLE_CSS_TMPL = FILTER_OPTION_CSS_TMPL + " i.otkicon-checkcircle";

    // For searching options within a menu
    protected static final String FILTER_OPTIONS_CHILD_CSS = "li.origin-filter-item.origin-filter-item-option[role='presentation']";
    protected static final String FILTER_OPTIONS_VISIBLE_CHILD_CSS = FILTER_OPTIONS_CHILD_CSS + ":not(.ng-hide)";
    protected static final String FILTER_OPTIONS_HIDDEN_CHILD_CSS = FILTER_OPTIONS_CHILD_CSS + ".ng-hide";
    protected static final By FILTER_OPTIONS_CHILD_LOCATOR = By.cssSelector(FILTER_OPTIONS_CHILD_CSS);
    protected static final By FILTER_OPTIONS_VISIBLE_CHILD_LOCATOR = By.cssSelector(FILTER_OPTIONS_VISIBLE_CHILD_CSS);
    protected static final By FILTER_OPTIONS_HIDDEN_CHILD_LOCATOR = By.cssSelector(FILTER_OPTIONS_HIDDEN_CHILD_CSS);

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public StoreFacet(WebDriver driver) {
        super(driver);
    }

    /**
     * Wait for store facets to load.
     */
    public void waitForFacetsToLoad() {
        waitForElementVisible(FILTER_PANEL_VISIBLE_LOCATOR);
    }

    /**
     * Verify the sorting options are available
     *
     * @return true if the sorting expected options are displayed, false otherwise
     */
    public boolean verifySortOptions(List<String> expectedSortOptions) {
        List<String> actualSortOptions = getVisibleOptions("Sort");

        for (String option : expectedSortOptions) {
            if (!actualSortOptions.contains(option)) {
                return false;
            }
        }
        return true;
    }
    
    /**
     * Verify the facet is visible
     * 
     * @return true if the facet is displayed, false otherwise
     */
    public boolean verifyFacetVisible() {
        return waitIsElementVisible(FILTER_PANEL_VISIBLE_LOCATOR);
    }

    ////////////////////////////////
    // Menus
    ////////////////////////////////
    /**
     * Verify if a store facet menu is collapsed (not expanded).
     *
     * @param menuName Menu to verify
     * @return true if collapsed, false otherwise
     */
    public boolean isMenuCollapsed(String menuName) {
        return waitIsElementPresent(By.cssSelector(String.format(FILTER_MENU_COLLAPSED_CSS_TMPL, menuName)), 2);
    }

    /**
     * Collapse a store facet menu if needed.
     *
     * @param menuName Menu to collapse
     */
    public void collapseMenu(String menuName) {
        if (!isMenuCollapsed(menuName)) {
            clickMenu(menuName);
        }
    }

    /**
     * Expand a store facet menu if needed.
     *
     * @param menuName Menu to expand
     */
    public void expandMenu(String menuName) {
        if (isMenuCollapsed(menuName)) {
            clickMenu(menuName);
        }
    }

    /**
     * Click the store facet menu 'menuName' (to expand or collapse).
     *
     * @param menuName Menu to click
     * @param downArrow if true, click the downArrow to the right, otherwise
     * click the text link
     */
    private void clickMenu(String menuName, boolean downArrow) {
        String locatorTemplate = downArrow ? FILTER_MENU_DOWNARROW_CSS_TMPL : FILTER_MENU_LINK_CSS_TMPL;
        By menuLocator = By.cssSelector(String.format(locatorTemplate, menuName));
        waitForFacetsToLoad();
        WebElement menu = waitForElementVisible(menuLocator, 15); // Occassional timeout with default 5 secs. Increase to 15 secs
        scrollToElement(menu);
        SystemMessage siteStripe = new SystemMessage(driver);
        if (siteStripe.isSiteStripeVisible()) {
            new SystemMessage(driver).closeSiteStripe();
        }
        waitForElementClickable(menuLocator, 15).click();
    }

    /**
     * Click facet menu 'menuName' text label link (to expand or collapse).
     *
     * @param menuName Menu to click
     */
    public void clickMenu(String menuName) {
        clickMenu(menuName, false); // click the text link
    }

    /**
     * Click facet menu 'menuName' down arrow (to expand or collapse).
     *
     * @param menuName Menu to click
     */
    public void clickMenuDownarrow(String menuName) {
        clickMenu(menuName, true); // click the down arrow
    }

    ////////////////////////////////
    // Clear All link
    ////////////////////////////////
    /**
     * Check if 'Clear All' link is enabled.
     *
     * @return true if enabled, false otherwise
     */
    public boolean isClearAllLinkEnabled() {
        return waitIsElementVisible(FILTER_CLEAR_ALL_LINK_ENABLED_LOCATOR, 2);
    }

    /**
     * Click 'Clear All' link.
     */
    public void clickClearAllLink() {
        clickOptionWithRetries(FILTER_CLEAR_ALL_LINK_LOCATOR);
        new BrowseGamesPage(driver).waitForGamesToLoad(); // wait for games to reload after clicking link
    }

    ////////////////////////////////
    // Options
    ////////////////////////////////
    /**
     * Verify if an option has been checked (is active).
     *
     * @param menuName Menu of the option to verify
     * @param optionName Option to verify
     * @return true if checked/active, false otherwise
     */
    public boolean isOptionChecked(String menuName, String optionName) {
        expandMenu(menuName);
        return waitIsElementPresent(By.cssSelector(String.format(FILTER_OPTION_CHECKED_CSS_TMPL, optionName)), 2);
    }

    /**
     * Click facet option 'optionName' under facet menu 'menuName'.
     *
     * @param menuName Menu of the option to click
     * @param optionName Option to click
     */
    public void clickOption(String menuName, String optionName) {
        By optionLocator = By.cssSelector(String.format(FILTER_OPTION_CSS_TMPL, optionName));
        waitForFacetsToLoad();
        expandMenu(menuName);
        WebElement option = waitForElementVisible(optionLocator, 15); // Occassional timeout with default 5 secs. Increase to 15 secs
        scrollToElement(option);
        waitForElementClickable(optionLocator, 15).click();
        scrollToTop();
        // when used in conjunction with uncheckOption(), sometimes it will through StaleElement
        for (int i = 0; i < 5; i++) {
            try {
                new BrowseGamesPage(driver).waitForGamesToLoad(); // wait for Games to reload after clicking an option
            } catch (StaleElementReferenceException ex) {
                sleep(2000);
            }
        }

    }

    /**
     * Check an option (if not already checked).
     *
     * @param menuName Menu of the option to check
     * @param optionName Option to check
     */
    public void checkOption(String menuName, String optionName) {
        if (!isOptionChecked(menuName, optionName)) {
            clickOption(menuName, optionName);
        }
    }

    /**
     * Uncheck an option (if not already unchecked).
     *
     * @param menuName Menu of the option to uncheck
     * @param optionName Option to uncheck
     */
    public void uncheckOption(String menuName, String optionName) {
        if (isOptionChecked(menuName, optionName)) {
            clickOption(menuName, optionName);
            sleep(5000); // unchecking the option causes a page refresh and an overlay will block elements interaction
        }
    }

    /**
     * Check if the 'Check Circle' of an option is visible.
     *
     * @param menuName Menu of the option to click
     * @param optionName Option to click
     * @return true if 'Check Circle' is visible, false otherwise
     */
    public boolean isCheckCircleVisible(String menuName, String optionName) {
        expandMenu(menuName);
        return waitIsElementVisible(By.cssSelector(String.format(FILTER_OPTION_CHECK_CIRCLE_CSS_TMPL, optionName)), 2);
    }

    /**
     * Check if an option is visible
     *
     * @param menuName Menu of the option to click
     * @param optionName Option to click
     * @return true if option is visible, false otherwise
     */
    public boolean isOptionVisible(String menuName, String optionName) {
        expandMenu(menuName);
        return waitIsElementPresent(By.cssSelector(String.format(FILTER_OPTION_VISIBLE_CSS_TMPL, optionName)), 2);
    }
    
    /**
     * Check the given parameter is selected in the facet menu
     * 
     * @param optionName the selected parameter
     * @return true if the parameter is selected, false otherwise
     */
    public boolean isParameterSelected(String optionName) {
        return waitIsElementPresent(By.cssSelector(String.format(FILTER_OPTIONS_VISIBLE_CHILD_CSS, optionName)), 2);
    }

    /**
     * Check if an option is hidden.
     *
     * @param menuName Menu of the option to click
     * @param optionName Option to click
     * @return true if option is hidden, false otherwise
     */
    public boolean isOptionHidden(String menuName, String optionName) {
        expandMenu(menuName);
        return waitIsElementPresent(By.cssSelector(String.format(FILTER_OPTION_HIDDEN_CSS_TMPL, optionName)), 2);
    }

    /**
     * Scroll to an option.
     */
    public void scrollToOption(String menuName, String optionName) {
        By optionLocator = By.cssSelector(String.format(FILTER_OPTION_CSS_TMPL, optionName));
        waitForFacetsToLoad();
        expandMenu(menuName);
        WebElement option = waitForElementVisible(optionLocator, 15); // Occassional timeout with default 5 secs. Increase to 15 secs
        scrollToElement(option);
    }

    ////////////////////////////////
    // List of Options under a Menu
    ////////////////////////////////
    // Child locators for visible options only, hidden options only, or all options
    private static final By optionsChildLocator[] = {FILTER_OPTIONS_VISIBLE_CHILD_LOCATOR,
        FILTER_OPTIONS_HIDDEN_CHILD_LOCATOR,
        FILTER_OPTIONS_CHILD_LOCATOR};

    // For specifying type of options list to retrieve under a menu
    private enum OptionsListGetType {
        GET_VISIBLE(0), GET_HIDDEN(1), GET_ALL(2);

        private final int index;

        /**
         * Enum Constructor
         *
         * @param index Index of the option type to get
         */
        private OptionsListGetType(int index) {
            this.index = index;
        }

        /**
         * Get child locator for the get-type.
         *
         * @return Child locator for the get-type
         */
        private By getChildLocator() {
            return optionsChildLocator[index];
        }
    }

    /**
     * Get list of options under a menu depending on the getType.
     *
     * @param menuName Menu of the options to retrieve
     * @param getType Can be GET_VISIBLE, GET_HIDDEN, or GET_ALL
     * @return List<String> of options for the getType.<BR>
     * Return an empty list if the menu is collapsed, except for GET_ALL when
     * the complete list is returned regardless of whether the menu is expanded
     * or collapsed. Reason for this is because visibility indicator of an
     * option remains unchanged when a menu is collapsed.
     */
    private List<String> getOptions(String menuName, OptionsListGetType getType) {
        waitForFacetsToLoad();
        // Return empty list if menu is collapsed, except when we wish to get all options
        if (isMenuCollapsed(menuName) && getType != OptionsListGetType.GET_ALL) {
            return new ArrayList<>(0);
        }

        By menuLocator = By.cssSelector(String.format(FILTER_MENU_CSS_TMPL, menuName));
        WebElement menu = waitForElementVisible(menuLocator, 15); // Occassional timeout with default 5 secs. Increase to 15 secs
        scrollToElement(menu);

        By optionsByTypeLocator = getType.getChildLocator();
        List<WebElement> optionElements = menu.findElements(optionsByTypeLocator);
        List<String> result = new ArrayList<>();
        for (WebElement option : optionElements) {
            result.add(option.getAttribute("label"));
        }
        return result;
    }

    /**
     * Get list of visible options under a menu.
     *
     * @param menuName Menu of the options to retrieve
     * @return List<String> of visible options, or an empty list if the menu is
     * collapsed
     */
    public List<String> getVisibleOptions(String menuName) {
        return getOptions(menuName, OptionsListGetType.GET_VISIBLE);
    }

    /**
     * Get list of hidden options under a menu.
     *
     * @param menuName Menu of the options to retrieve
     * @return List of hidden options, or an empty list if the menu is
     * collapsed
     */
    public List<String> getHiddenOptions(String menuName) {
        return getOptions(menuName, OptionsListGetType.GET_HIDDEN);
    }

    /**
     * Get list of all options under a menu.
     *
     * @param menuName Menu of the options list to retrieve
     * @return List of all options, regardless of whether the menu is
     * collapsed or not, and whether the options are hidden or not
     */
    public List<String> getAllOptions(String menuName) {
        return getOptions(menuName, OptionsListGetType.GET_ALL);
    }

    /**
     * Verify if options of a given facet menu are visible.
     *
     * @param menuName Menu of the options to retrieve
     * @return true if options are visible, false otherwise
     */
    public boolean areOptionsVisible(String menuName) {
        return waitIsElementVisible(By.cssSelector(String.format(FILTER_MENU_OPTIONS_UL_CSS_TMPL, menuName)), 2);
    }

    /**
     * Attempts to click a facet within 5 retries
     *
     * @param by Locator of the element to click
     */
    private void clickOptionWithRetries(By by) {
        int attempts = 0;
        while (attempts < 5) {
            try {
                waitForElementClickable(by, 15).click();
            } catch (Exception ex) {
                _log.debug("Failed to click Option, retrying...");
            }
            attempts++;
            sleep(2000);
        }
    }

    /**
     * Check if an option is active or not on a given menu
     *
     * @param optionName the name of the option being checked
     * @return true if active, false otherwise
     */
    private boolean checkIfOptionIsActive(String menuName, String optionName) {
        WebElement optionElement = driver.findElement(By.cssSelector(String.format(FILTER_MENU_CHILD_OPTION_CSS_TMPL, menuName, optionName)));
        return optionElement.getAttribute("class").contains("origin-filter-item-isactive");
    }

    /**
     * Verify if active options are active and inactive options are inactive
     *
     * @param menuName the menu to get the options
     * @param activeOptions a list of active options
     * @return true if all the active options are active and inactive options are inactive, false if it fails.
     */
    public boolean verifyActiveOptionsAreActive(String menuName, List<String> activeOptions) {

        List<String> allOptions = getAllOptions(menuName);

        for (String options : allOptions) {
            if (activeOptions.contains(options) && !checkIfOptionIsActive(menuName, options)) {
                return false;
            } else if (!activeOptions.contains(options) && checkIfOptionIsActive(menuName, options)) {
                return false;
            }
        }

        return true;
    }

}