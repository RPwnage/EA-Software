package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import java.lang.invoke.MethodHandles;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Page object representing the 'Activation Offline Error' dialog which appears
 * when trying to activate and un-entitled game in offline mode.
 *
 * @author rchoi
 * @author palui
 */
public class ActivationOfflineErrorDialog extends Dialog {

    protected static final String DIALOG_TITLE = "NOT CONNECTED";
    protected static final By MESSAGE_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-body div.otkc");
    protected static final By OK_BUTTON_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-footer .otkmodal-btn-no");

    protected static final String[] MESSAGE_KEYWORDS = {"offline", "reconnect", "Origin", "features"};

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver WebDriver
     */
    public ActivationOfflineErrorDialog(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify 'Activation Offline Error' dialog message content contains the
     * expected keywords (case ignored).
     *
     * @return true if message contains the expected keywords, false otherwise
     */
    public boolean verifyMessage() {
        String message = waitForElementVisible(MESSAGE_LOCATOR, 10).getText();
        return StringHelper.containsIgnoreCase(message, MESSAGE_KEYWORDS);
    }

    /**
     * Click 'OK' button at the 'Activation Offline Error' dialog.
     */
    public void clickOKButton() {
        waitForElementClickable(OK_BUTTON_LOCATOR, 10).click();
    }
}