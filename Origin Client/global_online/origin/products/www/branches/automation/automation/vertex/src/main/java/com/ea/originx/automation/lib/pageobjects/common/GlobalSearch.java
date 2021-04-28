package com.ea.originx.automation.lib.pageobjects.common;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.Keys;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Global Search page object
 *
 * @author sbentley
 */
public class GlobalSearch extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(NavigationSidebar.class);

    protected static final By SEARCH_BOX_LOCATOR = By.cssSelector("origin-global-search .origin-globalsearch > label > div > input");
    protected static final By SEARCH_BOX_FOCUSED_LOCATOR = By.cssSelector("origin-global-search .origin-globalsearch-isfocused");
    protected static final By SEARCH_BOX_SEARCHING_LOCATOR = By.cssSelector("origin-global-search .origin-globalsearch-issearching");
    protected static final By SEARCH_BOX_CLEAR_LOCATOR = By.cssSelector("origin-global-search .origin-globalsearch > label > div .otkicon-closecircle");

    protected final WebElement searchBox;

    public GlobalSearch(WebDriver driver) {
        super(driver);
        searchBox = waitForElementVisible(SEARCH_BOX_LOCATOR);
    }

    /**
     * Tests to see if search box is empty and contains placeholder text. This
     * is done by location the search box, verifying the search box isn't in
     * focus (placeholder text does not appear when in focused) and verifying
     * that it is not currently searching.
     *
     * @return if search box is empty
     */
    public boolean verifyEmptyGlobalSearch() {
        String textValue = searchBox.getAttribute("value");
        return (textValue.isEmpty() && !verifyGlobalSearchFocused() && !verifyGlobalSearchSearching());
    }

    /**
     * Checks to see if Global Search is in searching state (prompt for
     * input/searching using entered text)
     *
     * @return true if Global Search is in searching state
     */
    public boolean verifyGlobalSearchSearching() {
        return waitIsElementVisible(SEARCH_BOX_SEARCHING_LOCATOR);
    }

    /**
     * Checks to see if Global Search is in focused state (clicked on)
     *
     * @return true if in focus state
     */
    public boolean verifyGlobalSearchFocused() {
        return waitIsElementVisible(SEARCH_BOX_FOCUSED_LOCATOR);
    }

    /**
     * Compares given string to see if string is in Global Search
     *
     * @param compareString given string to compare to what is in Global Search
     * @return true if given string is equal to string in Global Search
     */
    public boolean verifySearchBoxValue(String compareString) {
        String textValue = searchBox.getAttribute("value");
        return textValue.equals(compareString);
    }

    /**
     * Clears the text in the Global Search box.
     *
     * @param useXButton - Boolean to decide whether to clear text or use the
     * cancel button
     */
    public void clearSearchText(boolean useXButton) {
        if (useXButton) {
            _log.debug("clicking search X button");
            waitForElementClickable(SEARCH_BOX_CLEAR_LOCATOR).click();
        } else {
            _log.debug("clearing search box");
            waitForElementClickable(searchBox).clear();
        }
    }

    /**
     * Clears and enters the given text into the Global Search box and then
     * presses Return key.
     *
     * @param text - The text we want to send in the message.
     */
    public void enterSearchText(String text) {
        new NavigationSidebar(driver).closeSidebar();   //mobile
        _log.debug("entering search text: " + text);
        searchBox.clear();
        searchBox.sendKeys(text);
        searchBox.sendKeys(Keys.RETURN);
        waitForAngularHttpComplete();
    }

    /**
     * Enters/adds character into Global Search without deleting
     *
     * @param text the text to be entered
     */
    public void addTextToSearch(String text) {
        new NavigationSidebar(driver).closeSidebar();   //mobile
        _log.debug("entering search text: " + text);
        searchBox.sendKeys(text);
        waitForAngularHttpComplete();
    }

    /**
     * Deletes a single character in Global Search using Backspace
     */
    public void deleteLetterByBackspace() {
        new NavigationSidebar(driver).closeSidebar();   //mobile
        searchBox.sendKeys(Keys.BACK_SPACE);
        waitForAngularHttpComplete();
    }
    
    /**
     * This will forcefully click on the Global Search Box regardless of whether
     * it is clickable or not. This can be used to click off open dialogs.
     */
    public void actionClickGlobalSearch() {
        clickOnElement(searchBox);
    }

    /**
     * Opens Navigation Sidebar if close, scrolls to and clicks on Global Search
     * to focus on it
     */
    public void clickOnGlobalSearch() {
        NavigationSidebar navBar = new NavigationSidebar(driver);
        navBar.openSidebar();
        navBar.scrollToElement(searchBox);
        waitForElementClickable(searchBox).click();
    }
}
