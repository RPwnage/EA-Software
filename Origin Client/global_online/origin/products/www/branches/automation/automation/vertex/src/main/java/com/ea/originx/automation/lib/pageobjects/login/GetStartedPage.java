package com.ea.originx.automation.lib.pageobjects.login;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Page object that represents 'Get Started' page that appears after successful
 * registration before the main origin window is launched.
 *
 * @author mkalaivanan
 */
public class GetStartedPage extends EAXVxSiteTemplate {

    protected static final By GET_STARTED_BUTTON_LOCATOR = By.cssSelector("#signIn");
    protected static final By GET_STARTED_PARENT_VERIFICATION_TEXT_LOCATOR = By.cssSelector("#createEndForm .otkc");
    protected static final String PARENT_VERIFICATION_EMAIL_KEYWORDS[] = {"parent", "guardian", "verification", "email"};

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public GetStartedPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Click on the 'Get Started' button.
     */
    public void clickGetStartedButton() {
        forceClickElement(waitForElementPresent(GET_STARTED_BUTTON_LOCATOR, 30));
    }

    /**
     * Wait for the page to load.
     */
    public void waitForPage() {
        Waits.waitForWindowThatHasElement(driver, GET_STARTED_BUTTON_LOCATOR, 35);
    }
    
    /**
     * Verify the 'Get Started' page is visible.
     *
     * @return true if the 'Get Started' page is visible, false otherwise
     */
    public boolean verifyOnGetStartedPage() {
        return waitIsElementVisible(GET_STARTED_BUTTON_LOCATOR);
    }

    /**
     * Verifies the text on page when creating an underage account
     *
     * @return true if the text matches the keywords, false otherwise
     */
    public boolean verifyParentEmailNotificationText() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(GET_STARTED_PARENT_VERIFICATION_TEXT_LOCATOR).getText(), PARENT_VERIFICATION_EMAIL_KEYWORDS);
    }
}