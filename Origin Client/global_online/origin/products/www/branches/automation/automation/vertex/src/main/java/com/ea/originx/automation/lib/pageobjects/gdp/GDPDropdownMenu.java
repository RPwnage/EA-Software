package com.ea.originx.automation.lib.pageobjects.gdp;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import static com.ea.vx.originclient.templates.OASiteTemplate.sleep;
import java.lang.invoke.MethodHandles;
import java.util.List;
import java.util.stream.Collectors;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represent the GDP drop down menu items. The drop down can be opened using
 * left or right click
 *
 * @author cdeaconu
 */
public class GDPDropdownMenu extends EAXVxSiteTemplate{
    
    protected static final String DROPDOWN_TMPL = ".otkex-dropdown-cta-contextmenu";
    protected static final By DROPDOWN_VISIBLE_LOCATOR = By.cssSelector(DROPDOWN_TMPL + "-visible");
    protected static final By MENU_ITEMS_LOCATOR = By.cssSelector(DROPDOWN_TMPL + " .otkex-dropdown-cta-submenuitem");

    protected final By contextMenuLocator;
    protected final boolean leftClick;
    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());
    
    /**
     * Constructor - default right click to open drop down menu
     *
     * @param driver Selenium WebDriver
     * @param contextMenuLocator locator for the context menu
     */
    public GDPDropdownMenu(WebDriver driver, By contextMenuLocator) {
        this(driver, contextMenuLocator, false);
    }
    
    /**
     * Constructor - specify left or right click to open drop down menu
     *
     * @param driver Selenium WebDriver
     * @param contextMenuLocator locator for the context menu
     * @param leftClick use left click to open the drop down
     */
    public GDPDropdownMenu(WebDriver driver, By contextMenuLocator, boolean leftClick) {
        super(driver);
        this.contextMenuLocator = contextMenuLocator;
        this.leftClick = leftClick;
    }
    
     /**
     *
     * @return true if WebElement to click to open/close the context menu is
     * visible, false otherwise
     */
    public boolean verifyContextMenuLocatorVisible() {
        return waitIsElementVisible(contextMenuLocator);
    }

    /**
     * Check if the drop down has been opened
     *
     * @return true if drop down has been opened and the menu items are visible,
     * false otherwise
     */
    public boolean verifyDropdownOpen() {
        return waitIsElementPresent(DROPDOWN_VISIBLE_LOCATOR, 2);
    }

    /**
     * Open the drop down if it is not already opened
     */
    public void openDropdown() {

        for (int i = 0; i <= 3; i++) {
            if (!verifyDropdownOpen()) {
                sleep(1000); // Opening dropdown too quickly may result in null items. Wait first
                if (leftClick) {
                    // Since we can have menus in the sidebar and selenium's default
                    // scroll does not work in the sidebar, we need to manually scroll
                    WebElement e = waitForElementClickable(contextMenuLocator);
                    scrollElementToCentre(e);
                    e.click();
                } else {
                    contextClick(waitForElementClickable(contextMenuLocator));
                }
            }
        }

        if (!verifyDropdownOpen()) { // wait for dropdown to open
            _log.error("Cannot open dropdown for context menu locator: " + contextMenuLocator.toString());
        }
    }

    /**
     * @return a list (String) of all menu items in the dropdown
     */
    public List<String> getAllItemsText() {
        openDropdown();
        return driver.findElements(MENU_ITEMS_LOCATOR).stream()
                .map(WebElement::getText)
                .collect(Collectors.toList());
    }

    /**
     * Select a menu item on the drop down menu given the item text (case
     * ignored), or throw a run time exception if item with matching text (case
     * ignored) is not found
     *
     * @param itemText text of menu item to be selected
     */
    public void selectItemContainingIgnoreCase(String itemText) {
        openDropdown();
        WebElement item = getItemContainingIgnoreCase(itemText);
        if (item == null) {
            throw new RuntimeException(String.format("No item '%s' in menu (case ignored) of items '%s'", itemText, getAllItemsText()));
        }
        waitForElementClickable(item).click();
    }

    /**
     * Get WebElement of the drop down item with matching text (case ignored)
     *
     * @param itemText expected item text (case ignored)
     *
     * @return first WebElement of the drop down items with matching text (case
     * ignored), or null if not found
     */
    public WebElement getItemContainingIgnoreCase(String itemText) {
        openDropdown();
        waitForElementPresent(MENU_ITEMS_LOCATOR);
        List<WebElement> items = driver.findElements(MENU_ITEMS_LOCATOR);
        _log.debug(String.format("Looking for context menu item '%s' (case ignored) ", itemText));
        for (WebElement item : items) {
            if (StringHelper.containsIgnoreCase(item.getText(), itemText)) {
                return item;
            }
        }
        _log.warn(String.format("No item '%s' (case ignored) in menu of items '%s'", itemText, items));
        return null;
    }
}
