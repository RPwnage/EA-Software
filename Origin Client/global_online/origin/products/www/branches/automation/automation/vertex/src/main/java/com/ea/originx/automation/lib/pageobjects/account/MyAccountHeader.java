package com.ea.originx.automation.lib.pageobjects.account;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Header of My Account (EADP) page object
 *
 * @author sbentley
 */
public class MyAccountHeader extends EAXVxSiteTemplate {

    protected static final By LOGOUT_BUTTON_LOCATOR = By.cssSelector("#logout");

    public MyAccountHeader(WebDriver driver) {
        super(driver);
    }

    // Clicks the logout button in My Account header
    public void clickLogout() {
        waitForElementClickable(LOGOUT_BUTTON_LOCATOR).click();
    }
}
