package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Redeem Confirm' dialog, which pops up after redeeming a game
 * with a product code
 *
 * @author cbalea
 */
public class RedeemConfirmDialog extends Dialog {

    protected static final String REDEEM_CONFIRM_DIALOG_CSS = ".checkout-content ";
    protected static final String IFRAME = "redeem-iframe";
    protected static final By IFRAME_LOCATOR = By.cssSelector("#" + IFRAME);
    protected static final By PRODUCT_TITLE_LOCATOR = By.cssSelector(REDEEM_CONFIRM_DIALOG_CSS + " .otkmodal-body .otkc");
    protected static final By REDEEM_CONFIRM_DIALOG_LOCATOR = By.cssSelector(REDEEM_CONFIRM_DIALOG_CSS);
    protected static final By TITLE_LOCATOR = By.cssSelector(REDEEM_CONFIRM_DIALOG_CSS + "h4");
    protected static final By CONFIRM_BUTTON_LOCATOR = By.cssSelector(REDEEM_CONFIRM_DIALOG_CSS + ".otkbtn-primary");
    protected static final String EXPECTED_CONFIRM_TITLE = "Confirm code activation";

    public RedeemConfirmDialog(WebDriver driver) {
        super(driver, REDEEM_CONFIRM_DIALOG_LOCATOR, TITLE_LOCATOR);
    }

    /**
     * Waits for the 'Redeem Confirm' dialog to be active and switches to it.
     */
    public void waitAndSwitchToRedeemConfirmDialog() {
        driver.switchTo().defaultContent();
        waitForPageToLoad();
        // switches focus to iframe, unless already focused
        if (!checkIfFrameIsActive("iframe")) {
            waitForElementVisible(IFRAME_LOCATOR, 30);
            waitForFrameAndSwitchToIt(IFRAME);
        }
    }

    /**
     * Clicks the 'Confirm' button
     *
     */
    public void clickConfirmButton() {
        waitForElementClickable(CONFIRM_BUTTON_LOCATOR).click();
    }

    /**
     * Verify entitlement name matches the expected name.
     *
     * @param productTitle Expected product title (e.g. Client Automation DiP
     * small)
     * @return true if title matches, false otherwise
     */
    public boolean verifyRedeemConfirmDialogProductTitle(String entitlementname) {
        String titleBody = waitForElementVisible(PRODUCT_TITLE_LOCATOR).getText();
        return StringHelper.containsIgnoreCase(titleBody, entitlementname);
    }

    /**
     * Verifies the 'Redeem Confirm' dialog title matches the expected title.
     *
     * @return true if the title is a match, false otherwise
     */
    public boolean verifyRedeemConfirmDialogTitle() {
        return verifyTitleEquals(EXPECTED_CONFIRM_TITLE);
    }
}
