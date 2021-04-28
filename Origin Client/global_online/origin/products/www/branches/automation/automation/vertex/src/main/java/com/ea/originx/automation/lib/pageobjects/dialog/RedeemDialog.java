package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.I18NUtil;
import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.originx.automation.lib.resources.games.MassEffect3;
import com.ea.vx.originclient.resources.OSInfo;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Component object class for the 'Redeem' dialog
 *
 * @author cbalea
 */
public class RedeemDialog extends Dialog {
    
    protected static final String REDEEM_DIALOG_CSS = ".checkout-content ";
    protected static final String IFRAME= "redeem-iframe";
    protected static final By IFRAME_LOCATOR = By.cssSelector("#" + IFRAME);
    protected static final By CANCEL_BUTTON_LOCATOR = By.cssSelector(".checkout-content .otkbtn-light");
    protected static final By REDEEM_DIALOG_LOCATOR = By.cssSelector(REDEEM_DIALOG_CSS);
    protected static final By NEXT_BUTTON_LOCATOR = By.cssSelector(REDEEM_DIALOG_CSS + ".otkbtn-primary");
    protected static final By INPUT_CODE_LOCATOR = By.cssSelector("#code-redemption-input");
    protected static final By TITLE_LOCATOR = By.cssSelector(REDEEM_DIALOG_CSS + "h4");
    protected String[] USED_CODE_WARNING_WORDS = {I18NUtil.getMessage("already"), I18NUtil.getMessage("redeemed")};
    protected String[] INVALID_CODE_WARNING_WORDS = {I18NUtil.getMessage("notValid")};
    protected final String INVALID_CODE = "INVALID-PRODUCT-CODE";
    protected static final By ERROR_MESSAGE_LOCATOR = By.cssSelector(REDEEM_DIALOG_CSS + ".otkinput-errormsg");
    
    public RedeemDialog(WebDriver driver){
        super(driver, REDEEM_DIALOG_LOCATOR, TITLE_LOCATOR);
    }
    
    /**
     * Waits for the 'Redeem Product' dialog to be active and switches to it.
     */
    public void waitAndSwitchToRedeemProductDialog(){
        driver.switchTo().defaultContent();
        waitForPageToLoad();
        // switches focus to iframe, unless already focused
        if (!checkIfFrameIsActive("iframe")) {
            waitForElementVisible(IFRAME_LOCATOR, 30);
            waitForFrameAndSwitchToIt(IFRAME);
        }
    }
    
    /**
     * Enters the product code in input text box
     * 
     * @param code product activation code
     */
    public void enterProductCode(String code){
        WebElement inputTextbox = waitForElementVisible(INPUT_CODE_LOCATOR);
        inputTextbox.clear();
        inputTextbox.sendKeys(code);
    }
    
    /**
     * Clicks the 'Next' button
     * 
     */
    public void clickNextButton(){
        waitForElementClickable(NEXT_BUTTON_LOCATOR).click();
    }
    
    /**
     * Clicks the 'Cancel' button
     * 
     */
    public void clickCancelButton(){
        waitForElementClickable(CANCEL_BUTTON_LOCATOR).click();
    }
    
    /**
     * Enters a redemption code that has already been used.
     */
    public void enterUsedProductCode() {
        String code = OSInfo.isProduction()
                ? MassEffect3.ME3_REDEMPTION_CODE_USED_PROD
                : MassEffect3.ME3_REDEMPTION_CODE_USED_NON_PROD;
        WebElement inputTextbox = waitForElementVisible(INPUT_CODE_LOCATOR);
        inputTextbox.clear();
        inputTextbox.sendKeys(code);
    }
    
    /**
     * Enters an invalid redemption code.
     */
    public void enterInvalidProductCode() {
        WebElement inputTextbox = waitForElementVisible(INPUT_CODE_LOCATOR);
        inputTextbox.clear();
        inputTextbox.sendKeys(INVALID_CODE);
    }
    
    /**
     * Check that the already used code warning exists.
     *
     * @return true if the warning indicating an entered code was already used
     * exists, false otherwise
     */
    public boolean verifyCodeAlreadyUsedWarning() {
        return StringHelper.containsIgnoreCase(
                waitForElementVisible(ERROR_MESSAGE_LOCATOR).getText(), USED_CODE_WARNING_WORDS);
    }
    
    /**
     * Checks that the code already used warning appears when trying to redeem an entitlement already present in 'Game Library'
     * (error text is the same as when we're entering an already used code)
     * 
     * @return true if the warning indicating an entered code was already used
     * exists, false otherwise
     */
    public boolean verifyCodeAlreadyRedeemedWarning() {
        return verifyCodeAlreadyUsedWarning();
    }
    
    /**
     * Check that the invalid code warning exists.
     *
     * @return true if the warning indicating an entered code was invalid exists,
     * false otherwise
     */
    public boolean verifyInvalidCodeWarning() {
        return StringHelper.containsIgnoreCase(
                waitForElementVisible(ERROR_MESSAGE_LOCATOR).getText(), INVALID_CODE_WARNING_WORDS);
    }
    
    /**
     * Check if 'Redeem Product' dialog is open for the activation of an entitlement.
     *
     * @param entitlementName The name of the entitlement that is being
     * activated
     * @return true if dialog is open and title contains name of entitlement,
     * false otherwise
     */
    public boolean isOpenForActivation(String entitlementName) {
        return isOpen() && waitForElementVisible(TITLE_LOCATOR, 10).getText().contains(entitlementName);
    }
    
    /**
     * Verify dialog title matches the expected title of the 'Redeem' dialog
     * opened by clicking 'Activate Game' button from the 'Activation Required'
     * dialog.
     *
     * @param gameName The name of the game to be activated
     * @return true if title matches the 'Redeem' (from 'Activate
     * Game') dialog title, false otherwise
     */
    public boolean verifyRedeemDialogActivateGameTitle(String entitlementName) {
        return waitForElementVisible(TITLE_LOCATOR).getText().contains(entitlementName);
    }
}
