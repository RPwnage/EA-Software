package com.ea.originx.automation.lib.pageobjects.dialog;

import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;

/**
 * Represents the 'Checkout Confirmation' dialog
 *
 * @author glivingstone
 */
public class CheckoutConfirmation extends Dialog {

    protected static final By CONFIRMATION_DIALOG_LOCATOR = By.cssSelector(".otkmodal-content #checkout-confirmation");
    protected static final By DOWNLOAD_LOCATOR = By.cssSelector(".otkmodal-content .origin-checkoutconfirmation-item-details .otkbtn-primary");
    protected static final By TITLE_LOCATOR = By.cssSelector("origin-dialog-content-checkoutconfirmation .otkmodal-header .otkmodal-title");
    protected static final By CLOSE_LOCATOR = By.cssSelector(".otkmodal-close");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public CheckoutConfirmation(WebDriver driver) {
        super(driver, CONFIRMATION_DIALOG_LOCATOR, TITLE_LOCATOR,CLOSE_LOCATOR);
    }

    /**
     * Verify the 'Vault Confirmation' dialog exists.
     *
     * @return true if the network problem dialog exists and visible, false
     * otherwise
     */
    public boolean isDialogVisible() {
        return waitIsElementVisible(CONFIRMATION_DIALOG_LOCATOR);
    }

    /**
     * Clicks the 'Download' button.
     */
    public void clickDownload() {
        waitForElementClickable(waitForElementVisible(DOWNLOAD_LOCATOR)).click();
    }
}