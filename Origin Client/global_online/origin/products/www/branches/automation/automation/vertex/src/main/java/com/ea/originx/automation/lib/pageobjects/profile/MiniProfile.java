package com.ea.originx.automation.lib.pageobjects.profile;

import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object that represents the 'Mini Profile' (Bottom left corner of the SPA).
 *
 * @author glivingstone
 */
public class MiniProfile extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(MiniProfile.class);

    protected static final By MINI_PROFILE_LOCATOR = By.cssSelector(".origin-miniprofile");

    protected static final By PROFILE_USERNAME_LOCATOR = By.cssSelector(".origin-miniprofile .origin-miniprofile-username-area");
    protected static final By PROFILE_AVATAR_LOCATOR = By.cssSelector("img.otkavatar-img");
    protected static final By PROFILE_ORIGIN_ACCESS_BADGE_LOCATOR = By.cssSelector(".origin-miniprofile-accessbadge img");

    protected static final String POPOUT_MENU_CSS = "ul.origin-nav.origin-nav-stacked";
    protected static final By POPOUT_MENU_LOCATOR = By.cssSelector(POPOUT_MENU_CSS);

    protected static final String MINIPROFILE_POPOUT_CONTAINER_CSS = ".origin-navigation-bottom-popout-container";
    protected static final By MINIPROFILE_POPOUT_CONTAINER_LOCATOR = By.cssSelector(MINIPROFILE_POPOUT_CONTAINER_CSS);

    protected static final String MINIPROFILE_MENU_PATH_CSS = MINIPROFILE_POPOUT_CONTAINER_CSS + " a.origin-nav-link";
    protected static final By MINIPROFILE_MENU_PATH_LOCATOR = By.cssSelector(MINIPROFILE_MENU_PATH_CSS);

    protected static final String POPOUT_MENU_PATH_CSS_TMPL = MINIPROFILE_POPOUT_CONTAINER_CSS + " li[href='%s'] > a";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public MiniProfile(WebDriver driver) {
        super(driver);
    }

    /**
     * Wait for the 'Mini Profile' to load.
     */
    public void waitForProfileToLoad() {
        waitForAngularHttpComplete();
    }

    /**
     * Clicks the profile avatar, which has the effect of opening a dropdown
     * menu.
     */
    public void clickProfileAvatar() {
        _log.debug("clicking profile avatar");
        waitForElementClickable(PROFILE_AVATAR_LOCATOR).click();
        sleep(1000); // Wait for dropdown menu animation to complete.
    }

    /**
     * Click on the username in the 'Mini Profile', which has the effect of opening a
     * dropdown menu.
     */
    public void clickProfileUsername() {
        _log.debug("clicking profile username");
        waitForElementClickable(PROFILE_USERNAME_LOCATOR).click();
        sleep(1000); // Wait for dropdown menu animation to complete.
    }

    /**
     * Checks if the 'Mini Profile' popout menu is hidden.
     *
     * @return True if the 'Mini Profile' popout menu is hidden, false otherwise
     */
    public boolean isPopoutHidden() {
        //Returns false sooner than waitIsElementVisible. It's more common for miniprofile v2 not to be visible since it needs to be hovered over
        return Waits.pollingWait(() -> !isElementVisible(MINIPROFILE_POPOUT_CONTAINER_LOCATOR));
    }

    /**
     * Get all menu items from the 'Mini Profile' popout menu.
     *
     * @return A list of WebElements for the popout menu items
     */
    private List<WebElement> getPopoutMenuItems() {
        if (isPopoutHidden()) {
            _log.debug("mini profile hidden");
            hoverOverMiniProfile();
            // need sleep here for animation stop, otherwise following click may
            // apply on unexpected element
            sleep(1000L);
        }
        return waitForElementPresent(MINIPROFILE_POPOUT_CONTAINER_LOCATOR).findElements(MINIPROFILE_MENU_PATH_LOCATOR);
    }

    /**
     * Check whether the 'Mini Profile' popout menu items with matching text Strings
     * exist.
     *
     * @param elementTextStrings List of text strings to match
     * @param exist An array of expected existence (true for existence, false for
     * non-existence)
     * @return true if list of Strings all match an element in the list, and all
     * elements match the existence statuses, false otherwise
     */
    public boolean verifyPopoutMenuItemsExist(List<String> elementTextStrings, boolean exist[]) {
        return TestScriptHelper.verifyElementsExistStatuses(getPopoutMenuItems(), elementTextStrings, exist);
    }

    /**
     * Click an item at the 'Mini Profile' popout menu given its 'href'
     * attribute.
     *
     * @param href Href attribute for the item to click on
     */
    public void selectProfileDropdownMenuItemByHref(String href) {
        if (isPopoutHidden()) {
            _log.debug("mini profile hidden");
            hoverOverMiniProfile();
        }
        By locator = By.cssSelector(String.format(POPOUT_MENU_PATH_CSS_TMPL, href));
        waitForElementClickable(locator).click();
    }

    /**
     * Clicks on an item from the 'Mini Profile' popout menu given
     * its menu item name.
     *
     * @param itemText The item to click on
     */
    public void selectProfileDropdownMenuItem(String itemText) {
        _log.debug("Popout items :" + getPopoutMenuItems());
        waitForElementClickable(getElementWithText(getPopoutMenuItems(), itemText)).click();
    }

    /**
     * Click on 'View My Profile' from the 'Mini Profile' popout menu.
     */
    public void selectViewMyProfile() {
        selectProfileDropdownMenuItem("View My Profile");
    }

    /**
     * Click 'Application Settings' from the 'Mini Profile' popout menu.
     */
    public void selectApplicationSettings() {
        selectProfileDropdownMenuItem("Application Settings");
    }

    /**
     * Click 'EA Account and Billing' from the 'Mini Profile' popout menu.
     */
    public void selectAccountBilling() {
        selectProfileDropdownMenuItem("EA Account and Billing");
    }

    /**
     * On browser login, click 'Language Preferences' (for any current language)
     * from the 'Mini Profile' popout menu.
     */
    public void selectBrowserLanguagePreferences() {
        selectProfileDropdownMenuItemByHref("selectlanguage");
    }

    /**
     * Verify the user being shown in the mini profile matches the expected
     * user.
     *
     * @param expectedUsername The user we expect to be logged in
     * @return true if matches, false otherwise
     */
    public boolean verifyUser(String expectedUsername) {
        new NavigationSidebar(driver).openSidebar();
        return getUsername().equals(expectedUsername);
    }

    /**
     * Verify the avatar is shown in the 'Mini Profile'.
     *
     * @return true if shown, false otherwise
     */
    public boolean verifyAvatarVisible() {
        return waitIsElementVisible(PROFILE_AVATAR_LOCATOR);
    }
    
    /**
     * Verify the 'Origin Access' badge is show in the 'Mini Profile'.
     *
     * @return true if shown, false otherwise
     */
    public boolean verifyOriginAccessBadgeVisible() {
        return waitIsElementVisible(PROFILE_ORIGIN_ACCESS_BADGE_LOCATOR);
    }
    
    /**
     * Verify the 'Origin Access' badge is show in the 'Mini Profile'.
     *
     * @return true if shown, false otherwise
     */
    public boolean verifyOriginPremierBadgeVisible() {
        String imageURL = waitForElementVisible(PROFILE_ORIGIN_ACCESS_BADGE_LOCATOR).getAttribute("ng-src");
        return imageURL.contains("premier") && verifyUrlResponseCode(imageURL);
    }

    /**
     * Waits for the username element in the 'Mini Profile' to be visible and then
     * gets the username text.
     *
     * @return The username text that was found
     * @throws TimeoutException If the username element is not found within the
     * maximum allowed duration
     */
    public String getUsername() throws TimeoutException {
        WebElement usernameElement = waitForElementVisible(PROFILE_USERNAME_LOCATOR, 180);
        return usernameElement.getText();
    }

    /**
     * Gets the avatar source from the 'Mini Profile'.
     *
     * @return A string containing the URL for the avatar in this profile.
     */
    public String getAvatarSource() {
        WebElement userAvatar = waitForChildElementPresent(MINI_PROFILE_LOCATOR, PROFILE_AVATAR_LOCATOR);
        return userAvatar.getAttribute("ng-src");
    }

    /**
     * Click on 'Sign Out' from the 'Mini Profile' dropdown menu.
     */
    public LoginPage selectSignOut() {
        selectProfileDropdownMenuItem("Sign Out");
        Waits.waitForWindowHandlesToStabilize(driver, 60);
        return new LoginPage(driver);
    }

    /**
     * Hovers over the 'Mini Profile' to open popout menu.
     */
    public void hoverOverMiniProfile() {
        hoverElement(waitForElementVisible(MINI_PROFILE_LOCATOR));
    }

}
