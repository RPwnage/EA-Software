package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.pageobjects.store.StorePage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test purchase a currency pack without interstitial
 *
 * @author svaghayenegar
 */
public class OAPurchaseCurrencyPackWithoutInterstitial extends EAXVxTestTemplate {

    public enum TEST_TYPE {
        SIGNED_IN_USER,
        ANONYMOUS_USER
    }

    public void testPurchaseCurrencyPackWithoutInterstitial(ITestContext context, TEST_TYPE type) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        final EntitlementInfo baseGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_18);
        final EntitlementInfo currencyPack = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_18_CURRENCY_PACK);

        logFlowPoint("Login as a newly registered account"); // 1
        logFlowPoint("Purchase a base entitlement which has currency pack"); //2
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logFlowPoint("Logout from origin and stay logged out"); //3
        }
        logFlowPoint("Navigate to GDP of a currency pack"); // 4
        logFlowPoint("Click on the primary button to purchase the coins"); //5
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logFlowPoint("Login back as the same 'newly registered' user"); //6
        }
        logFlowPoint("After logging in expect the checkout flow and complete the checkout flow"); //7

        WebDriver driver = startClientObject(context, client);

        // 1
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        logPassFail(MacroPurchase.purchaseEntitlement(driver, baseGame), true);

        //3
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            new MiniProfile(driver).selectSignOut();
            logPassFail(new StorePage(driver).verifyStorePageReached(), true);
        }

        //4
        logPassFail(MacroGDP.loadGdpPage(driver, currencyPack), true);

        //5
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        boolean isBuyButtonVisible = gdpActionCTA.isBuyCTAVisible();
        logPassFail(isBuyButtonVisible, true);

        gdpActionCTA.clickBuyCTA();

        //6
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        //7
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);

        softAssertAll();
    }
}
