package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Redeem Complete' dialog, which pops up after confirming a
 * redemption at the 'Redeem Confirm' dialog.
 *
 * @author cbalea
 */
public class RedeemCompleteDialog extends Dialog {

    protected static final String REDEEM_COMPLETE_DIALOG_CSS = ".checkout-content ";
    protected static final String IFRAME = "redeem-iframe";
    protected static final By IFRAME_LOCATOR = By.cssSelector("#" + IFRAME);
    protected static final By REDEEM_COMPLETE_DIALOG_LOCATOR = By.cssSelector(REDEEM_COMPLETE_DIALOG_CSS);
    protected static final By LAUNCH_GAME_BUTTON_LOCATOR = By.cssSelector(REDEEM_COMPLETE_DIALOG_CSS + ".otkbtn-primary");
    protected static final By TITLE_LOCATOR = By.cssSelector(REDEEM_COMPLETE_DIALOG_CSS + "h4");
    protected static final By PRODUCT_DESCRIPTION_LOCATOR = By.cssSelector(REDEEM_COMPLETE_DIALOG_CSS + ".otkc");
    protected static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(REDEEM_COMPLETE_DIALOG_CSS + ".otkbtn-light");
    protected static final String EXPECTED_ACTIVATION_TITLE = "Product Redeemed";
    protected static final String[] EXPECTED_ACTIVATION_BODY = {"Congrats", "Download", "game library"};

    public RedeemCompleteDialog(WebDriver driver) {
        super(driver, REDEEM_COMPLETE_DIALOG_LOCATOR, TITLE_LOCATOR);
    }

    /**
     * Waits for the 'Redeem Complete' dialog to be active and switches to it.
     */
    public void waitAndSwitchToRedeemCompleteDialog() {
        driver.switchTo().defaultContent();
        waitForPageToLoad();
        // switches focus to iframe, unless already focused
        if (!checkIfFrameIsActive("iframe")) {
            waitForElementVisible(IFRAME_LOCATOR, 30);
            waitForFrameAndSwitchToIt(IFRAME);
        }
    }

    /**
     * Verifies the 'Redeem Complete' dialog title is correct after going
     * through the activation process.
     *
     * @return true if the title is a match, false otherwise
     */
    public boolean verifyRedeemCompleteDialogActivationTitle() {
        return verifyTitleEquals(EXPECTED_ACTIVATION_TITLE);
    }

    /**
     * Verify entitlement name matches the expected name in the description
     *
     * @param productTitle Expected product title (e.g. Client Automation DiP
     * small)
     * @return true if title matches, false otherwise
     */
    public boolean verifyRedeemCompleteDialogProductTitle(String productTitle) {
        String description = waitForElementVisible(PRODUCT_DESCRIPTION_LOCATOR).getText();
        return StringHelper.containsAnyIgnoreCase(description, productTitle);
    }

    /**
     * Clicks the 'Close' button
     *
     */
    public void clickCloseButton() {
        waitForElementClickable(CLOSE_BUTTON_LOCATOR).click();
    }

    /**
     * Verifies if the 'Redeem Complete' dialog is visible
     *
     * @return true if the dialog is visible, false otherwise
     */
    public boolean isRedeemCompleteDialogOpen() {
        return isElementVisible(REDEEM_COMPLETE_DIALOG_LOCATOR);
    }

    /**
     * Verifies the 'Redeem Complete' dialog title matches the expected title
     *
     * @return true if the title is a match or description contains expected
     * keywords, false otherwise
     */
    public boolean verifyRedeemCompleteDialogTitle() {
        return verifyTitleEquals(EXPECTED_ACTIVATION_TITLE) || verifyTitleContainsIgnoreCase(EXPECTED_ACTIVATION_BODY);
    }

    /**
     * Clicks the 'Launch Game' button
     */
    public void clickLaunchGameButton() {
        waitForElementClickable(LAUNCH_GAME_BUTTON_LOCATOR).click();
    }

    /**
     * @return Text from Launch Game Button
     */
    public String getTextFromLaunchEntitlemetButton() {
        return waitForElementVisible(LAUNCH_GAME_BUTTON_LOCATOR).getText();
    }
}
