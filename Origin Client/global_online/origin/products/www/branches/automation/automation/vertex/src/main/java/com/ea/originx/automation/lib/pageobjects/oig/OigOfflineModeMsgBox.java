package com.ea.originx.automation.lib.pageobjects.oig;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.pageobjects.template.OAQtSiteTemplate;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the 'OIG Offline Mode' message xox inside the OIG.
 *
 * @author palui
 */
public class OigOfflineModeMsgBox extends OAQtSiteTemplate {

    protected static final By WINDOW_TITLE_LOCATOR = By.xpath("//*[@id='lblWindowTitle']");
    protected static final By TITLE_LOCATOR = By.xpath("//*[@id='lblMsgBoxTitle']");
    protected static final By MESSAGE_LOCATOR = By.xpath("//*[@id='lblMsgBoxText']");
    protected static final By GO_ONLINE_BUTTON_LOCATOR = By.xpath("//Origin--UIToolkit--OriginPushButton");

    protected static final String OFFLINE_MODE_MSGBOX_WINDOW_TITLE = "OFFLINE MODE";
    protected static final String OFFLINE_MODE_MSGBOX_TITLE = "You are offline";
    protected static final String[] OFFLINE_MODE_MSGBOX_MESSAGE_KEYWORDS = {"features", "not available", "Offline", "Go online"};

    // RegEx pattern to accept leading '&', or even after 'Go', Selenium may insert to the button label
    protected static final String GO_ONLINE_BUTTON_LABEL_REGEX = "^&?Go&? Online$";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OigOfflineModeMsgBox(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify OIG 'Offline Mode' message box Windows title is "OFFLINE MODE".
     *
     * @return true if title is as expected, false otherwise
     */
    public boolean verifyWindowTitle() {
        EAXVxSiteTemplate.switchToOigOfflineModeMsgBox(driver);
        String title = waitForElementVisible(WINDOW_TITLE_LOCATOR, 10).getText();
        return title.equals(OFFLINE_MODE_MSGBOX_WINDOW_TITLE); // match exact windows title
    }

    /**
     * Verify 'OIG Offline Mode' message box content title is 'You are offline'
     * (case ignored).
     *
     * @return true if title is as expected, false otherwise
     */
    public boolean verifyTitle() {
        EAXVxSiteTemplate.switchToOigOfflineModeMsgBox(driver);
        String title = waitForElementVisible(TITLE_LOCATOR, 10).getText();
        return title.equalsIgnoreCase(OFFLINE_MODE_MSGBOX_TITLE);
    }

    /**
     * Verify 'OIG Offline Mode' message box message content contains the
     * expected keywords (case ignored).
     *
     * @return true if message contains the expected keywords, false otherwise
     */
    public boolean verifyMessage() {
        EAXVxSiteTemplate.switchToOigOfflineModeMsgBox(driver);
        String message = waitForElementVisible(MESSAGE_LOCATOR, 10).getText();
        return StringHelper.containsIgnoreCase(message, OFFLINE_MODE_MSGBOX_MESSAGE_KEYWORDS);
    }

    /**
     * Click the 'Go Online' button at the 'OIG Offline Mode' message box.
     */
    public void clickGoOnline() {
        EAXVxSiteTemplate.switchToOigOfflineModeMsgBox(driver);
        WebElement goOnlineButton = waitForElementClickable(GO_ONLINE_BUTTON_LOCATOR, 10);
        String goOnlineButtonLabel = goOnlineButton.getText();
        if (StringHelper.matchesIgnoreCase(goOnlineButtonLabel, GO_ONLINE_BUTTON_LABEL_REGEX)) {
            goOnlineButton.click();
        } else {
            String errorMessage = "Unexpected 'Go Online' button label: " + goOnlineButton.getText();
            _log.error(errorMessage);
            throw new RuntimeException(errorMessage);
        }
    }
}