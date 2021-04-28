package com.ea.originx.automation.lib.pageobjects.settings;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.utils.Waits;
import java.lang.invoke.MethodHandles;
import java.util.ArrayList;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.NoSuchElementException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * A page object that represents the 'Application Settings' page.
 *
 * @author palui
 * @author lscholte
 */
public class SettingsPage extends EAXVxSiteTemplate {

    protected static final String PAGE_SECTIONS_CSS = "#content .otknavbar section.origin-settings-tabbed-section.origin-settings-clearfix";

    protected static final By PAGE_SECTIONS_LOCATOR = By.cssSelector(PAGE_SECTIONS_CSS);

    protected static final String ACTIVE_SETTINGS_PAGE_CSS = "#content .otknavbar section.origin-settings-tabbed-section.origin-settings-clearfix.active-section";
    protected static final String ACTIVE_SETTINGS_PAGE_XPATH = "//div[@id='content']//nav[contains(@class, 'otknavbar')]//section[@class='origin-settings-tabbed-section origin-settings-clearfix active-section']";

    protected static final By ACTIVE_SETTINGS_PAGE_LOCATOR = By.cssSelector(ACTIVE_SETTINGS_PAGE_CSS);

    // Use the followings to locate a toggle slide given its sectionId and itemStr
    protected static final String ID_SECTION_XPATH_TMPL = ACTIVE_SETTINGS_PAGE_XPATH + "//SECTION[@id='%s']";
    protected static final String ITEMSTR_XPATH_TMPL = ID_SECTION_XPATH_TMPL + "//div[contains(@class, 'origin-settings-div-main') and contains(@itemstr,'%s')]";
    protected static final String DIV_SIDE_XPATH_TMPL = ITEMSTR_XPATH_TMPL + "/../origin-settings-slide-toggle//div[contains(@class, 'origin-settings-div-side')]";
    protected static final String TOGGLE_SLIDE_XPATH_TMPL = DIV_SIDE_XPATH_TMPL + "//div[contains(@class, 'otktoggle origin-settings-toggle')]";
    protected static final By NOTIFICATION_TOAST_DISPLAY_LOCATOR = By.cssSelector("#content #auto-save-toast .origin-toast-dialog.origin-toast-dialog-display");

