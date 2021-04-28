package com.ea.originx.automation.lib.pageobjects.common;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.JavascriptExecutor;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebDriverException;
import org.openqa.selenium.WebElement;
import java.lang.invoke.MethodHandles;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

/**
 * Represent a context menu object in an SPA. Each context has a drop down of
 * menu items. The drop down can be opened using left or right click
 *
 * @author palui
 */
public class ContextMenu extends EAXVxSiteTemplate {

    protected static final String DROPDOWN_TMPL = ".otkcontextmenu-wrap.otkcontextmenu-isvisible";
    protected static final By DROPDOWN_VISIBLE_LOCATOR = By.cssSelector(DROPDOWN_TMPL + ":not(.ng-hide)");
    protected static final By MENU_ITEMS_LOCATOR = By.cssSelector(DROPDOWN_TMPL + " [role='menuitem']");

    protected final By contextMenuLocator;
    protected final boolean leftClick;
    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor - default right click to open drop down menu
     *
     * @param driver Selenium WebDriver
     * @param contextMenuLocator locator for the context menu
     */
    public ContextMenu(WebDriver driver, By contextMenuLocator) {
        this(driver, contextMenuLocator, false);
    }

    /**
     * Constructor - specify left or right click to open drop down menu
     *
     * @param driver Selenium WebDriver
     * @param contextMenuLocator locator for the context menu
     * @param leftClick use left click to open the drop down
     */
    public ContextMenu(WebDriver driver, By contextMenuLocator, boolean leftClick) {
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
            try {
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
            } catch (WebDriverException e) {
                _log.debug("Error thrown trying to click on drop down");
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
     * Select a menu item on the drop down given the item text, or throw a run
     * time exception if item with matching text is not found
     *
     * @param itemText text of item to be selected
     */
    public void selectItem(String itemText) {
        openDropdown();
        WebElement item = getItem(itemText);
        if (item == null) {
            throw new RuntimeException(String.format("No item '%s' in menu containing items '%s'", itemText, getAllItemsText()));
        }
        item.click();
    }

    /**
     * Select a menu item on the drop down given the item text using JavascriptExecutor, or throw a run time exception
     * if item with matching text is not found
     * Use this method instead of slectItem method if selecting the menu item results 'Open Directory' dialog open
     *
     * @param itemText text of item to be selected
     */
    public void selectItemUsingJavaScriptExecutor(String itemText) {
        openDropdown();
        WebElement item = getItem(itemText);
        if (item == null) {
            throw new RuntimeException(String.format("No item '%s' in menu containing items '%s'", itemText, getAllItemsText()));
        }
        waitForElementClickable(item);
        JavascriptExecutor executor = (JavascriptExecutor) driver;
        executor.executeScript("var elem=arguments[0]; setTimeout(function() {elem.click();}, 100)", item);
        sleep(10000); //wait for the Open Directory window to appear
    }

    /**
     * Select a menu item with text matching one of those in itemTexts
     *
     * @param itemTexts list of menu items text to match
     */
    public void selectAnyItem(String... itemTexts) {
        openDropdown();
        WebElement item = getAnyItem(itemTexts);
        if (item == null) {
            throw new RuntimeException(String.format("No item '%s' in menu containing items '%s'", Arrays.toString(itemTexts), getAllItemsText()));
        }

        item.click();
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
     * Determine if an item with matching text is present in the context menu
     *
     * @param itemText text of menu item
     *
     * @return true if matching context menu item found, false otherwise
     */
    public boolean isItemPresent(String itemText) {
        openDropdown();
        return getItem(itemText) != null;
    }

    /**
     * Determine if an item containing text (case ignored) is present in the
     * context menu
     *
     * @param itemText text of menu item to match for containment
     *
     * @return true if context menu item (containing text with case ignored)
     * found, false otherwise
     */
    public boolean isItemPresentContainingIgnoreCase(String itemText) {
        openDropdown();
        return getItemContainingIgnoreCase(itemText) != null;
    }

    /**
     * Determine if all the items with matching text is displayed in the context menu
     *
     * @param items a list of menu items
     * @return true if all menu items are found within the context menu, false otherwise
     */
    public boolean verifyAllItemsVisible(String... items){
        return Arrays.stream(items).allMatch(item -> getItem(item) != null);
    }

    /**
     * Determine if an item in the context menu is disabled
     *
     * @param itemText text of menu item
     *
     * @return true if the context menu item is disabled, false otherwise
     */
    public boolean isItemDisabled(String itemText) {
        openDropdown();
        WebElement item = getItem(itemText);
        if (item == null) {
            throw new RuntimeException(String.format("No item '%s' in menu of items '%s'", itemText, getAllItemsText()));
        }
        return item.findElement(By.xpath("./..")).getAttribute("class").contains("otkmenu-item-disabled");
    }

    /**
     * Get WebElement of the drop down item with matching text
     *
     * @param itemText expected item text
     *
     * @return WebElement of the drop down item with matching text, or null if
     * not found
     */
    public WebElement getItem(String itemText) {
        openDropdown();
        waitForElementPresent(MENU_ITEMS_LOCATOR);
        List<WebElement> items = driver.findElements(MENU_ITEMS_LOCATOR);
        _log.debug(String.format("Looking for context menu item '%s'", itemText));
        for (WebElement item : items) {
            if (item.getText().equals(itemText)) {
                return item;
            }
        }
        _log.warn(String.format("No item '%s' in menu of items '%s'", itemText, items));
        return null;
    }

    /**
     * Get WebElement of the drop down item with text matching any one of the
     * itemTexts
     *
     * @param itemTexts array of expected item texts
     *
     * @return WebElement of the drop down item with matching text, or null if
     * not found
     */
    public WebElement getAnyItem(String... itemTexts) {
        openDropdown();
        waitForElementPresent(MENU_ITEMS_LOCATOR);
        List<WebElement> items = driver.findElements(MENU_ITEMS_LOCATOR);
        _log.debug(String.format("Looking for context menu item '%s'", Arrays.toString(itemTexts)));
        for (WebElement item : items) {
            if (StringHelper.containsAnyIgnoreCase(item.getText(), itemTexts)) {
                _log.debug("Found menu item with text: " + item.getText());
                return item;
            }
        }
        _log.warn(String.format("None of '%s' can be found in menu of items '%s'", Arrays.toString(itemTexts), items));
        return null;
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
