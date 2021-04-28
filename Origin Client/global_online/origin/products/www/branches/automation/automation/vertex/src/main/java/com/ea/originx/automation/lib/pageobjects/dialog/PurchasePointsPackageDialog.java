package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;


/**
 * A dialog that represents purchasing points to buy extra content.
 * 
 * @author rchoi
 */
public class PurchasePointsPackageDialog extends Dialog {

    protected static final By IFRAME_LOCATOR = By.cssSelector("origin-iframe-modal .origin-iframemodal .origin-iframemodal-content iframe");
    protected static final By IFRAME_TITLE_LOCATOR = By.cssSelector(".checkout-content .otkmodal-header .otkmodal-title");
    protected static final By CLOSE_BUTTON_LOCATOR = By.cssSelector("origin-iframe-modal .otkmodal-dialog .otkmodal-close .otkicon-closecircle");
    protected static final By PROCEED_TO_REVIEW_ORDER_BUTTON_LOCATOR = By.cssSelector(".checkout-content.otkmodal-content .otkmodal-footer .otkbtn.otkbtn-primary");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public PurchasePointsPackageDialog(WebDriver driver) {
        super(driver, IFRAME_LOCATOR, IFRAME_TITLE_LOCATOR, CLOSE_BUTTON_LOCATOR);
    }

    /**
     * Verify 'Points Package' dialog has been reached
     *
     * @return true if reached this dialog, false otherwise
     */
    public boolean verifyPurchasePointsPackageDialogReached() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(IFRAME_TITLE_LOCATOR).getText(), "point");
    }

    /**
     * Switches the driver to the window and frame containing the dialog.
     */
    public void waitForPurchasePointsPackageDialogToLoad() {
        waitForPageToLoad();
        waitForFrameAndSwitchToIt(waitForElementVisible(IFRAME_LOCATOR, 10));
    }

    /**
     * Click on the 'Review Order' button
     */
    public void clickReviewOrderButton() {
        waitForElementVisible(PROCEED_TO_REVIEW_ORDER_BUTTON_LOCATOR).click();
    }
}