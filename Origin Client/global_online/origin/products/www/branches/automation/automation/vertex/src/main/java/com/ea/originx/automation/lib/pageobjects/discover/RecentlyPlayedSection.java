package com.ea.originx.automation.lib.pageobjects.discover;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * This handles the 'Quick Launch' (Recently Played) section of 'My Home'.
 *
 * @author sbentley
 */
public class RecentlyPlayedSection extends EAXVxSiteTemplate {

    protected static final String RECENTLY_PLAYED_SECTION_CSS = "origin-home-quick-launch";
    protected static final By RECENTLY_PLAYED_SECTION_LOCATOR = By.cssSelector(RECENTLY_PLAYED_SECTION_CSS);
    protected static final By QUICK_LAUNCH_TILES_LOCATOR = By.cssSelector(RECENTLY_PLAYED_SECTION_CSS + " otkex-hometile");

    public RecentlyPlayedSection(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify if the 'Quick Launch' area is visible.
     *
     * @return true if element visible, false otherwise
     */
    public boolean verifyRecentlyPlayedAreaVisisble() {
        return waitIsElementVisible(RECENTLY_PLAYED_SECTION_LOCATOR);
    }
}
