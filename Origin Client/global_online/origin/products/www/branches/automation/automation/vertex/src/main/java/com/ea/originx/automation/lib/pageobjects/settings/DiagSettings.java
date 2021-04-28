package com.ea.originx.automation.lib.pageobjects.settings;

import org.openqa.selenium.WebDriver;

/**
 * Page object representing the 'Settings - Diagnostics' page.
 *
 * @author palui
 */
public class DiagSettings extends SettingsPage {

    private static final String HELP_ORIGIN_SECTION = "helporigin";
    private static final String SHARE_HARDWARE_INFO_ITEMSTR = "Share hardware info";
    private static final String SHARE_SYSTEM_INTERACTION_DATA_ITEMSTR = "Share system interaction data";

    private static final String TROUBLE_SHOOTING_SECTION = "trouble";
    private static final String ORIGIN_IN_GAME_LOGGING_ITEMSTR = "Origin In-Game logging";
    private static final String SAFE_MODE_DOWNLOADING_ITEMSTR = "Safe mode downloading";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public DiagSettings(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify 'Settings - Diagnostics' page is displayed.
     *
     * @return true if 'Settings - Diagnostics is displayed', false otherwise
     */
    public boolean verifyDiagSettingsReached() {
        return getActiveSettingsPageType() == PageType.DIAGNOSTICS;
    }

    /**
     * Toggle 'Share Hardware Info' in the 'Help Improve Origin' section.
     */
    public void toggleSharedHarwareInfo() {
        clickToggleSlide(HELP_ORIGIN_SECTION, SHARE_HARDWARE_INFO_ITEMSTR);
    }

    /**
     * Toggle 'Share System Interaction Data' in the 'Help Improve Origin'
     * section.
     */
    public void toggleShareSystemInteractionData() {
        clickToggleSlide(HELP_ORIGIN_SECTION, SHARE_SYSTEM_INTERACTION_DATA_ITEMSTR);
    }

    /**
     * Toggle 'Origin In-Game Logging' in the 'Trouble-Shooting' section.
     */
    public void toggleOriginInGameLogging() {
        clickToggleSlide(TROUBLE_SHOOTING_SECTION, ORIGIN_IN_GAME_LOGGING_ITEMSTR);
    }

    /**
     * Toggle 'Safe Mode Downloading' in the 'Trouble-Shooting' section.
     */
    public void toggleSafeModeDownloading() {
        clickToggleSlide(TROUBLE_SHOOTING_SECTION, SAFE_MODE_DOWNLOADING_ITEMSTR);
    }

    /**
     * Toggle all toggle slides in this settings page and verify 'Notifcation
     * Toast' flashes each time.
     *
     * @return true if and only if every toggle results in a 'Notification
     * Toast' flash, false otherwise
     */
    public boolean toggleDiagSettingsAndVerifyNotifications() {

        toggleSharedHarwareInfo();
        boolean notificationsFlashed = verifyNotificationToastFlashed("Diagnostics - Help Improve Origin - Share hardware info");

        toggleShareSystemInteractionData();
        notificationsFlashed &= verifyNotificationToastFlashed("Diagnostics - Help Improve Origin - Share system interaction data");

        toggleOriginInGameLogging();
        notificationsFlashed &= verifyNotificationToastFlashed("Diagnostics - Trouble-Shooting - Origin In-Game logging");

        toggleSafeModeDownloading();
        notificationsFlashed &= verifyNotificationToastFlashed("Diagnostics - Trouble-Shooting - Safe mode downloading");

        return notificationsFlashed;
    }
}