package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * A page object representing the 'Search for Friends' dialog.
 *
 * @author lscholte
 */
public class SearchForFriendsDialog extends Dialog {

    protected static final By SEARCH_BOX_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-body .dialog-content-searchforfriends input");
    protected static final By SEARCH_BUTTON_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-footer .otkbtn-primary");
    protected static final String DIALOG_TITLE = "Search for friends";

    public SearchForFriendsDialog(WebDriver driver) {
        super(driver);
    }

    /**
     * Clicks the 'Search' button.
     */
    public void clickSearch() {
        waitForElementVisible(SEARCH_BUTTON_LOCATOR).click();
    }

    /**
     * Enters the specified text into the search bar.
     *
     * @param text The text to search for
     */
    public void enterSearchText(String text) {
        WebElement searchBox = waitForElementVisible(SEARCH_BOX_LOCATOR);
        searchBox.clear();
        searchBox.sendKeys(text);
    }
}