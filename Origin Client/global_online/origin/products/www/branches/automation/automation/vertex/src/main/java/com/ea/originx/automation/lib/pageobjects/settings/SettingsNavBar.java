package com.ea.originx.automation.lib.pageobjects.settings;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.util.ArrayList;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object representing the 'Navigation Bar' of a 'Settings' page.
 *
 * @author palui
 */
public class SettingsNavBar extends EAXVxSiteTemplate {

    protected static final String NAV_CSS = "#content .otknavbar.origin-pills ul.otknav.otknav-pills";

    protected static final String NAV_LINKS_CSS = NAV_CSS + " > li.otkpill";
    protected static final By NAV_LINK_ITEMS_LOCATOR = By.cssSelector(NAV_LINKS_CSS + " > a");
    
    protected static final String VISIBLE_NAV_LINKS_CSS = NAV_LINKS_CSS + ":not(.ng-hide)";
    protected static final By VISIBLE_NAV_LINK_ITEMS_LOCATOR = By.cssSelector(VISIBLE_NAV_LINKS_CSS + " > a");

    protected static final String NAV_LINK_PARENT_CSS_TMPL = NAV_LINKS_CSS + ":nth-of-type(%s)";
    protected static final String NAV_LINK_CSS_TMPL = NAV_LINK_PARENT_CSS_TMPL + " > a";

    protected static final String NAV_MORE_DROPDOWN_CSS = NAV_CSS + " > li.otkpill-overflow.otkdropdown";
    protected static final String NAV_MORE_ITEMS_CSS_TMPL = NAV_MORE_DROPDOWN_CSS + " li[role='presentation']:nth-of-type(%s) > a";

    protected static final By NAV_MORE_BUTTON_LOCATOR = By.cssSelector(NAV_MORE_DROPDOWN_CSS + " > a");

    /**
     * Enum for notification toggle slide types (representing rows in the
     * Settings - Notifications page).
     */
    public enum NavLinkType {
        APPLICATION(1), DIAGNOSTICS(2), INSTALL_SAVE(3), NOTIFICATIONS(4), OIG(5), VOICE(6),
        APPLICATION_UNDERAGE(1), INSTALL_SAVE_UNDERAGE(2), NOTIFICATIONS_UNDERAGE(3), OIG_UNDERAGE(4);
        private final int index;

        /**
         * Constructor
         *
         * @param index Index of the Settings PageType
         */
        private NavLinkType(int index) {
            this.index = index;
        }

        /**
         * Get locator for the parent (the LI element) of this link type.
         *
         * @return Parent locator
         */
        private By getLinkParentLocator() {
            return By.cssSelector(String.format(NAV_LINK_PARENT_CSS_TMPL, index));
        }

        /**
         * Get locator for the link of this link type.
         *
         * @return Link locator
         */
        private By getLinkLocator() {
            return By.cssSelector(String.format(NAV_LINK_CSS_TMPL, index));
        }

        /**
         * Get locator for the link at the 'More' menu of this link type.
         *
         * @return Link locator
         */
        private By getMoreLinkLocator() {
            return By.cssSelector(String.format(NAV_MORE_ITEMS_CSS_TMPL, index));
        }
    }

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public SettingsNavBar(WebDriver driver) {
        super(driver);
    }

    /**
     * Wait for navigation links to be loaded.
     *
     * @return List of navigation link WebElements
     */
    public List<WebElement> getVisibleNavLinks() {
        return waitForAllElementsVisible(VISIBLE_NAV_LINK_ITEMS_LOCATOR);
    }

    /**
     * Get list of navigation link labels on the 'Settings' page.
     *
     * @return List of navigation links text on the 'Settings' page.
     */
    public List<String> getNavLinkLabels() {
        List<WebElement> navLinks = getVisibleNavLinks();
        List<String> labels = new ArrayList<>();
        for (WebElement link : navLinks) {
            String label = link.getText();
            labels.add(label);
        }
        return labels;
    }

    /**
     * Check if the link is hidden in the 'Navigation Bar' for this link type. (If
     * hidden, the parent has class attribute "ng-hide").
     *
     * @param type Link type corresponding to a menu link item on the 'Navigation
     * Bar'
     * @return true if menu item hidden (presumably under 'More') on the
     * Navigation Bar, false otherwise
     */
    private boolean verifyLinkHidden(NavLinkType type) {
        WebElement parent = waitForElementPresent(type.getLinkParentLocator());
        return parent.getAttribute("class").contains("ng-hide");
    }

    /**
     * Click on a link given its link type. If the link is hidden, click it
     * under the 'More' menu.
     *
     * @param type Link type corresponding to a menu link item on the 'Navigation
     * Bar'
     */  
    public void clickNavLink(NavLinkType type) {
        if (verifyLinkHidden(type)) {
            // Navigation link hidden due to low screen resolution
            // Click it at the 'More' menu
            waitForElementClickable(NAV_MORE_BUTTON_LOCATOR).click();
            waitForElementClickable(type.getMoreLinkLocator()).click();
        } else {
            waitForElementClickable(type.getLinkLocator()).click();
        }
    }

    /**
     * Click on the 'Applications' link.
     */
    public void clickApplicationLink() {
        clickNavLink(NavLinkType.APPLICATION);
    }
    
    /**
     * Click on the 'Applications' link. Use this method if logged in as an
     * underage user.
     */
    public void clickApplicationLinkUnderage() {
        clickNavLink(NavLinkType.APPLICATION_UNDERAGE);
    }

    /**
     * Click on the 'Diagnostics' link.
     */
    public void clickDiagnosticsLink() {
        clickNavLink(NavLinkType.DIAGNOSTICS);
    }

    /**
     * Click on the 'Installs & Saves' link.
     */
    public void clickInstallSaveLink() {
        clickNavLink(NavLinkType.INSTALL_SAVE);
    }
    
    /**
     * Click on the 'Installs & Saves' link. Use this method if logged in as an
     * underage user.
     */
    public void clickInstallSaveLinkUnderage() {
        clickNavLink(NavLinkType.INSTALL_SAVE_UNDERAGE);
    }

    /**
     * Click on the 'Notifications' link.
     */
    public void clickNotificationsLink() {
        clickNavLink(NavLinkType.NOTIFICATIONS);
    }
    
    /**
     * Click on the 'Notifications' link. Use this method if logged in as an
     * underage user.
     */
    public void clickNotificationsLinkUnderage() {
        clickNavLink(NavLinkType.NOTIFICATIONS_UNDERAGE);
    }

    /**
     * Click on the 'Origin In-Game' link.
     */
    public void clickOIGLink() {
        clickNavLink(NavLinkType.OIG);
    }
    
    /**
     * Click on the 'Origin In-Game' link. Use this method if logged in as an
     * underage user.
     */
    public void clickOIGLinkUnderage() {
        clickNavLink(NavLinkType.OIG_UNDERAGE);
    }

    /**
     * Click on the 'Voice' link.
     */
    public void clickVoiceLink() {
        clickNavLink(NavLinkType.VOICE);
    }
}