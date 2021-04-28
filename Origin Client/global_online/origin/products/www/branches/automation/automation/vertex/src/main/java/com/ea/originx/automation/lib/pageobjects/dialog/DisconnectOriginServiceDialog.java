package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.WebDriver;

/**
 * Represents the dialog that appears when a user is logged in two
 * different browsers/tabs in the same machine.
 *
 * @author nvarthakavi
 */
public class DisconnectOriginServiceDialog extends Dialog{

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public DisconnectOriginServiceDialog(WebDriver driver) {
        super(driver);
    }
}