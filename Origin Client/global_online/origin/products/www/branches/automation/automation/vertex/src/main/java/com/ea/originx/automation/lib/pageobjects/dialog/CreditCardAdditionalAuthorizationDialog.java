package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.WebDriver;

/**
 * Represents the Credit Card Additional Authorization dialog
 * In the browser, The additional authorization window opens in a new tab
 *
 *
 * @author SeanLi
 */
public class CreditCardAdditionalAuthorizationDialog extends Dialog {

    public CreditCardAdditionalAuthorizationDialog(WebDriver driver) {
        super(driver);
    }


    /**
     * Switches the driver to the window and frame containing the dialog
     */
    public void focusDialog() {
        waitForPageThatMatches(".*cardinalcommerce.*");
    }

}
