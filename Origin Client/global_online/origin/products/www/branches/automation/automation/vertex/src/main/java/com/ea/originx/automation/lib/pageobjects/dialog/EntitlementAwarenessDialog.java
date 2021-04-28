package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * This dialog pops up when a user signs into a web browser, after trying to buy
 * a game anonymously that is already owned
 *
 * @author jdickens
 */
public class EntitlementAwarenessDialog extends Dialog {

    public static final By DIALOG_LOCATOR = By.cssSelector("origin-dialog-errormodal");
    public static final By ENTITLEMENT_LINK = By.cssSelector(".origin-dialog-content-errormodal .clearfix .origin-dialog-content-errormodal-text a");

    /**
     * Constructor
     *
     * @param driver          Selenium Web Driver
     */
    public EntitlementAwarenessDialog(WebDriver driver){
        super(driver, DIALOG_LOCATOR);
    }

    /**
     * Clicks on the link containing the entitlement that is already owned, navigating
     * to the 'Game Library'
     */
    public void clickEntitlementLink() {
        waitForElementClickable(ENTITLEMENT_LINK).click();
    }
}
