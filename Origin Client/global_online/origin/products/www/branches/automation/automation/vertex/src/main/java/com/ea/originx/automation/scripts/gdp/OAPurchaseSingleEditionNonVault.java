package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test purchase a single edition non vault entitlement
 *
 * @author cdeaconu
 */
public class OAPurchaseSingleEditionNonVault extends EAXVxTestTemplate {

    public enum TEST_TYPE {
        SUBSCRIBER,
        NON_SUBSCRIBER,
        ANONYMOUS_USER
    }

    public void testPurchaseSingleEditionNonVault(ITestContext context, OAPurchaseSingleEditionNonVault.TEST_TYPE type) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        final String environment = OSInfo.getEnvironment();

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BLACK_MIRROR);
        final String entitlementName = entitlement.getName();

        if (type != TEST_TYPE.ANONYMOUS_USER) {
            logFlowPoint("Log into Origin as newly registered account."); // 1
        }
        if (type == TEST_TYPE.SUBSCRIBER) {
            logFlowPoint("Purchase Origin Access."); // 2
        }
        logFlowPoint("Navigate to GDP of a single edition non vault entitlement."); // 3
        logFlowPoint("Verify 'Buy with <price>' CTA is visible and click on it."); // 4
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logFlowPoint("Login as a newly registered user."); // 5
        }
        logFlowPoint("Complete the purchase flow and close the thank you modal."); // 6
        logFlowPoint("Verify the primary button changes to 'View in Library'."); // 7
        logFlowPoint("Click on the 'View in Library' primary button and verify the page is navigated to 'Game Library'."); // 8
        logFlowPoint("Verify the purchased game is now in 'Game Library'."); // 9

        WebDriver driver = startClientObject(context, client);

        // 1
        if (isClient) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        // 2
        if (type == TEST_TYPE.SUBSCRIBER) {
            if(environment.equals("production")) {
                new MainMenu(driver).selectRedeemProductCode();
                logPassFail(MacroRedeem.redeemOABasicSubscription(driver, userAccount.getEmail()), true);
            } else {
                logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
            }
        }

        // 3
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 4
        GDPHeader gdpHeader = new GDPHeader(driver);
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        boolean isGdpHeaderPresent = gdpHeader.verifyGDPHeaderReached();
        boolean isGameTitleCorrect = gdpHeader.verifyGameTitle(entitlementName);
        boolean isBuyButtonVisible = gdpActionCTA.isBuyCTAVisible();
        logPassFail(isGdpHeaderPresent && isGameTitleCorrect && isBuyButtonVisible, true);

        gdpActionCTA.clickBuyCTA();

        //5
        if (!isClient) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        // 6
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        if((environment.equals("production")) && (type != TEST_TYPE.ANONYMOUS_USER)) {
            paymentInformationPage.switchToPaymentByPaypalAndSkipToReviewOrderPage();
            MacroPurchase.completePurchaseByPaypalOffCode(driver, entitlement.getName(), userAccount.getEmail());
            MacroPurchase.handleThankYouPage(driver);
        } else {
            logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);
        }
        // 7
        logPassFail(gdpActionCTA.verifyViewInLibraryCTAVisible(), true);

        // 8
        gdpActionCTA.clickViewInLibraryCTA();
        GameLibrary gameLibrary = new GameLibrary(driver);
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        // 9
        logPassFail(gameLibrary.isGameTileVisibleByName(entitlementName), true);

        softAssertAll();
    }
}
