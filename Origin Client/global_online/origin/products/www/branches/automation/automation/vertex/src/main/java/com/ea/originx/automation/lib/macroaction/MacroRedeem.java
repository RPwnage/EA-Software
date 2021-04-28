package com.ea.originx.automation.lib.macroaction;

import com.ea.originx.automation.lib.pageobjects.dialog.RedeemCompleteDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.RedeemConfirmDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.RedeemDialog;
import java.io.IOException;
import java.lang.invoke.MethodHandles;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessNUXFullPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
import com.ea.vx.originclient.net.helpers.CrsHelper;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;

/**
 * Macro class containing static methods for multi-step actions related to the
 * redemption using a product code.
 *
 * @author palui
 */
public final class MacroRedeem {
    
    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());
    
    /**
     * Constructor - make private as class should not be instantiated
     */
    private MacroRedeem() {
    }
    
    /**
     * Static method to redeem a valid code. Handles the 'Redeem Dialog' opened from
     * Games menu, or by game activation (i.e. launching a game outside of
     * Origin and clicking 'Activate Game' at the 'Activation Required' dialog).
     *
     * Assumption: Redeem dialog has been opened
     *
     * @param driver Selenium WebDriver
     * @param productCode The redemption code
     * @param entitlementName Name of the game being activated (null if
     * opening from the 'Origin' menu with no entitlement name to verify)
     * @param closeDialog false when launching the game instead of closing the dialog
     * @return true if successfully redeemed the given entitlement using the
     * given product code, false otherwise
     */
    public static boolean redeemProductCode(WebDriver driver, String productCode, String entitlementName, boolean closeDialog) {
        RedeemDialog redeemDialog = new RedeemDialog(driver);
        redeemDialog.waitAndSwitchToRedeemProductDialog();
        redeemDialog.waitForVisible();
        boolean redeemDialogDisplayed ;
        boolean isEntitlementNullOrSubscription = entitlementName == null || entitlementName == "premier" || entitlementName == "basic";
        
        // Check for the 'Redeem' dialog
        if (isEntitlementNullOrSubscription) {
            redeemDialogDisplayed = redeemDialog.isOpen();
        } else {
            redeemDialogDisplayed = redeemDialog.verifyRedeemDialogActivateGameTitle(entitlementName);
        }
        
        if (!redeemDialogDisplayed) {
            _log.error("'Redeem' dialog does not open");
            return false;
        }
        
        // Enter product code and check for the 'Redeem Confirm' dialog
        redeemDialog.enterProductCode(productCode);
        redeemDialog.clickNextButton();
        RedeemConfirmDialog confirmDialog = new RedeemConfirmDialog(driver);
        confirmDialog.waitAndSwitchToRedeemConfirmDialog();
        confirmDialog.waitForVisible();
        if (confirmDialog.isOpen()) {
            _log.debug("'Redeem Confirm' dialog opens successfully after entering redemption code");
        } else {
            _log.error("'Redeem Confirm' dialog does not open after entering redemption code");
            return false;
        }
        
        // Click 'Confirm' and check for the 'Redeem Complete' dialog
        confirmDialog.clickConfirmButton();
        RedeemCompleteDialog completeDialog = new RedeemCompleteDialog(driver);
        completeDialog.waitAndSwitchToRedeemCompleteDialog();
        completeDialog.waitForVisible();
        boolean completeDialogDisplayed;
        
        if (isEntitlementNullOrSubscription) {
            completeDialogDisplayed = completeDialog.verifyRedeemCompleteDialogActivationTitle();
        } else {
            completeDialogDisplayed = completeDialog.verifyRedeemCompleteDialogTitle();
        }
        
        if (completeDialogDisplayed) {
            _log.debug("'Redeem Complete' dialog opens successfully after confirmation");
        } else {
            _log.error("'Redeem Complete' dialog does not open after confirmation");
            return false;
        }
        
        if(closeDialog) {
            // Click 'Close' to close the 'Redeem Complete' dialog
            completeDialog.clickCloseButton();
        }

        if(completeDialog.getTextFromLaunchEntitlemetButton().equals("Get Started")) {
            completeDialog.clickLaunchGameButton();
            driver.switchTo().defaultContent(); // refocus the window required sometimes to close the dialog properly
            OriginAccessNUXFullPage originAccessNUXFullPage = new OriginAccessNUXFullPage(driver);
            if (originAccessNUXFullPage.verifyPageReached()) {
                if (entitlementName.equals("basic")) {
                    originAccessNUXFullPage.clickSkipNuxGoToVaultLink();
                } else {
                    originAccessNUXFullPage.clickSkipNuxGoToVaultLinkPremierSubscriber();
                }
            } else {
                _log.debug("Origin Access Full NUX Intro Page not reached");
            }
            VaultPage vaultPage = new VaultPage(driver);
            return vaultPage.verifyPageReached();
        }
        
        return true;
    }
    
    /**
     * Static method to redeem a valid product code. Handles the 'Redeem Dialog'
     * opened from the 'Origin' menu, with no entitlement name to verify.
     *
     * @param driver Selenium WebDriver
     * @param productCode The redemption code
     * @return true if successfully redeemed the given product code, false otherwise
     */
    public static boolean redeemProductCode(WebDriver driver, String productCode) {
        return redeemProductCode(driver, productCode, null, true);
    }
    
    /**
     * Static method to redeem a valid product code. Handles the 'Redeem Dialog'
     * opened from the 'Origin' menu which doesn't close the 'Complete Dialog'.
     *
     * @param driver Selenium WebDriver
     * @param productCode The redemption code
     * @param entitlementname Name of the game being activated
     * @return true if successfully redeemed the given product code, false otherwise
     */
    public static boolean redeemProductCode(WebDriver driver, String productCode, String entitlementname){
        return redeemProductCode(driver, productCode, entitlementname, false);
    }

    /**
     * @param driver Selenium WebDriver
     * @param subscriptionCode The redemption code
     * @param subscriptionType Type of subscription
     * @return true on successful purchase
     */
    public static boolean redeemSubscriptionCode(WebDriver driver, String subscriptionCode, String subscriptionType) {
        return redeemProductCode(driver, subscriptionCode, subscriptionType, false);
    }
    
    /**
     * Verify that a message appears saying you cannot redeem an already used code in
     * the redeem dialog
     *
     * @param driver Selenium WebDriver
     * @return true if a message appears saying your redeem code is already used, false otherwise
     */
    public static boolean verifyRedeemCodeAlreadyUsed(WebDriver driver) {
        RedeemDialog redeemDialog = new RedeemDialog(driver);
        redeemDialog.enterUsedProductCode();
        redeemDialog.clickNextButton();
        RedeemConfirmDialog redeemConfirmDialog = new RedeemConfirmDialog(driver);
        redeemConfirmDialog.waitAndSwitchToRedeemConfirmDialog();
        redeemConfirmDialog.waitForVisible();
        redeemConfirmDialog.clickConfirmButton();
        boolean isCodeAlreadyRedeemed = redeemDialog.verifyCodeAlreadyUsedWarning();
        redeemDialog.clickCancelButton();
        return isCodeAlreadyRedeemed;
    }
    
    /**
     * Verify that a message appears saying you cannot redeem an invalid code in
     * the redeem dialog
     *
     * @param driver Selenium WebDriver
     * @return true if a message appears saying your redeem code is invalid, false otherwise
     */
    public static boolean verifyRedeemCodeInvalid(WebDriver driver) {
        RedeemDialog redeemDialog = new RedeemDialog(driver);
        redeemDialog.enterInvalidProductCode();
        redeemDialog.clickNextButton();
        boolean isCodeInvalid = redeemDialog.verifyInvalidCodeWarning();
        redeemDialog.clickCancelButton();
        return isCodeInvalid;
    }

    /**
     * Static method to redeem Origin Access subscription. Handles the 'Redeem Dialog'
     * opened from the 'Origin' menu, with no entitlement name to verify.
     *
     * @param driver Selenium WebDriver
     * @param emailId Email ID of the account redeeming the code
     * @param subscriptionType Type of subscription - Basic or Premier
     * @return true if subscription is successfully redeemed
     */
    public static boolean redeemOriginAccessSubscription(WebDriver driver, String emailId, String subscriptionType) throws IOException {
        subscriptionType = subscriptionType.toLowerCase();
        if(subscriptionType == "basic" || subscriptionType == "premier") {
            String redemptionCode = CrsHelper.getProdOffCode(emailId, subscriptionType, subscriptionType);
            return redeemSubscriptionCode(driver, redemptionCode , subscriptionType);
        } else {
            _log.error("Invalid subscription type");
            return false;
        }
    }

    /**
     * Static method to redeem Origin Access Premier subscription
     *
     * @param driver Selenium WebDriver
     * @param emailId Email ID of the account redeeming the code
     * @return true if subscription is successfully redeemed
     * @throws IOException Exception thrown by unsuccessful Crs API call
     */
    public static boolean redeemOAPremierSubscription(WebDriver driver, String emailId) throws IOException {
        return redeemOriginAccessSubscription(driver, emailId, "premier");
    }

    /**
     * Static method to redeem Origin Access Basic subscription
     *
     * @param driver Selenium WebDriver
     * @param emailId Email ID of the account redeeming the code
     * @return true if subscription is successfully redeemed
     * @throws IOException Exception thrown by unsuccessful Crs API call
     */
    public static boolean redeemOABasicSubscription(WebDriver driver, String emailId) throws IOException {
        return redeemOriginAccessSubscription(driver, emailId, "basic");
    }
}
