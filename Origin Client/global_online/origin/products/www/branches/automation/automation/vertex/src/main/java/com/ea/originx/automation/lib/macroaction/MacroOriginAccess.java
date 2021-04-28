package com.ea.originx.automation.lib.macroaction;

import com.ea.originx.automation.lib.pageobjects.account.OriginAccessSettingsPage;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.dialog.EditPaymentDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.SelectOriginAccessPlanDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessNUXFullPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
import com.ea.vx.originclient.utils.Waits;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;

/**
 * Macro class containing static methods for multi-step actions related to 'Origin Access'.
 *
 * @author rocky, micwang
 */
public final class MacroOriginAccess {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     */
    private MacroOriginAccess() {
    }

    /**
     * Navigates to the 'Origin Access Plan Selection' dialog from the 'Origin
     * Access' page.
     */
    public static void openOriginAccessPlanSelectionDialog(WebDriver driver) {
        // Navigate to the Origin Access Page
        new NavigationSidebar(driver).clickOriginAccessLink();

        // Click the CTA to join Origin Access
        OriginAccessPage oaPage = new OriginAccessPage(driver);
        // to make popup menu for Origin Access sub-menu disappear
        new NavigationSidebar(driver).hoverOnStoreButton();
        oaPage.verifyPageReached();
        oaPage.clickJoinBasicButton();

        //Plan Selection dialog
        SelectOriginAccessPlanDialog selectOriginAccessPlanDialog = new SelectOriginAccessPlanDialog(driver);
        if (Waits.pollingWait(() -> selectOriginAccessPlanDialog.waitIsVisible())) {
            selectOriginAccessPlanDialog.waitForYearlyPlanAndStartTrialButtonToLoad();
        }
    }

