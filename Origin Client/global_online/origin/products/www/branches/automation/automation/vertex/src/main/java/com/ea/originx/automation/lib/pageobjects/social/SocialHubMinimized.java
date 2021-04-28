package com.ea.originx.automation.lib.pageobjects.social;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * The 'Social Hub' when it is minimized.
 * 
 * @author glivingstone
 */
public class SocialHubMinimized extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(SocialHubMinimized.class);

    protected By MINIMIZED_LOCATOR = By.cssSelector("div.origin-social-hub-area-minimized");
    protected By PRESENCE_LOCATOR = By.cssSelector(".origin-social-hub-area-minimized .otkpresence");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public SocialHubMinimized(WebDriver driver) {
        super(driver);
    }

    /**
     * Restore 'Social Hub' if minimized.
     */
    public void restoreSocialHub() {
        if (verifyMinimized()) {
            _log.debug("social hub minimized");
            forceClickElement(waitForElementPresent(MINIMIZED_LOCATOR));
            sleep(1000); // Let the animation complete.
        }
    }

    /**
     * Verify if 'Social Hub' is minimized.
     *
     * @return true if minimized, false otherwise
     */
    public boolean verifyMinimized() {
        return waitIsElementVisible(MINIMIZED_LOCATOR);
    }

    /**
     * Verify the current user's presence is the same as the given presence.
     *
     * @param presence The expected presence of the user (online, away, or
     * invisible; 'in-game' is not represented in the minimized social hub)
     * @return true if the user presence is the same as the given presence,
     * false otherwise
     */
    private boolean verifyPresence(String presence) {
        return StringHelper.containsIgnoreCase(waitForElementPresent(PRESENCE_LOCATOR).getAttribute("class"), presence);
    }

    /**
     * Verify the user presence is set to 'Invisible'.
     *
     * @return true if the user presence is 'Invisible', false otherwise
     */
    public boolean isUserInvisible() {
        return verifyPresence("invisible");
    }
}