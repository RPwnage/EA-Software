package com.ea.originx.automation.lib.pageobjects.gdp;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import java.lang.invoke.MethodHandles;
import java.util.ArrayList;
import java.util.List;

/**
 * Page object that represents the 'GDP Navigation Bar' object. A GDPNavbar object contains 'Navigation'
 * links.
 *
 * @author tdhillon
 */
public class GDPNavBar extends EAXVxSiteTemplate {

    private static final String  NAVBAR_CSS = "#storecontent origin-store-pdp-nav .store-pdp-nav-bar";
    private static final By NAVBAR_LOCATOR = By.cssSelector(NAVBAR_CSS);

    private static final String NAVBAR_NAV_CSS = NAVBAR_CSS + " .otknavbar .otknav";
    private static final String NAV_LINKS_CSS = NAVBAR_NAV_CSS + " li.otkpill";
    private static final By NAV_LINKS_LOCATOR = By.cssSelector(NAV_LINKS_CSS + " a");

    private static final String NAV_LINK_PARENT_CSS_TMPL = NAV_LINKS_CSS + ":nth-of-type(%s)";
    private static final String NAV_LINK_CSS_TMPL = NAV_LINK_PARENT_CSS_TMPL + " a";

    private static final String NAV_MORE_DROPDOWN_CSS = NAVBAR_NAV_CSS + " > li.otkpill-overflow.otkdropdown";
    private static final String NAV_MORE_LINKS_CSS = NAV_MORE_DROPDOWN_CSS + " li";
    private static final By NAV_MORE_LINKS_LOCATOR = By.cssSelector(NAV_MORE_LINKS_CSS + " a");


    private static final String NAV_MORE_LINK_CSS_TMPL = NAV_MORE_LINKS_CSS + ":nth-of-type(%s) a";
    private static final By NAV_MORE_DROPDOWN_BUTTON_LOCATOR = By.cssSelector(NAV_MORE_DROPDOWN_CSS + " a");

    private static final By NAV_PINNED_LOCATOR = By.cssSelector(".store-pdp-nav-bar-pinned");
    private static final By NAV_BUY_CTA_LOCATOR = By.cssSelector(".otkbtn-dropdown .otkbtn-primary-btn");
    private static final By PACK_ART_LOCATOR = By.cssSelector("div.store-pdp-nav-mini-poster img");

    private List<GDPNavBar.GDPNavLink> navLinks = null; // List of navigation links each with an index and a label

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Inner class representing a GDP 'Navigation' link object.
     */
    private class GDPNavLink {

        int index; // link index (starting from 1)
        String label; // link label

        By linkLocator; // to locate link when visible on the navigation bar
        By linkParentLocator; // to locate link's parent - to check if link is hidden under the 'More' dropdown menu
        By moreLinkLocator; // to locate link when hidden under the 'More' dropdwon menu

        /**
         * Constructor - GDPNavLink a navigation link
         *
         * @param index Index of the GDP Navigation Link within the Navigation
         * Bar (starting from 1)
         * @param label Text label of the link (Configuration, can be different
         * for different offer)<br>
         * Can include: Screenshots and Videos, Compare Editions, Overview,
         * Feature, Label, System Requirements, Related Products, ...
         */
        public GDPNavLink(int index, String label) {
            this.index = index;
            this.label = label;
            this.linkLocator = By.cssSelector(String.format(NAV_LINK_CSS_TMPL, index));
            this.linkParentLocator = By.cssSelector(String.format(NAV_LINK_PARENT_CSS_TMPL, index));
            this.moreLinkLocator = By.cssSelector(String.format(NAV_MORE_LINK_CSS_TMPL, index));
        }

        /**
         * Get the index of the GDP 'Navigation' link within the 'GDP Navigation Bar'.
         * @return Index of the GDPNavLink within the GDPNavBar (starting from 1)
         */
        public int getIndex() {
            return index;
        }

        /**
         * Get the label of the GDP 'Navigation' link.
         *
         * @return Label of the GDPNavLink
         */
        public String getLabel() {
            return label;
        }

        /**
         * Get the locator for the link on the 'Navigation Bar'.
         *
         * @return Link locator
         */
        public By getLinkLocator() {
            return linkLocator;
        }

        /**
         * Get the locator for the link at the 'More' menu.
         *
         * @return Link locator under the 'More' dropdown menu
         */
        public By getMoreLinkLocator() {
            return moreLinkLocator;
        }

        /**
         * Check if the link is hidden at the navigation bar (if hidden, the
         * parent has class attribute "ng-hide").
         *
         * @return true if menu item hidden (under 'More') at the navigation
         * bar, false otherwise
         */
        private boolean isHidden() {
            WebElement parent = waitForElementPresent(linkParentLocator);
            return parent.getAttribute("class").contains("pdp-hide");
        }

