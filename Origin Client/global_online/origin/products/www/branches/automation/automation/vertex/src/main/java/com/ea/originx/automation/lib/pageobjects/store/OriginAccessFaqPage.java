package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object that represents the 'Origin Access FAQ' page.
 *
 * @author rocky
 */
public class OriginAccessFaqPage extends EAXVxSiteTemplate {

    public static final By ORIGIN_ACCESS_FAQ_TEXT_LOCATOR = By.cssSelector("#storecontent .l-origin-store-row .l-origin-store-row-content > h2");
    public static final By HOW_DO_I_JOIN_OA_LINK_LOCATOR = By.cssSelector("origin-store-accordionlist-item[topic*='How do I join'] div");
    public static final By ARROW_LOCATOR = By.cssSelector("i");
    public static final String ORIGIN_ACCESS_TITLE_TEXT[] = {"FAQ"};
    
    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OriginAccessFaqPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'Origin Access FAQ' title exists.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyFaqTitleMessage() {
        return StringHelper.containsAnyIgnoreCase(waitForElementVisible(ORIGIN_ACCESS_FAQ_TEXT_LOCATOR).getText(), ORIGIN_ACCESS_TITLE_TEXT);
    }
    
    /**
     * Click on 'How do I join Origin Access' link
     */
    public void clickHowDoIJoinOALink(){
        scrollElementToCentre(waitForElementVisible(HOW_DO_I_JOIN_OA_LINK_LOCATOR));
        waitForElementClickable(HOW_DO_I_JOIN_OA_LINK_LOCATOR).findElement(ARROW_LOCATOR).click();
    }
    
    /**
     * Click on 'Sign Up' link
     */
    public void clickHowDoIJoinOASignUpLink(){
        WebElement signUpLink = waitForChildElementVisible(HOW_DO_I_JOIN_OA_LINK_LOCATOR ,By.cssSelector("a"));
        scrollElementToCentre(signUpLink);
        signUpLink.click();
    }
}