package com.ea.originx.automation.lib.pageobjects.gamelibrary;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.dialog.QtDialog;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Remove Game' message box.
 *
 * @author palui
 */
public class RemoveGameMsgBox extends QtDialog {

    protected static final By WINDOW_TITLE_LOCATOR = By.xpath("//*[@id='lblWindowTitle']");
    protected static final By TITLE_LOCATOR = By.xpath("//*[@id='lblMsgBoxTitle']");
    protected static final By MESSAGE_LOCATOR = By.xpath("//*[@id='lblMsgBoxText']");
    protected static final String BUTTONS_XPATH = "//Origin--UIToolkit--OriginButtonBox/Origin--UIToolkit--OriginPushButton";
    protected static final By REMOVE_BUTTON_LOCATOR = By.xpath(BUTTONS_XPATH + "[1]");
    protected static final By CANCEL_BUTTON_LOCATOR = By.xpath(BUTTONS_XPATH + "[2]");

    protected static final String EXPECTED_REMOVE_GAME_MSGBOX_WINDOW_TITLE = "Origin";
    protected static final String[] REMOVE_GAME_MSGBOX_TITLE_KEYWORDS = {"Remove", "game"};
    protected static final String[] REMOVE_GAME_MSGBOX_MESSAGE_KEYWORDS = {"Remove", "from", "game library"};

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param chromeDriver Selenium WebDriver (chrome)
     */
    public RemoveGameMsgBox(WebDriver chromeDriver) {
        super(chromeDriver, TITLE_LOCATOR, CANCEL_BUTTON_LOCATOR);
    }

    /**
     * Verify that the 'Remove Game' message box is visible for the entitlement.
     *
     * @param entitlementName Name of entitlement to be checked against the
     * message
     * @return true if the expected windows title, title, and message containing
     * the entitlement are present, false otherwise
     *
     */
    public boolean verifyVisible(String entitlementName) {
        try {
            EAXVxSiteTemplate.switchToRemoveGameMsgBox(driver);
        } catch (TimeoutException e) {
            _log.warn(String.format("Exception: Cannot locate 'Remove Game' message box - %s", e.getMessage()));
            return false;
        }
        if (!waitIsElementVisible(TITLE_LOCATOR, 2)) {
            return false;
        };
        return verifyWindowTitle() && verifyTitle() && verifyMessage(entitlementName);
    }

    /**
     * Verify 'Remove Game' message box window title is 'Origin'.
     *
     * @return true if title is as expected, false otherwise
     */
    public boolean verifyWindowTitle() {
        EAXVxSiteTemplate.switchToRemoveGameMsgBox(driver);
        String title = waitForElementVisible(WINDOW_TITLE_LOCATOR, 10).getText();
        return title.equals(EXPECTED_REMOVE_GAME_MSGBOX_WINDOW_TITLE); // match exact windows title
    }

    /**
     * Verify 'Remove Game Message Box' content title contains the expected
     * keywords (case ignored).
     *
     * @return true if title is as expected, false otherwise
     */
    public boolean verifyTitle() {
        EAXVxSiteTemplate.switchToRemoveGameMsgBox(driver);
        String title = waitForElementVisible(TITLE_LOCATOR, 10).getText();
        return StringHelper.containsIgnoreCase(title, REMOVE_GAME_MSGBOX_TITLE_KEYWORDS);
    }

    /**
     * Verify 'Remove Game Message Box' message content contains the expected
     * keywords (case ignored) as well as the entitlement name.
     *
     * @param gameTitle The entitlement name to match
     * @return true if message contains the expected keywords (case ignored) and
     * the entitlement name, false otherwise
     */
    public boolean verifyMessage(String gameTitle) {
        EAXVxSiteTemplate.switchToRemoveGameMsgBox(driver);
        String message = waitForElementVisible(MESSAGE_LOCATOR, 10).getText();
        return StringHelper.containsIgnoreCase(message, REMOVE_GAME_MSGBOX_MESSAGE_KEYWORDS)
                && message.contains(gameTitle);
    }

    /**
     * Click 'Remove' button at the 'Remove Game Message Box'.
     */
    public void clickRemoveButton() {
        EAXVxSiteTemplate.switchToRemoveGameMsgBox(driver);
        waitForElementClickable(REMOVE_BUTTON_LOCATOR).click();
    }

    /**
     * Click 'Cancel' button at the 'Remove Game Message Box'.
     */
    public void clickCancelButton() {
        EAXVxSiteTemplate.switchToRemoveGameMsgBox(driver);
        waitForElementClickable(CANCEL_BUTTON_LOCATOR).click();
    }
}