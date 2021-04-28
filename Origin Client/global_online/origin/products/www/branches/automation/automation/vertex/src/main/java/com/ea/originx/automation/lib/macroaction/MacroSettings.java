package com.ea.originx.automation.lib.macroaction;

import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.dialog.ConfirmLanguageDialog;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.pageobjects.settings.InstallSaveSettings;
import com.ea.originx.automation.lib.pageobjects.settings.OIGSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.resources.LanguageInfo;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import org.apache.commons.lang3.tuple.Pair;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;

import java.lang.invoke.MethodHandles;

/**
 * Macroaction for the 'Application Settings' in the 'Main Menu'
 *
 * @author nvarthakavi
 */
public class MacroSettings {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     */
    private MacroSettings() {
    }

    /**
     * Change the 'Cloud Save' settings through 'Application Settings'
     *
     * @param driver Selenium WebDriver
     * @param setOn  true if user wants to set this on.
     * @return true if the value is set to the expected value
     */
    public static boolean changeCloudSaveSettings(WebDriver driver, boolean setOn) {
        new MainMenu(driver).selectApplicationSettings();
        new SettingsNavBar(driver).clickInstallSaveLink();
        InstallSaveSettings installSettings = new InstallSaveSettings(driver);
        Waits.pollingWait(() -> installSettings.verifyInstallSaveSettingsReached());
        installSettings.setCloudSave(setOn);
        return installSettings.getCloudSave() == setOn;
    }

    /**
     * Go to the Application Settings, change the language to the given language, and then restart the client
     *
     * @param driver   Selenium WebDriver
     * @param client   The OriginClient instance
     * @param language LanguageEnum enumeration
     * @return True if the client is successfully restarted, false otherwise
     * @throws Exception
     */
    public static boolean setLanguage(WebDriver driver, OriginClient client, LanguageInfo.LanguageEnum language) throws Exception {
        try {
            AppSettings appSettings = new MainMenu(driver).selectApplicationSettings();
            if (appSettings.getCurrentLanguage().getLocalName() == language.getLocalName()) {
                return true;
            } else {
                appSettings.selectLanguage(language);
                ConfirmLanguageDialog confirmLanguageDialog = new ConfirmLanguageDialog(driver);
                confirmLanguageDialog.waitForVisible();
                confirmLanguageDialog.restart();
                Waits.pollingWaitEx(() -> !ProcessUtil.isOriginRunning(client));
                return Waits.pollingWaitEx(() -> ProcessUtil.isOriginRunning(client));
            }
        } catch (TimeoutException e) {
            _log.error("Failed to change the settings : " + e);
            return false;
        }
    }

    /**
     * Go to the 'Settings', enable/disable 'OIG Notifications'.
     *
     * @param driver    Selenium WebDriver
     * @param isEnabled true to enable 'OIG Notifications', false otherwise
     * @return true if the setting was successful, false otherwise
     */
    public static boolean setOIGNotification(WebDriver driver, boolean isEnabled) {
        new MainMenu(driver).selectApplicationSettings();
        SettingsNavBar settingsNavBar = new SettingsNavBar(driver);
        settingsNavBar.clickOIGLink();
        OIGSettings oigSettings = new OIGSettings(driver);
        Waits.pollingWait(oigSettings::verifyOIGSettingsReached);
        oigSettings.setEnableOIG(isEnabled);
        return oigSettings.verifyEnableOIG(isEnabled);
    }

    /**
     * Go to 'Settings', change 'OIG Notifications' shortcut.
     *
     * @param driver           Selenium WebDriver
     * @param newOIGKeyStrings The expected keys to find in the OIG keyboard shortcut
     * @param newOIGKeyCodes   The keys to be assigned to the OIG shortcut
     * @return true if successfully set to given keys, false otherwise
     */
    public static boolean setOIGShortcut(WebDriver driver, Pair<String, String> newOIGKeyStrings, int... newOIGKeyCodes) {
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectApplicationSettings();
        SettingsNavBar settingsNavBar = new SettingsNavBar(driver);
        settingsNavBar.clickOIGLink();
        OIGSettings oigSettings = new OIGSettings(driver);
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60);
        Waits.pollingWait(oigSettings::verifyOIGSettingsReached);
        oigSettings.setOIGKeyboardShortcut(newOIGKeyCodes);
        return oigSettings.verifyOIGKeyboardShortcut(newOIGKeyStrings);
    }

    /**
     * Change the 'Automatic game updates' setting on through 'Application Setting'
     *
     * @param driver Selenium WebDriver
     * @param setOn  true if user wants to set this on.
     * @return true if the value is set to the expected value
     */
    public static boolean setKeepGamesUpToDate(WebDriver driver, boolean setOn) {
        new MainMenu(driver).selectApplicationSettings();
        new SettingsNavBar(driver).clickApplicationLink();
        AppSettings appSettings = new AppSettings(driver);
        appSettings.setKeepGamesUpToDate(setOn);
        return appSettings.verifyKeepGamesUpToDate(setOn);
    }
}
