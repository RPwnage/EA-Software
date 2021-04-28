package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.QtDialog;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Problem Launching Game' message box
 * todo: need test added
 * @author palui
 */
public class ProblemLaunchingGameMsgBox extends QtDialog {

    protected static final By TITLE_LOCATOR = By.xpath("//*[@id='lblMsgBoxTitle']");
    protected static final By MESSAGE_LOCATOR = By.xpath("//*[@id='lblMsgBoxText']");
    protected static final String BUTTONS_XPATH = "//Origin--UIToolkit--OriginButtonBox/Origin--UIToolkit--OriginPushButton";
    protected static final By CANCEL_BUTTON_LOCATOR = By.xpath(BUTTONS_XPATH + "[1]");

    protected static final String COMMAND_LINKS_XPATH = "//Origin--UIToolkit--OriginMsgBox//Origin--UIToolkit--OriginCommandLink";
    protected static final By UPDATE_SHORTCUT_COMMAND_LINK_LOCATOR = By.xpath(COMMAND_LINKS_XPATH + "[1]");
    protected static final By REMOVE_GAME_COMMAND_LINK_LOCATOR = By.xpath(COMMAND_LINKS_XPATH + "[2]");

    protected static final String EXPECTED_PROBLEM_LAUNCHING_GAME_MSGBOX_WINDOW_TITLE = "Problem Launching Game";
    protected static final String[] PROBLEM_LAUNCHING_GAME_MSGBOX_TITLE_KEYWORDS = {"Missing", "Files"};
    protected static final String[] PROBLEM_LAUNCHING_GAME_MSGBOX_MESSAGE_KEYWORDS = {"shortcut", "no longer points", "valid location", "select a new location", "remove game"};

    protected static final String EXPECTED_UPDATE_SHORTCUT_COMMAND_LINK_LABEL = "Update Shortcut";
    protected static final String[] EXPECTED_REMOVE_GAME_COMMAND_LINK_LABEL_KEYWORDS = {"Remove", "Game", "From"};

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param chromeDriver Selenium WebDriver (chrome)
     */
    public ProblemLaunchingGameMsgBox(WebDriver chromeDriver) {
        super(chromeDriver, TITLE_LOCATOR, CANCEL_BUTTON_LOCATOR);
    }

    /**
     * Verify that the 'Problem Launching Game' message box is visible for the
     * entitlement.
     *
     * @param entitlementName Name of entitlement to be checked against the
     * message title
     * @return true if the expected windows title, title containing the
     * entitlement, and message are present, false otherwise
     *
     */
    public boolean verifyVisible(String entitlementName) {
        try {
            EAXVxSiteTemplate.switchToProblemLaunchingGameMsgBox(driver);
        } catch (TimeoutException e) {
            _log.warn(String.format("Exception: Cannot locate 'Problem Launching Game' message box - %s", e.getMessage()));
            return false;
        }
        if (!waitIsElementVisible(TITLE_LOCATOR, 2)) {
            return false;
        }
        return verifyWindowTitle() && verifyTitle(entitlementName) && verifyMessage();
    }

    /**
     * Verify 'Problem Launching Game' message box window title is as expected.
     *
     * @return true if title is as expected, false otherwise
     */
    public boolean verifyWindowTitle() {
        EAXVxSiteTemplate.switchToProblemLaunchingGameMsgBox(driver);
        String title = waitForElementVisible(TITLE_LOCATOR, 10).getText();
        boolean result = title.equals(EXPECTED_PROBLEM_LAUNCHING_GAME_MSGBOX_WINDOW_TITLE); // match exact windows title
        if (!result) {
            _log.warn(String.format("verifyWindowTile: '%s' does not match the expected '%s'",
                    title, EXPECTED_PROBLEM_LAUNCHING_GAME_MSGBOX_WINDOW_TITLE));
        }
        return result;
    }

