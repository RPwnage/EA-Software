package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Page object that represents the 'Great Game Guarantee Policy' page.
 *
 * @author mkalaivanan
 */
public class GreatGameGuaranteePolicyPage extends EAXVxSiteTemplate {

    public static final By GREAT_GAME_GUARANTEE_POLICY_TEXT_LOCATOR = By.cssSelector("#storecontent origin-store-about-gggpolicy .otktitle-2");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public GreatGameGuaranteePolicyPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'Great Game Guarantee Policy' text is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyGreatGameGuaranteePolicyPageText() {
        return waitForElementVisible(GREAT_GAME_GUARANTEE_POLICY_TEXT_LOCATOR).getText().equalsIgnoreCase("Great Game Guarantee Policy");
    }
}