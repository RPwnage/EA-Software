package com.ea.originx.automation.lib.pageobjects.settings;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.vx.originclient.helpers.RobotKeyboard;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.originx.automation.lib.pageobjects.dialog.InstallationSettingsChangedDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.InvalidDirectoryDialog;
import java.awt.AWTException;
import org.openqa.selenium.By;
import org.openqa.selenium.JavascriptExecutor;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object representing the 'Settings - Installs & Saves' page.
 *
 * @author palui
 */
public class InstallSaveSettings extends SettingsPage {

    private static final String SETTINGS_DETAILS_CSS
            = "#content .otknavbar section.origin-settings-tabbed-section.origin-settings-clearfix.active-section .origin-settings-section .origin-settings-details";
    private static final By CHANGE_GAME_LIBRARY_LOCATION_BUTTON_LOCATOR = By.cssSelector(SETTINGS_DETAILS_CSS + " span[ng-click='changeFolder()']");
    private static final By CHANGE_GAME_INSTALLER_LOCATION_BUTTON_LOCATOR = By.cssSelector(SETTINGS_DETAILS_CSS + " span[ng-click='changeCacheFolder()']");
    private static final String SETTINGS_TABBED_CSS
            = "#content .otknavbar .origin-settings-tabbed-section origin-settings-installs .origin-settings-section:nth-of-type(2)";
    private static final By GAME_LIBRARY_LOCATION_LOCATOR
            = By.cssSelector(SETTINGS_TABBED_CSS + " span[ng-bind='gameLibFolderStr']");
    private static final By GAME_INSTALLER_LOCATION_LOCATOR
            = By.cssSelector(SETTINGS_TABBED_CSS + " span[ng-bind='cacheFolderStr']");
    private static final By GAME_LIBRARY_RESTORE_DEFAULT_LOCATOR = By.xpath("//a[@ng-click= 'resetDownloadFolder()']");
    private static final String IN_THE_CLOUD_SECTION = "#cloud";
    private static final String SAVES_ITEMSTR = "Saves";
    private static final By KEEP_LEGACY_GAME_INSTALLER_TOGGLE_LOCATOR
            = By.cssSelector("origin-settings-slide-toggle[toggleaction='toggleKeepInstallerSetting()'] > div > div");
    private static final By CLOUD_SAVE_SETTING_STATE_LOCATOR = By.cssSelector("origin-settings-slide-toggle[toggleaction='toggleCloudSetting()']");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public InstallSaveSettings(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify 'Settings - Installs and Saves' page is displayed.
     *
     * @return true if 'Settings - Installs and Saves' is displayed, false
     * otherwise
     */
    public boolean verifyInstallSaveSettingsReached() {
        return getActiveSettingsPageType() == PageType.INSTALL_SAVE;
    }

    /**
     * Toggle 'Saves' in the 'In the Cloud' section.
     */
    public void toggleSaves() {
        clickToggleSlide(IN_THE_CLOUD_SECTION, SAVES_ITEMSTR);
    }

    /**
     * Toggle 'Keep Legacy Game Installers' in the 'On Your Computer' section.
     */
    public void toggleKeepLegacyGameInstallers() {
        waitForElementClickable(KEEP_LEGACY_GAME_INSTALLER_TOGGLE_LOCATOR).click();

    }

    /**
     * Toggle all toggle slides in this settings page and verify 'Notifcation
     * Toast' flashes each time.
     *
     * @return true if and only if every toggle results in a 'Notification
     * Toast' flash, false otherwise
     */
    public boolean toggleInstallSaveSettingsAndVerifyNotifications() {
        boolean notificationsFlashed = true;

        toggleSaves();
        notificationsFlashed &= verifyNotificationToastFlashed("Installs & Saves - In the Cloud - Saves");

        toggleKeepLegacyGameInstallers();
        notificationsFlashed &= verifyNotificationToastFlashed("Installs & Saves - On your computer - Keep legacy game installers");

        return notificationsFlashed;
    }

    /**
     * Click on the 'Change Game Folder Location' button.
     */
    public void clickChangeGameFolderLocation() {
        WebElement changeLibraryButton = waitForElementClickable(CHANGE_GAME_LIBRARY_LOCATION_BUTTON_LOCATOR);
        // Temporary measure due to a bug with Selenium webdriver
        JavascriptExecutor executor = (JavascriptExecutor) driver;
        executor.executeScript("var elem=arguments[0]; setTimeout(function() {elem.click();}, 100)", changeLibraryButton);
        sleep(12000); //wait for the Open Directory window to appear
    }

    /**
     * Click on the 'Change Game Installer Location' button.
     */
    public void clickChangeGameInstallerLocation() {
        WebElement changeInstallerButton = waitForElementClickable(CHANGE_GAME_INSTALLER_LOCATION_BUTTON_LOCATOR);
        // Temporary measure due to a bug with Selenium webdriver
        JavascriptExecutor executor = (JavascriptExecutor) driver;
        executor.executeScript("var elem=arguments[0]; setTimeout(function() {elem.click();}, 100)", changeInstallerButton);
        sleep(2000); //wait for the Open Directory window to appear
    }

    /**
     * Click on the 'Restore Default' link for the 'Game Library' location.
     */
    public void clickGameLibraryRestoreDefaultLocation() {
        waitForElementClickable(GAME_LIBRARY_RESTORE_DEFAULT_LOCATOR).click();
    }

    /**
     * Verify the 'Game Library' location.
     *
     * @param expectedFileLocation Expected file location
     * @return true if the 'Game Library' location is equal to the expected file
     * location, false otherwise
     */
    public boolean verifyGameLibraryLocation(String expectedFileLocation) {
        return waitForElementVisible(GAME_LIBRARY_LOCATION_LOCATOR).getText().contains(expectedFileLocation);
    }

    /**
     * Verify the 'Game Installer' Location.
     *
     * @param expectedFileLocation Expected file location
     * @return true if the game installer location is equal to the expected file
     * location, false otherwise
     */
    public boolean verifyGameInstallerLocation(String expectedFileLocation) {
        return waitForElementVisible(GAME_INSTALLER_LOCATION_LOCATOR).getText().contains(expectedFileLocation);
    }

    /**
     * Changes the 'Game Installation' location.
     *
     * @param newLocation The path to change to
     * @return true if the location was successfully changed to the new location,
     * false otherwise
     * @throws java.lang.Exception
     */
    public boolean changeGamesLocationTo(String newLocation) throws Exception {
        clickChangeGameFolderLocation();
        changeLocation(newLocation, new InstallationSettingsChangedDialog(driver));
        return verifyGameLibraryLocation(newLocation);
    }

    /**
     * Changes the 'Game Installation' location.
     *
     * @param newLocation The path to change to
     * @return true if the location was successfully changed to the new location,
     * false otherwise
     * @throws java.lang.Exception
     */
    public boolean changeInstallerLocationTo(String newLocation) throws Exception {
        clickChangeGameInstallerLocation();
        changeLocation(newLocation, new InstallationSettingsChangedDialog(driver));
        // Origin automatically adds "cache" to the end of the path
        String newPath = newLocation + (newLocation.endsWith("\\") ? "" : "\\") + "cache";
        return verifyGameInstallerLocation(newPath);
    }

    /**
     * Changes the 'Game Installation' location to an invalid location expecting an error from
     * the client.
     *
     * @param newLocation The path to change to
     * @return true if the location was not changed, false otherwise
     * @throws java.lang.Exception
     */
    public boolean changeInstallerFolderToInvalid(String newLocation) throws Exception {
        clickChangeGameInstallerLocation();
        changeLocation(newLocation, new InvalidDirectoryDialog(driver));
        return !verifyGameInstallerLocation(newLocation);
    }

    /**
     * Changes the 'Game Installer' to an invalid location expecting an error from
     * the client.
     *
     * @param newLocation The path to change to
     * @return true if the location was not changed, false otherwise
     * @throws java.lang.Exception
     */
    public boolean changeGamesFolderToInvalid(String newLocation) throws Exception {
        clickChangeGameFolderLocation();
        changeLocation(newLocation, new InvalidDirectoryDialog(driver));
        return !verifyGameLibraryLocation(newLocation);
    }

    /**
     * Helper function to do the work of changing an install location.
     *
     * @param newLocation The location to change to
     * @param dialog      The dialog to expect to appear
     */
    private void changeLocation(String newLocation, Dialog dialog) throws Exception {
        sleep(15000);
        try {
            RobotKeyboard robotHandler = new RobotKeyboard();
            robotHandler.type(client, newLocation, 200);
            sleep(1000);
            robotHandler.type(client, "\n\n", 2000);
        } catch (AWTException | InterruptedException e) {
            // If the robot fails, the check in the parent's function
            // will catch it
        }
        dialog.waitForVisible();
        dialog.closeAndWait();
    }

    /**
     * Set cloud setting on/off.
     *
     * @param setOn true if user wants to set this on.
     */
    public void setCloudSave(boolean setOn) {
        boolean isOn = StringHelper.containsAnyIgnoreCase(waitForElementPresent(CLOUD_SAVE_SETTING_STATE_LOCATOR).getAttribute("togglecondition"), "true") ? true : false;
        if (setOn && !isOn) {
            toggleSaves();
        } else if (!setOn && isOn) {
            toggleSaves();
        }
    }

    /**
     * Get cloud setting whether it is on/off.
     *
     * @return true if the settings is turned on, else false
     */
    public boolean getCloudSave() {
        return Boolean.parseBoolean(waitForElementPresent(CLOUD_SAVE_SETTING_STATE_LOCATOR).getAttribute("togglecondition"));
    }
}