    /**
     * Verify 'Problem Launching Game' message box message title contains (case
     * ignored) the expected keywords as well as the entitlement name.
     *
     * @param entitlementName Name of entitlement to match in the message title
     * @return true if title has the expected keywords (case ignored) as well as
     * the entitlement name (case ignored), false otherwise
     */
    public boolean verifyTitle(String entitlementName) {
        EAXVxSiteTemplate.switchToProblemLaunchingGameMsgBox(driver);
        String title = waitForElementVisible(TITLE_LOCATOR, 10).getText();
        boolean keywordsMatch = StringHelper.containsIgnoreCase(title, PROBLEM_LAUNCHING_GAME_MSGBOX_TITLE_KEYWORDS);
        boolean entitmenentNameMatches = StringHelper.containsIgnoreCase(title, entitlementName);
        if (!keywordsMatch) {
            _log.warn(String.format("veryTitle: '%s' does not match (case ignored) the expected message title keywords '%s'",
                    title, PROBLEM_LAUNCHING_GAME_MSGBOX_TITLE_KEYWORDS.toString()));
        }
        if (!entitmenentNameMatches) {
            _log.warn(String.format("verifyTitle: '%s' message title does not contain (case ignored) the entitlement name '%s'",
                    title, entitlementName));
        }
        return keywordsMatch && entitmenentNameMatches;
    }

    /**
     * Verify 'Problem Launching Game Message Box' message content contains the
     * expected keywords (case ignored).
     *
     * @return true if message contains the expected keywords (case ignored),
     * false otherwise
     */
    public boolean verifyMessage() {
        EAXVxSiteTemplate.switchToProblemLaunchingGameMsgBox(driver);
        String message = waitForElementVisible(MESSAGE_LOCATOR, 10).getText();
        boolean result = StringHelper.containsIgnoreCase(message, PROBLEM_LAUNCHING_GAME_MSGBOX_MESSAGE_KEYWORDS);
        if (!result) {
            _log.warn(String.format("verifyMessage: '%s' does not match (case ignored) expected message keywords '%s'",
                    message, PROBLEM_LAUNCHING_GAME_MSGBOX_MESSAGE_KEYWORDS.toString()));
        }
        return result;
    }

    /**
     * Verify 'Update Shortcut' command link is visible and has the expected
     * label.
     *
     * @return true if 'Update Shortcut' command link is visible and has the
     * expected label, false otherwise
     */
    public boolean verifyUpdateShortcutCommandLink() {
        EAXVxSiteTemplate.switchToProblemLaunchingGameMsgBox(driver);
        String label = waitForElementVisible(UPDATE_SHORTCUT_COMMAND_LINK_LOCATOR).getText();
        boolean result = label.equalsIgnoreCase(EXPECTED_UPDATE_SHORTCUT_COMMAND_LINK_LABEL);
        if (!result) {
            _log.warn(String.format("verifyUpdateShortcutCommandLink: '%s' does not match (case ignored) expected label '%s'",
                    label, EXPECTED_UPDATE_SHORTCUT_COMMAND_LINK_LABEL));
        }
        return result;
    }

    /**
     * Verify 'Remove Game from Origin' command link is visible and has the
     * expected label.
     *
     * @return true if 'Remove Game from Origin' command link is visible and has
     * the expected label, false otherwise
     */
    public boolean verifyRemoveGameCommandLink() {
        EAXVxSiteTemplate.switchToProblemLaunchingGameMsgBox(driver);
        String label = waitForElementVisible(REMOVE_GAME_COMMAND_LINK_LOCATOR).getText();
        boolean result = StringHelper.containsIgnoreCase(label, EXPECTED_REMOVE_GAME_COMMAND_LINK_LABEL_KEYWORDS);
        if (!result) {
            _log.warn(String.format("verifyRemoveGameCommandLink: '%s' does not contain (case ignored) the expected label keywords '%s'",
                    label, EXPECTED_REMOVE_GAME_COMMAND_LINK_LABEL_KEYWORDS.toString()));
        }
        return result;
    }

    /**
     * Click 'Update Shortcut' command link at the 'Problem Launching Game'
     * message box.
     */
    public void clickUpdateShortcutCommandLink() {
        EAXVxSiteTemplate.switchToProblemLaunchingGameMsgBox(driver);
        waitForElementClickable(UPDATE_SHORTCUT_COMMAND_LINK_LOCATOR).click();
    }

    /**
     * Click 'Remove Game From Origin' command link at the 'Problem Launching
     * Game' message box.
     */
    public void clickRemoveGameCommandLink() {
        EAXVxSiteTemplate.switchToProblemLaunchingGameMsgBox(driver);
        waitForElementClickable(REMOVE_GAME_COMMAND_LINK_LOCATOR).click();
    }

    /**
     * Click 'Cancel' button at the 'Problem Launching Game' message box.
     */
    public void clickCancelButton() {
        EAXVxSiteTemplate.switchToProblemLaunchingGameMsgBox(driver);
        waitForElementClickable(CANCEL_BUTTON_LOCATOR).click();
    }
}