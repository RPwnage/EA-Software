package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.vx.originclient.resources.LanguageInfo.LanguageEnum;
import com.ea.vx.originclient.utils.Waits;
import org.apache.log4j.LogManager;
import org.apache.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebDriverException;

/**
 * Dialog is a base class for dialog and popup boxes in Origin.
 *
 * @author jmittertreiner
 */
public abstract class Dialog extends EAXVxSiteTemplate {

    private static Logger _log;
    private final By dialogLocator;
    private String titleText = null;
    private final By closeLocator;
    private final By TITLE_LOCATOR = By.cssSelector("origin-dialog-content-prompt");

    private final By CLOSE_CIRCLE_LOCATOR = By.cssSelector("origin-dialog .otkicon-closecircle");

    /**
     * Construct a dialog with no title verification. Each parameter can be
     * null, but then you will lose (runtime) access to the associated
     * functions.
     * <p>
     * dialogLocator: waitForVisible<br>
     * defaultLocator: defaultAction<br>
     * closeLocator: close
     *
     * @param driver Selenium WebDriver
     * @param dialogLocator A locator for the dialog
     * @param closeLocator A locator to close the dialog
     */
    public Dialog(WebDriver driver, By dialogLocator, By closeLocator) {
        super(driver);
        _log = LogManager.getLogger(this.getClass().getName());
        this.dialogLocator = dialogLocator;
        this.closeLocator = closeLocator;
    }

    /**
     * Construct a dialog with no with verification. Each parameter can be null,
     * but then you will lose (runtime) access to the associated functions.
     * <p>
     * dialogLocator: waitForVisible<br>
     * defaultLocator: defaultAction<br>
     * closeLocator: close
     *
     * @param driver Selenium WebDriver
     * @param dialogLocator A locator for the dialog title
     * @param closeLocator A locator to close the dialog
     * @param titleText The text the title should be
     */
    public Dialog(WebDriver driver, By dialogLocator, By closeLocator, String titleText) {
        this(driver, dialogLocator, closeLocator);
        this.titleText = titleText;
    }

    /**
     * Waits for the dialog to appear.
     *
     * @return true if the dialog has appeared, false otherwise
     */
    public boolean waitForVisible() {
        if (dialogLocator == null) {
            throw new UnsupportedOperationException("This dialog doesn't have a title locator defined, "
                    + "You should either define it in the dialog class, or use a different method");
        }
        return Waits.pollingWait(this::isOpen);
    }

    /**
     * Clicks the 'Close' button on the dialog.
     */
    public void close() {
        if (closeLocator == null) {
            throw new UnsupportedOperationException("This dialog doesn't have a close action defined, "
                    + "You should either define it in the dialog class, or use a different method");
        }
        waitForElementClickable(closeLocator).click();
    }

    /**
     * Click the 'Close' circle in the upper right corner, just outside of the
     * dialog.
     */
    public void clickCloseCircle() {
        waitForElementVisible(CLOSE_CIRCLE_LOCATOR).click();
        sleep(1000); // Let any animation complete
    }

    /**
     * Checks that the dialog is open and the title text is correct if set.
     *
     * @return true if the dialog is open and the dialog titleText is correct (if
     * set), false otherwise
     */
    public boolean isOpen() {
        if (dialogLocator == null) {
            throw new UnsupportedOperationException("This dialog doesn't have"
                    + " a title locator defined, You should either define it in"
                    + " the dialog class, or use a different method");
        }

        final boolean isDialogVisible = isElementVisible(dialogLocator);

        boolean correctTitleText;
        /*
         It's possible that when the dialog is closing, between the check for
         if the dialog is visible and then checking the title text, the dialog
         closes, causing the waitForElementVisible(TITLE_LOCATOR) to throw
         We catch error and return false since the dialog is no longer open
         */
        try {
            // Check the title text to ensure we have the right dialog, but don't
            // check it if it is not set, or the language isn't currently in English
            correctTitleText = (titleText == null)
                    || OriginClient.getInstance(driver).getLanguage() != LanguageEnum.ENGLISH
                    || (isDialogVisible && StringHelper.containsIgnoreCase(waitForElementVisible(TITLE_LOCATOR)
                            .getText(), titleText));
        } catch (WebDriverException e) {
            return false;
        }

        return isDialogVisible && correctTitleText;
    }

    /**
     * Waits for the dialog to no longer be open.
     *
     * @return true when the dialog is closed, false otherwise
     */
    public boolean waitForClose() {
        return Waits.pollingWait(() -> !isOpen());
    }

    /**
     * Closes and waits until the dialog is confirmed to be closed.
     *
     * @return true if the dialog was closed, false otherwise
     */
    public boolean closeAndWait() {
        close();
        return waitForClose();
    }
}