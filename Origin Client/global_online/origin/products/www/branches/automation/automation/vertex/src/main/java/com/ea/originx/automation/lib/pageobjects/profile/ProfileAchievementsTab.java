package com.ea.originx.automation.lib.pageobjects.profile;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.WebDriver;

/**
 * Page object that represents the 'Achievements' tab on the 'Profile' page.
 *
 * @author lscholte
 */
public class ProfileAchievementsTab extends EAXVxSiteTemplate {

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ProfileAchievementsTab(WebDriver driver) {
        super(driver);
    }
}