    protected static final String SECTION_BY_NAME_XPATH_TMPL = "//SECTION[contains(@class, 'origin-settings-section') and .//H1[contains(@class, 'otktitle-2') and contains(text(), '%s')]";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());
    /**
     * Enum for settings sections, with expected ng-class attribute text string
     */
    private static final String SECTION_TEXT[] = {"application", "diagnostics", "installs", "notifications", "oig", "voice"};

    protected enum PageType {
        APPLICATION(0), DIAGNOSTICS(1), INSTALL_SAVE(2), NOTIFICATIONS(3), OIG(4), VOICE(5);
        private final int index;

        /**
         * Constructor
         *
         * @param index Index of the Settings PageType
         */
        private PageType(int index) {
            this.index = index;
        }

        /**
         * Get the section text for the Settings PageType.
         * @return Section text for the Settings PageType
         */
        private String getSectionText() {
            return SECTION_TEXT[this.index];
        }
    }

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public SettingsPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Click a toggle slide on the current settings page given its section ID and
     * itemStr. Works for pages with multiple sections each with unique
     * sectionId.
     *
     * @param sectionId Each settings page are divided into sections with a
     * unique ID (except for Notification Settings page which has non-unique
     * section ids). For example the 'Application Settings' page has a 'Client
     * Update' section with sectionId '#update'
     *
     * @param itemStr A section of a settings page has rows of option items some
     * of which are toggle slides. This string uniquely identifies the toggle
     * slide item
     */
    protected void clickToggleSlide(String sectionId, String itemStr) {
        By locator = By.xpath(String.format(TOGGLE_SLIDE_XPATH_TMPL, sectionId, itemStr));
        waitForElementClickable(locator).click();
    }

    /**
     * Switch a toggle slide on the current settings page to a specific state
     * when given its section ID and itemStr.
     *
     * @param sectionId Each settings page are divided into sections with a
     * unique ID. For example the Application Settings page has a 'Client
     * Update' section with sectionId '#update'
     *
     * @param itemStr A section of a settings page has rows of option items some
     * of which are toggle slides. This string uniquely identifies the toggle
     * slide item
     *
     * @param toggleState Use true to set the toggle state to on. Use false to
     * set the toggle state to off
     */
    protected void setToggleSlide(String sectionId, String itemStr, boolean toggleState) {
        if (!verifyToggleSlideState(sectionId, itemStr, toggleState)) {
            clickToggleSlide(sectionId, itemStr);
        }
    }

    /**
     * Verify that a toggle slide on the current settings page is in a specific
     * state when given its section ID and itemStr.
     *
     * @param sectionId Each settings page are divided into sections with a
     * unique ID. For example the Application Settings page has a 'Client
     * Update' section with sectionId '#update'
     *
     * @param itemStr A section of a settings page has rows of option items some
     * of which are toggle slides. This string uniquely identifies the toggle
     * slide item
     *
     * @param expectedState The expected state for the toggle. True if the
     * toggle is expected to be on. False if the toggle is expected to be off
     *
     * @return true if the toggle state matches the expected state, false
     * otherwise
     */
    protected boolean verifyToggleSlideState(String sectionId, String itemStr, boolean expectedState) {
        By locator = By.xpath(String.format(TOGGLE_SLIDE_XPATH_TMPL, sectionId, itemStr));
        scrollElementToCentre(waitForElementVisible(locator));
        WebElement toggleSlide = waitForElementVisible(locator);
        return toggleSlide.getAttribute("class").contains("otktoggle-ison") == expectedState;
    }

    /**
     * Get list of section 'ng-class' attributes.
     *
     * @return List of section 'ng-class' attributes
     */
    public List<String> getSectionClassAttributes() {
        List<WebElement> sections = driver.findElements(PAGE_SECTIONS_LOCATOR);
        List<String> ngClasses = new ArrayList<>();
        for (WebElement s : sections) {
            String cl = s.getAttribute("ng-class");
            ngClasses.add(cl);
        }
        return ngClasses;
    }

    /**
     * Verify if a settings page has been loaded.
     *
     * @return true if we can see the navigation link labels for a settings
     * page, false otherwise
     */
    public boolean verifySettingsPageLoaded() {
        return new SettingsNavBar(driver).getNavLinkLabels().size() > 0;
    }

    /**
     * Find the active settings page - can be application, diagnostics, etc.
     *
     * @return true if we can see the navigation link labels for a settings
     * page, false otherwise
     */
    public PageType getActiveSettingsPageType() {
        WebElement activePage;
        try {
            activePage = waitForElementVisible(ACTIVE_SETTINGS_PAGE_LOCATOR);
        } catch (Exception e) {
            _log.warn(String.format("Exception: Cannot locate Active Settings Page - %s", e.getMessage()));
            return null;
        }
        String activeNgClass = activePage.getAttribute("ng-class");
        for (PageType pageType : PageType.values()) {
            if (activeNgClass.contains(pageType.getSectionText())) {
                return pageType;
            }
        }

        // No match - perhaps a new settings page has been added?
        _log.warn("No defined PageType text contained in active page section's ng-class attribute: " + activeNgClass);
        return null;
    }

    /**
     * Checks if an element is visible - to use the new isElementVisible
     *
     * @return true if displayed, false otherwise
     */
    public boolean verifyElementVisible(By locator) {
        try {
            final WebElement element = driver.findElement(locator);
            return element.isDisplayed();
        } catch (NoSuchElementException e) {
            return false;
        }
    }

    /**
     * Checks if the 'Notification Toast' is being displayed.
     *
     * @return true if displayed, false otherwise
     */
    private boolean verifyNotificationToastDisplayed() {
        return verifyElementVisible(NOTIFICATION_TOAST_DISPLAY_LOCATOR);
    }

    /**
     * Catch the display (or non-display) of the 'Notification Toast'.
     *
     * @param visible indicates if the check is for visibility or non-visibility
     * @return true if visible is true and toast is displayed, or visible is
     * false and toast is not displayed, false otherwise
     */
    public boolean catchNotificationToastDisplay(boolean visible) {
        return Waits.pollingWait(() -> verifyNotificationToastDisplayed() == visible,
                2000, 100, 0);
    }

    /**
     * Verify the 'Notification Toast' appears and then disappears.
     *
     * @param toggleDescription Describes the toggle for logging purposes (no
     * logging if null)
     * @return true if the 'Notification Toast' appears then disappears, false
     * otherwise
     */
    public boolean verifyNotificationToastFlashed(String toggleDescription) {
        boolean flashed = catchNotificationToastDisplay(true);
        if (!flashed && toggleDescription != null) {
            _log.error(String.format("Notification Toast does not appear for '%s'", toggleDescription));
        } else {
            // Wait for the notification toast to disappear
            sleep(OriginClientData.DEFAULT_NOTIFICATION_EXPIRY_TIME);
            flashed = flashed && catchNotificationToastDisplay(false);
            if (!flashed && toggleDescription != null) {
                _log.error(String.format("Notification Toast has appeared but does not disappear for '%s'", toggleDescription));
            }
        }
        return flashed;
    }

    /**
     * Verify that a section is visible.
     *
     * @param sectionName The name of the section
     * @return true if the section is visible, false otherwise
     */
    protected boolean verifySectionVisible(String sectionName) {
        By sectionLocator = By.xpath(String.format(SECTION_BY_NAME_XPATH_TMPL, sectionName));
        return waitIsElementVisible(sectionLocator);
    }
}