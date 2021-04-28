package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * This dialog appears when a user tries to sync to the cloud save
 * which has already saved more than 20 syncs to show the user can
 * keep only 20 recent syncs.
 *
 * @author nvarthakavi
 */
public class CloudSaveConflictDialog extends Dialog{

    public static final By DIALOG_LOCATOR = By.cssSelector("origin-dialog-content-prompt[header='Cloud save conflict']");

    public static final By USE_LOCAL_DATA_BUTTON_LOCATOR = By.cssSelector(".origin-cldialog-commandbuttons button.otkbtn-command:nth-of-type(1)");

    public CloudSaveConflictDialog(WebDriver driver) {
        super(driver, DIALOG_LOCATOR);
    }

	/**
     * Click 'Delete Local Data' button to clear the local data in the computer.
     */
    public void clickUseLocalDataButton() {
        waitForElementClickable(USE_LOCAL_DATA_BUTTON_LOCATOR).click();
    }
}