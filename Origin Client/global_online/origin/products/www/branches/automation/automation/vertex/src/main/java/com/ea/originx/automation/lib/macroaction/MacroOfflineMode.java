package com.ea.originx.automation.lib.macroaction;

import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;

/**
 * Macro class containing static methods for multi-step actions related to
 * Origin Client Offline Mode
 *
 * @author palui
 */
public final class MacroOfflineMode {

    /**
     * Constructor
     */
    private MacroOfflineMode() {
    }

    /**
     * Click 'Go Offline' in the Origin menu and verify client goes offline.
     *
     * @param driver Selenium WebDriver
     * @return true if client goes offline as indicated at the main menu bar,
     * false otherwise
     */
    public static boolean goOffline(WebDriver driver) {
        new MainMenu(driver).selectGoOffline();
        return isOffline(driver);
    }

    /**
     * Click 'Go Online' at the Origin menu and verify client goes online
     *
     * @param driver Selenium WebDriver
     * @return true if client goes online as indicated at the main menu bar,
     * false otherwise
     */
    public static boolean goOnline(WebDriver driver) {
        new MainMenu(driver).selectGoOnline();
        return isOnline(driver);
    }

    /**
     * Verify client is offline by inspecting the main menu bar.
     *
     * @param driver Selenium WebDriver
     * @return true if client is offline as indicated at the main menu bar,
     * false otherwise
     */
    public static boolean isOffline(WebDriver driver) {
        MainMenu mainMenu = new MainMenu(driver);
        return Waits.pollingWait(() -> mainMenu.verifyItemEnabledInOrigin("Go Online") && mainMenu.verifyOfflineModeButtonText());
    }

    /**
     * Verify client is online by inspecting the main menu bar.
     *
     * @param driver Selenium WebDriver
     * @return true if client is online as indicated at the main menu bar, false
     * otherwise
     */
    public static boolean isOnline(WebDriver driver) {
        MainMenu mainMenu = new MainMenu(driver);
        return Waits.pollingWait(() -> mainMenu.verifyItemEnabledInOrigin("Go Offline") && !mainMenu.verifyOfflineModeButtonText());
    }
}