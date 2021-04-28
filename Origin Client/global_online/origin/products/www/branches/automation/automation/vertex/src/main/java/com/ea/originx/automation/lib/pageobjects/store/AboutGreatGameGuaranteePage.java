package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'About Great Game Guarantee' page.
 *
 * @author mkalaivanan
 */
public class AboutGreatGameGuaranteePage extends EAXVxSiteTemplate {

    public final static By ABOUT_GREAT_GAME_GUARANTEE_LOCATOR = By.cssSelector("#storecontent origin-store-informational[header-text*='Great Game Guarantee']");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public AboutGreatGameGuaranteePage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the header 'Great Game Guarantee' text is visible.
     *
     * @return true if text is visible, false otherwise
     */
    public boolean verifyAboutGreatGameGuaranteeTextVisible() {
        return waitIsElementVisible(ABOUT_GREAT_GAME_GUARANTEE_LOCATOR);
    }
}