package com.ea.originx.automation.lib.pageobjects.settings;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.vx.originclient.helpers.RobotKeyboard;
import com.ea.vx.originclient.utils.Waits;
import org.apache.commons.lang3.tuple.Pair;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object representing the 'Settings - Origin In-Game' page.
 *
 * @author palui
 */
public class OIGSettings extends SettingsPage {

    private static final By OIG_KEYBOARD_SHORTCUT_INPUT_LOCATOR = By.cssSelector(".oig-leHotKeys");

    // The followings are used to locate a toggle slide in a single section settings page using its itemStr only
    // This currently applies to Settings - OIG page only
    private static final String NON_ID_SECTION_XPATH = ACTIVE_SETTINGS_PAGE_XPATH + "//section[contains(@class, 'origin-settings-section')]";
    private static final String NON_ID_ITEMSTR_XPATH_TMPL = NON_ID_SECTION_XPATH + "//div[contains(@class, 'origin-settings-details')]/div[contains(@class, 'origin-settings-div-main') and contains(@itemstr,'%s')]";
    private static final String NON_ID_TOGGLE_SLIDE_XPATH_TMPL = NON_ID_ITEMSTR_XPATH_TMPL + "/..//div[contains(@class, 'otktoggle origin-settings-toggle')]";

    private static final String ENABLE_OIG_ITEMSTR = "Enable Origin In-Game";
    private static final String ALLOW_IN_UNVERIFED_GAME_ITEMSTR = "Allow unverified games";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OIGSettings(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify 'Settings - Application' page is displayed.
     *
     * @return true if 'Settings - Origin In-Game' is displayed, false otherwise
     */
    public boolean verifyOIGSettingsReached() {
        return getActiveSettingsPageType() == PageType.OIG;
    }

    /**
     * Sets the keyboard shortcut for toggling the OIG overlay.
     *
     * @param keycodes The keys to be assigned to the OIG shortcut
     */
    public void setOIGKeyboardShortcut(int... keycodes) {
        waitForElementClickable(OIG_KEYBOARD_SHORTCUT_INPUT_LOCATOR).click();
        Waits.sleep(5000); // wait 5 seconds before entering keys to make sure element is clicked
        try {
            new RobotKeyboard().pressAndReleaseKeys(client, keycodes);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Verify that the OIG keyboard shortcut matches the expected value.
     *
     * @param keys The expected keys to find in the OIG keyboard shortcut
     * @return true if the expected keys are found within the OIG keyboard
     * shortcut, false otherwise
     */
    public boolean verifyOIGKeyboardShortcut(Pair<String, String> keys) {
        String keyboardShortcutText = waitForElementVisible(OIG_KEYBOARD_SHORTCUT_INPUT_LOCATOR).getAttribute("value");
        return StringHelper.containsIgnoreCase(keyboardShortcutText, keys.getLeft(), keys.getRight());
    }

    /**
     * Click a toggle slide on the OIG settings page given its itemStr.
     *
     * @param itemStr String that uniquely identifies a toggle slide item
     */
    protected void clickToggleSlide(String itemStr) {
        By locator = By.xpath(String.format(NON_ID_TOGGLE_SLIDE_XPATH_TMPL, itemStr));
        waitForElementVisible(locator);
        waitForElementClickable(locator).click();
    }

    /**
     * Verify that a toggle slide on the current settings page is in a specific
     * state when given its itemStr.
     *
     * @param itemStr A section of a settings page has rows of option items some
     * of which are toggle slides. This string uniquely identifies the toggle
     * slide item
     * @param expectedState The expected state for the toggle. True if the
     * toggle is expected to be on. False if the toggle is expected to be off
     * @return true if the toggle state matches the expected state, false
     * otherwise
     */
    protected boolean verifyToggleSlideState(String itemStr, boolean expectedState) {
        By locator = By.xpath(String.format(NON_ID_TOGGLE_SLIDE_XPATH_TMPL, itemStr));
        WebElement toggleSlide = waitForElementVisible(locator);
        return toggleSlide.getAttribute("class").contains("otktoggle-ison") == expectedState;
    }

    /**
     * Switch a toggle slide on the current settings page to a specific state
     * when given its itemStr.
     *
     * @param itemStr String that uniquely identifies a toggle slide item
     * @param toggleState Use true to set the toggle state to on. Use false to
     * set the toggle state to off
     */
    protected void setToggleSlide(String itemStr, boolean toggleState) {
        if (!verifyToggleSlideState(itemStr, toggleState)) {
            clickToggleSlide(itemStr);
        }
    }

    /**
     * Toggle 'Enable Origin In-Game' in the 'Origin In-Game' section.
     */
    public void toggleEnableOIG() {
        clickToggleSlide(ENABLE_OIG_ITEMSTR);
    }

    /**
     * Toggle 'Allow In Unverified Games' in the 'Origin In-Game' section.
     */
    public void toggleAllowInUnverifiedGames() {
        clickToggleSlide(ALLOW_IN_UNVERIFED_GAME_ITEMSTR);
    }

    /**
     * Set 'Enable Origin In-Game' to 'On' in the 'Origin In-Game' section.
     */
    public void setEnableOIG(boolean toggleState) {
        setToggleSlide(ENABLE_OIG_ITEMSTR, toggleState);
    }

    /**
     * Verify that 'Enable Origin In-Game' in the 'Origin In-Game' section is
     * set to the correct state.
     *
     * @param expectedState The expected state for the toggle. True if the
     * toggle is expected to be on. False if the toggle is expected to be off.
     * @return true if the toggle state matches the expected state, false otherwise
     */
    public boolean verifyEnableOIG(boolean expectedState) {
        return verifyToggleSlideState(ENABLE_OIG_ITEMSTR, expectedState);
    }

    /**
     * Toggle all toggle slides in this settings page and verify 'Notifcation
     * Toast' flashes each time.
     *
     * @return true if and only if every toggle results in a 'Notification
     * Toast' flash, false otherwise
     */
    public boolean toggleOIGSettingsAndVerifyNotifications() {

        boolean notificationsFlashed = true;
        toggleEnableOIG();
        notificationsFlashed &= verifyNotificationToastFlashed("Origin In-Game - Origin-In-Game - Enable Origin In-Game");
        // Ensure OIG is enabled otherwise "Allow in unverified games" will be hidden
        setEnableOIG(true);
        // Wait for notfication flash if any (don't care about returned result)
        verifyNotificationToastFlashed(null);
        toggleAllowInUnverifiedGames();
        notificationsFlashed &= verifyNotificationToastFlashed("Origin In-Game - Origin-In-Game - Allow unverified games");

        return notificationsFlashed;
    }
}