        /**
         * Click on a link. If the link is hidden, click it at the 'More'
         * menu.
         */
        public void click() {
            if (isHidden()) {
                // access from the 'More' dropdown menu
                scrollElementToCentre(waitForElementVisible(NAV_MORE_DROPDOWN_BUTTON_LOCATOR));
                waitForElementClickable(NAV_MORE_DROPDOWN_BUTTON_LOCATOR).click();
                waitForElementClickable(getMoreLinkLocator()).click();
            } else {
                waitForElementClickable(getLinkLocator()).click();
            }
        }
    }  // End of inner class GDPNavLink

    /**
     * Constructor - GDPNavBar the navigation bar
     *
     * @param driver Selenium WebDriver
     */
    public GDPNavBar(WebDriver driver) {
        super(driver);
    }

    /**
     * Check if the 'GDP Navigation Bar' is visible.
     *
     * @return true if navigation bar is visible, false otherwise
     */
    public boolean isVisible() {
        return waitIsElementVisible(NAVBAR_LOCATOR);
    }

    /**
     * Re-populate navLinks, the private list of GDPNavLink's that stores the
     * index, label and derived locators of each link.
     */
    private void refreshNavLinks() {
        if (!isVisible()) {
            String errorMessage = "Error populating navLinks: GDP navigation bar is not visible";
            _log.error(errorMessage);
            throw new RuntimeException(errorMessage);
        }
        navLinks = new ArrayList<>(); // re-initialize
        List<WebElement> navLinkElements = driver.findElements(NAV_LINKS_LOCATOR); // include links not visible
        int index = 1;
        for (WebElement element : navLinkElements) {
            // Skip the last entry if it is 'More'
            String label = element.getAttribute("textContent"); // use getAttr so text retrieved even if element hidden
            if (index < navLinkElements.size() || !label.equalsIgnoreCase("More")) {
                GDPNavBar.GDPNavLink link = new GDPNavBar.GDPNavLink(index++, element.getAttribute("textContent"));
                navLinks.add(link);
            }
        }
    }

    /**
     * Populate if necessary navLinks - the private list of GDPNavLink's that
     * stores the index, label and derived locators of each link.<br>
     * Note: this method does not re-populate if already populated. Use method
     * refreshNavLinks() if force re-populate is needed.
     */
    private void populateNavLinks() {
        if (navLinks == null) {
            refreshNavLinks();
        }
    }

    /**
     * Get list of navigation link labels at the 'GDP Navigation Bar'.
     *
     * @return List of the navigation link labels
     */
    public List<String> getNavLinkLabels() {
        populateNavLinks();
        List<String> labels = new ArrayList<>();
        for (GDPNavBar.GDPNavLink link : navLinks) {
            String label = link.getLabel();
            labels.add(label);
        }
        return labels;
    }

    /**
     * Find GDPNavLink object with matching label.
     *
     * @param label Label to match
     * @return GDPNavLink with matching label, or null if not found
     */
    private GDPNavBar.GDPNavLink findNavLink(String label) {
        populateNavLinks();
        for (GDPNavBar.GDPNavLink link : navLinks) {
            if (link.getLabel().equalsIgnoreCase(label)) {
                return link;
            }
        }
        return null;
    }

    /**
     * Click the GDP 'Navigation' link with matching label.
     *
     * @param label Label of the navigation link to click
     */
    private void clickNavLink(String label) throws InterruptedException {
        populateNavLinks();
        findNavLink(label).click();
        Thread.sleep(2000); // Wait for scroll to finish
    }

    /**
     * Click the GDP 'Description' link
     *
     * @throws InterruptedException
     */
    public void clickDescriptionNavLink() throws InterruptedException {
        clickNavLink(GDPSections.DESCRIPTION_NAVBAR_LABEL);
    }

    /**
     * Click the GDP 'System Requirements' link
     *
     * @throws InterruptedException
     */
    public void clickSystemRequirements() throws InterruptedException {
        clickNavLink(GDPSections.SYSTEM_REQUIREMENTS_NAVBAR_LABEL);
    }

    /**
     * Gets PackArt webelement
     *
     * @return packart webelement
     */
    public WebElement getPackArt() {
        return waitForElementPresent(PACK_ART_LOCATOR);
    }

    /**
     * Gets URL of Packart
     *
     * @return url of Packart
     */
    public String getPackArtSrc() {
        return waitForElementPresent(PACK_ART_LOCATOR).getAttribute("src");
    }

    /**
     * Clicks the pack art on the nav bar.
     */
    public void clickPackArt(){
        waitForElementClickable(PACK_ART_LOCATOR).click();
    }

    /**
     * Verifies that the pack art is visible.
     *
     * @return true if the pack art is visible, false otherwise.
     */
    public boolean verifyPackArtIsVisible(){
        return waitIsElementVisible(PACK_ART_LOCATOR);
    }

    /**
     * Verifies that the GDP nav bar is pinned
     *
     * @return true if nav bar is pinned, false otherwise
     */
    public boolean verifyNavBarIsPinned(){
        return waitIsElementVisible(NAV_PINNED_LOCATOR);
    }

    /**
     * Verifies that the buy CTA is visible
     *
     * @return true if the buy CTA is visible, false otherwise.
     */
    public boolean verifyBuyCtaIsVisible(){
        return waitIsElementVisible(NAV_BUY_CTA_LOCATOR);
    }
}