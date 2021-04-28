package com.ea.originx.automation.lib.pageobjects.discover;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * A page object that represents the 'New User Area'
 *
 * @author mkalaivanan
 */
public class NewUserArea extends EAXVxSiteTemplate {

    protected static final String NEW_USER_AREA_CSS = ".origin-message.origin-message-collapsible";
    protected static final By NEW_USER_AREA_LOCATOR = By.cssSelector(NEW_USER_AREA_CSS);
    protected static final By NEW_USER_AREA_COLLAPSIBLE_LOCATOR = By.cssSelector(NEW_USER_AREA_CSS + ".origin-message-collapsible");

    protected static final By NEW_USER_AREA_TITLE_LOCATOR = By.cssSelector(".origin-message-title");
    protected static final By NEW_USER_AREA_SUBTITLE_LOCATOR = By.cssSelector(".origin-message-content");
    protected static final By GOT_IT_BUTTON_LOCATOR = By.cssSelector(".otkbtn-nux");

    protected static final String EXPECTED_TITLE = "Welcome to your Origin home page";
    protected static final String EXPECTED_SUBTITLE = "Origin";

    public NewUserArea(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify 'New User Area' is visible.
     *
     * @return true if element visible, false otherwise
     */
    public boolean verifyNewUserAreaVisible() {
        WebElement newUserArea = waitForElementPresent(NEW_USER_AREA_LOCATOR);
        this.scrollToElement(newUserArea);
        return waitIsElementVisible(NEW_USER_AREA_LOCATOR);
    }

    /**
     * Verify title is displaying the correct message.
     *
     * @return true if expected text is found, false otherwise
     */
    public boolean verifyTitleCorrect() {
        return waitForElementVisible(NEW_USER_AREA_TITLE_LOCATOR).getText().contains(EXPECTED_TITLE);
    }

    /**
     * Verify subtitle is displaying the correct message.
     *
     * @return true if expected text is found, false otherwise
     */
    public boolean verifySubTitleCorrect() {
        return waitForElementVisible(NEW_USER_AREA_SUBTITLE_LOCATOR).getText().contains(EXPECTED_SUBTITLE);
    }

    /*
     * Click on the 'Got it' button.
     */
    public void clickGotItButton() {
        waitForElementClickable(GOT_IT_BUTTON_LOCATOR).click();
    }

    /**
     * Verify if the 'Got it' button is enabled.
     *
     * @return true if the 'Got It' button is enabled, false otherwise
     */
    public boolean verifyGotItButtonEnabled() {
        return isElementEnabled(GOT_IT_BUTTON_LOCATOR);
    }

    /**
     * Verify the 'New User Area' is closed.
     *
     * @return true if the 'New User Area' is closed, false otherwise
     */
    public boolean verifyNewUserClosed() {
        WebElement collapsible = waitForElementPresent(NEW_USER_AREA_COLLAPSIBLE_LOCATOR);
        String classAttribute = collapsible.getAttribute("class");
        return (classAttribute.contains("ng-hide") || classAttribute.contains("origin-message-collapsible-iscollapsed"));
    }
}