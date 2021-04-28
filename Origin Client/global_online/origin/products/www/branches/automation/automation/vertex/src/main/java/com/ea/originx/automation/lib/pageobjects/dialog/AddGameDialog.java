package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.QtDialog;
import com.ea.vx.core.helpers.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Component object class for the 'Add Game' dialog - a complete QT dialog for
 * now.
 *
 * @author palui
 */
public final class AddGameDialog extends QtDialog {

    protected static final String ADD_GAME_DIALOG_TITLE = "Add Game";
    protected final By messageBoxTitleLocator = By.xpath("//*[@id='lblMsgBoxTitle']");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public AddGameDialog(WebDriver driver) {
        super(driver);
    }

    /**
     * Wait for 'Add Game' dialog and switch to it.
     *
     * @return true if successfully switched to the 'Add Game' dialog,
     * false otherwise
     */
    @Override
    public boolean waitIsVisible() {
        sleep(2000);
        switchToAddGameWindow(driver);
        return true;
    }

    /**
     * Verify the dialog's title bar title to confirm it is the 'Add Game'
     * dialog.
     *
     * @return true if title matches the expected, false otherwise
     */
    public boolean verifyAddGameDialogTitleBarTitle() {
        WebElement title = waitForElementVisible(QT_TITLE_LOCATOR);
        return title.getText().equals(ADD_GAME_DIALOG_TITLE);
    }

    /**
     * Verify the 'Add Game' dialog title to confirm it is the 'Add Game' dialog.
     *
     * @return true if title matches the expected, false otherwise
     */
    public boolean verifyAddGameDialogTitle() {
        WebElement dialogTitle = waitForElementVisible(messageBoxTitleLocator);
        return dialogTitle.getText().equalsIgnoreCase(ADD_GAME_DIALOG_TITLE);
    }

    /**
     * Close 'Add Game' dialog and switch to Origin window (Clicking X may cause
     * lost of view executor).
     */
    @Override
    public void close() {
        switchToOriginWindow(driver);
        String originWin = driver.getWindowHandle();
        try {
            switchToAddGameWindow(driver);
            clickXButton();
        } catch (Exception e) {
            Logger.console(String.format("Exception: No 'Add Game' dialog to close - %s", e.getMessage()), Logger.Types.WARN);
        }
        driver.switchTo().window(originWin);
    }
}