    /**
     * Purchase Origin Access, starting from anywhere in the client. This will
     * navigate to the 'Origin Access' page and click 'Join Now' or will click
     * 'Join Origin Access' CTA on the 'Access Interstitial Page' if that page
     * is currently opened. Then it will fill in all the purchasing information,
     * complete the order, then handle the Origin Access NUX dialogs.
     *
     * @param driver         Selenium WebDriver
     * @param plan           The Origin Access plan to select
     * @param skipNUXDialogs close NUX if true, or leave it open if false
     * @param clickOriginAccessLink click 'Origin Access' link if true, or click 
     * 'Join Origin Access & Play' CTA on 'Access Interstitial Page' if false
     * @return true if there are no issues purchasing Origin Access, false otherwise
     */
    public static boolean purchaseOriginAccessWithPlan(WebDriver driver, SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN plan, boolean skipNUXDialogs, boolean clickOriginAccessLink, boolean basicSubscription) {
        
        if(clickOriginAccessLink) {
            // Navigate to the Origin Access Page
            new NavigationSidebar(driver).clickOriginAccessLink();
            
            // Click the CTA to join Origin Access
            OriginAccessPage oaPage = new OriginAccessPage(driver);
            // to make popup menu for Origin Access sub-menu disappear
            oaPage.verifyPageReached();
            new NavigationSidebar(driver).hoverOnStoreButton();
            
            if (basicSubscription) {
                oaPage.clickJoinBasicButton();
            } else {
                oaPage.clickJoinPremierButton();
            }

        } else {
            // Click 'Join Origin Access' CTA on the 'Access Interstitial Page'
            new AccessInterstitialPage(driver).clickJoinOriginAccessCTA();
        }
        
        //Plan Selection dialog
        SelectOriginAccessPlanDialog selectOriginAccessPlanDialog = new SelectOriginAccessPlanDialog(driver);
        selectOriginAccessPlanDialog.waitForYearlyPlanAndStartTrialButtonToLoad();
        selectOriginAccessPlanDialog.selectPlan(plan);
        selectOriginAccessPlanDialog.clickNext();
        if (selectOriginAccessPlanDialog.isOpen()) {
            _log.debug("Error: Could not complete the Origin Access Plan Dialog");
            return false;
        }

        // Fill in the payment details to complete a purchase
        if (MacroPurchase.completePurchase(driver)) {
            driver.switchTo().defaultContent();
        } else {
            _log.debug("Error: Could not complete filling in the purchase information");
            return false;
        }

        if (skipNUXDialogs) {
            // Close the NUX for Origin Access
            driver.switchTo().defaultContent(); // refocus the window required sometimes to close the dialog properly
            OriginAccessNUXFullPage originAccessNUXFullPage = new OriginAccessNUXFullPage(driver);
            if (originAccessNUXFullPage.verifyPageReached()) {
                if (basicSubscription) {
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
     * Purchase the monthly Origin Access subscription from anywhere in the client and show the NUX
     * if it exists.
     *
     * @param driver Selenium WebDriver
     * @return true if purchase of Origin Access monthly subscription was successful.
     */
    public static boolean purchaseMonthlySubscriptionAndShowNux(WebDriver driver) {
        return purchaseOriginAccessWithPlan(driver, SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.MONTHLY_PLAN, false, true, true);
    }

    /**
     * Purchase the monthly Origin Access subscription from anywhere in the
     * client
     *
     * @param driver Selenium WebDriver
     * @return true if purchase of Origin Access monthly subscription was
     * successful.
     */
    public static boolean purchaseMonthlySubscription(WebDriver driver) {
        return purchaseOriginAccessWithPlan(driver, SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.MONTHLY_PLAN, true, true, true);
    }

    /**
     * Purchase the yearly 'Origin Basic Access' subscription from anywhere in the client and show the NUX
     * if it exists.
     *
     * @param driver Selenium WebDriver
     * @return true if purchase of Origin Access yearly subscription was successful.
     */
    public static boolean purchaseOriginAccess(WebDriver driver) {
        return purchaseOriginAccessWithPlan(driver, SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.YEARLY_PLAN, true, true, true);
    }
    
    /**
     * Purchase the yearly 'Origin Basic Access' subscription from 'Access Interstitial
     * Page' and show the NUX if it exists.
     *
     * @param driver Selenium WebDriver
     * @return true if purchase of Origin Access yearly subscription was
     * successful.
     */
    public static boolean purchaseOriginAccessThroughAccessInterstitialPage(WebDriver driver){
        return purchaseOriginAccessWithPlan(driver, SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.YEARLY_PLAN, true, false, true);
    }

    /**
     * Purchase the monthly 'Origin Premier Access' subscription from anywhere in
     * the client and show the NUX if it exists.
     *
     * @param driver Selenium WebDriver
     * @return true if purchase of 'Origin Premier Access' monthly subscription
     * was successful.
     */
    public static boolean purchasePremierMonthlySubscriptionAndShowNux(WebDriver driver) {
         return purchaseOriginAccessWithPlan(driver, SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.MONTHLY_PLAN, false, true, false);
    }

    /**
     * Purchase the monthly 'Origin Premier Access' subscription from anywhere
     * in the client and skips the NUX.
     *
     * @param driver Selenium WebDriver
     * @return true if purchase of 'Origin Premier Access' monthly subscription
     * was successful.
     */
    public static boolean purchasePremierMonthlySubscription(WebDriver driver) {
        return purchaseOriginAccessWithPlan(driver, SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.MONTHLY_PLAN, true, true, false);
    }

    /**
     * Purchase the yearly 'Origin Premier Access' subscription from anywhere in
     * the client and show the NUX if it exists.
     *
     * @param driver Selenium WebDriver
     * @return true if purchase of 'Origin Premier Access' yearly subscription was
     * successful.
     */
    public static boolean purchaseOriginPremierAccess(WebDriver driver) {
        return purchaseOriginAccessWithPlan(driver, SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.YEARLY_PLAN, true, true, false);
    }
    
    /**
     * Open and switch to 'Edit Payment' dialog in 'EA Account and Billing' - 'Origin Access' Tab
     *
     * @param driver Selenium WebDriver
     */
    public static void openAndSwitchToEditPaymentDialog(WebDriver driver) {
        OriginAccessSettingsPage originAccessSettingsPage = new OriginAccessSettingsPage(driver);
        originAccessSettingsPage.clickEditPaymentLink();
        EditPaymentDialog editPaymentDialog = new EditPaymentDialog(driver);
        Waits.sleep(1000); //wait a bit before attempting to switch
        editPaymentDialog.waitAndSwitchToEditPaymentDialog();
        Waits.sleep(3000); //wait for dialog to be refreshed to void stale exception
    }

    /**
     * Add an entitlement to the 'Game Library' by offer ID and handles the
     * confirmation dialogs
     *
     * @param driver  Selenium WebDriver
     * @param offerId The offer ID of the entitlement to navigate to
     */
    public static void addEntitlementAndHandleDialogs(WebDriver driver, String offerId) {
        new VaultPage(driver).clickEntitlementByOfferID(offerId);
        GDPHeader gdpHeader = new GDPHeader(driver);
        if (gdpHeader.verifyGDPHeaderReached()) {
            GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
            gdpActionCTA.clickAddToLibraryVaultGameCTA();
            CheckoutConfirmation checkoutConfirmationDemo = new CheckoutConfirmation(driver);
            checkoutConfirmationDemo.waitForVisible();
            checkoutConfirmationDemo.clickCloseCircle();
        } else {
            _log.error("Cannot Add game to library");
        }
    }
}