package com.ea.originx.automation.lib.pageobjects.gamelibrary;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.pageobjects.template.OAQtSiteTemplate;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Download Game' message box.
 *
 * @author palui
 */
public class DownloadGameMsgBox extends OAQtSiteTemplate {

    protected static final By WINDOW_TITLE_LOCATOR = By.xpath("//*[@id='lblWindowTitle']");
    protected static final By TITLE_LOCATOR = By.xpath("//*[@id='lblMsgBoxTitle']");

    protected static final String EXPECTED_DOWNLOAD_GAME_MSGBOX_WINDOW_TITLE = "Origin";
    protected static final String[] DOWNLOAD_GAME_MSGBOX_TITLE_KEYWORDS = {"INSTALL"};

    // Install location section
    protected static final By INSTALL_LOCATION_TITLE_LOCATOR = By.xpath("//*[@id='Inst_Location_Title']");
    protected static final By INSTALL_LOCATION_DESC_LOCATOR = By.xpath("//*[@id='Inst_Location_Desc']");
    protected static final By INSTALL_LOCATION_LOCATOR = By.xpath("//*[@id='downloadInPlaceLocation']");
    protected static final By INSTALL_LOCATION_CHANGE_BUTTON_LOCATOR = By.xpath("//*[@id='btnBrowseDownloadInPlaceLocation']");
    protected static final By INSTALL_LOCATION_RESET_BUTTON_LOCATOR = By.xpath("//*[@id='btnReset_dl_in_place_dir']");

    // Sizing section
    protected static final By DOWNLOAD_SIZE_LOCATOR = By.xpath("//*[@id='lblDownloadSize']");
    protected static final By INSTALL_SIZE_LOCATOR = By.xpath("//*[@id='lblInstallSize']");
    protected static final By AVAILABLE_SPACE_LOCATOR = By.xpath("//*[@id='lblAvailableSpace']");

    // Check boxes
    protected static final By START_MENU_SHORTCUT_CHECKBOX_LOCATOR = By.xpath("//*[@id='startmenuShortcut']");
    protected static final By DESKTOP_SHORTCUT_CHECKBOX_LOCATOR = By.xpath("//*[@id='desktopShortcut']");

    // Buttons
    protected static final String BUTTONS_XPATH = "//Origin--UIToolkit--OriginButtonBox/Origin--UIToolkit--OriginPushButton";
    protected static final By NEXT_BUTTON_LOCATOR = By.xpath(BUTTONS_XPATH + "[1]");
    protected static final By CANCEL_BUTTON_LOCATOR = By.xpath(BUTTONS_XPATH + "[2]");

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param chromeDriver Selenium WebDriver (chrome)
     */
    public DownloadGameMsgBox(WebDriver chromeDriver) {
        super(chromeDriver);
    }

    /**
     * Verify that the 'Download Game' message box is visible for the
     * entitlement.
     *
     * @param entitlementName Name of entitlement to be checked against the
     * message title
     * @return true if the expected windows title and message title (with
     * expected keywords and contains the entitlement) are present,
     * false otherwise
     */
    public boolean verifyVisible(String entitlementName) {
        try {
            EAXVxSiteTemplate.switchToDownloadGameMsgBox(driver);
        } catch (TimeoutException e) {
            _log.warn(String.format("Exception: Cannot locate any Origin message box - %s", e.getMessage()));
            return false;
        }
        if (!waitIsElementVisible(TITLE_LOCATOR, 2)) {
            return false;
        }
        return verifyWindowTitle() && verifyTitle(entitlementName);
    }

    /**
     * Verify 'Download Game' message box window title is 'Origin'.
     *
     * @return true if title is as expected, false otherwise
     */
    public boolean verifyWindowTitle() {
        EAXVxSiteTemplate.switchToDownloadGameMsgBox(driver);
        String title = waitForElementVisible(WINDOW_TITLE_LOCATOR, 10).getText();
        return title.equals(EXPECTED_DOWNLOAD_GAME_MSGBOX_WINDOW_TITLE); // match exact windows title
    }

    /**
     * Verify 'Download Game' message box title contains the expected keywords
     * (case ignored) as well as the entitlement name.
     *
     * @param entitlementName Name of entitlement to be checked against the
     * message title
     * @return true if message title contains the expected keywords (case
     * ignored) and the entitlement name, false otherwise
     */
    public boolean verifyTitle(String entitlementName) {
        EAXVxSiteTemplate.switchToDownloadGameMsgBox(driver);
        String title = waitForElementVisible(TITLE_LOCATOR, 10).getText();
        return StringHelper.containsIgnoreCase(title, DOWNLOAD_GAME_MSGBOX_TITLE_KEYWORDS)
                && StringHelper.containsIgnoreCase(title, entitlementName);
    }

    /**
     * Set the start menu shortcut check box to the specified state.
     *
     * @param state true if checkbox is to be checked, false otherwise
     */
    public void setStartMenuShortcutCheckbox(boolean state) {
        EAXVxSiteTemplate.switchToDownloadGameMsgBox(driver);
        setCheckbox(START_MENU_SHORTCUT_CHECKBOX_LOCATOR, state);
    }

    /**
     * Set the desktop shortcut checkbox to the desired state.
     *
     * @param state true if checkbox is to be checked, false otherwise
     */
    public void setDesktopShortcutCheckbox(boolean state) {
        EAXVxSiteTemplate.switchToDownloadGameMsgBox(driver);
        setCheckbox(DESKTOP_SHORTCUT_CHECKBOX_LOCATOR, state);
    }

    /**
     * Click 'Next' button at the 'Download Game' message box.
     */
    public void clickNextButton() {
        EAXVxSiteTemplate.switchToDownloadGameMsgBox(driver);
        waitForElementClickable(NEXT_BUTTON_LOCATOR).click();
    }

    /**
     * Click 'Cancel' button at the 'Download Game' message box.
     */
    public void clickCancelButton() {
        EAXVxSiteTemplate.switchToDownloadGameMsgBox(driver);
        waitForElementClickable(CANCEL_BUTTON_LOCATOR).click();
    }
}