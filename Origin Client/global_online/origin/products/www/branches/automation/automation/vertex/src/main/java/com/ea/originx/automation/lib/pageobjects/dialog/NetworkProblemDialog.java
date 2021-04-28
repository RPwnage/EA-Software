package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Confirmation dialog indicating a network issue.
 *
 * @author glivingstone
 * @author palui
 */
public class NetworkProblemDialog extends Dialog {

    protected static final By DIALOG_NETWORK_PROBLEM_LOCATOR = By.cssSelector("origin-dialog-content-prompt[header='THE NETWORK IS HAVING PROBLEMS']");
    protected static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-footer .otkbtn-primary");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public NetworkProblemDialog(WebDriver driver) {
        super(driver, DIALOG_NETWORK_PROBLEM_LOCATOR);
    }

    // No need to override isOpen as dialog locator is specific enough
    /**
     * Click the 'Close' button on the dialog.
     */
    public void clickCloseButton() {
        waitForElementClickable(CLOSE_BUTTON_LOCATOR).click();
    }
}