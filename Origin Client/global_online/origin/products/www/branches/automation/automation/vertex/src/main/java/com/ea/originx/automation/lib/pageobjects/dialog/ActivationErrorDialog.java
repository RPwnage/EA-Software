package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.vx.core.helpers.Logger;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Activation Error' dialog, which pops up after redemption of a
 * game that is not the (un-entitled) one waiting to be launched.
 *
 * @author palui
 */
public final class ActivationErrorDialog extends Dialog {

    protected static final String AE_DIALOG_TITLE = "GAME ACTIVATION ERROR";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ActivationErrorDialog(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify dialog title matches the expected 'Activation Error' dialog title.
     *
     * @return true if title matches, false otherwise
     */
    @Override
    public boolean isOpen() {
        String title = getTitle();
        if (title ==  null) {
            Logger.console(String.format("Exception: Cannot locate 'Activation Error' dialog title"), Logger.Types.WARN);
            return false;
        }
        return StringHelper.containsAnyIgnoreCase(title, AE_DIALOG_TITLE);
    }
}