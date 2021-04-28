package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.dialog.BaseGameMessageDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
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
 * Test purchase a currency pack from gdp with interstitial
 *
 * @author mirani
 */
public class OAPurchaseCurrencyPackWithInterstitial extends EAXVxTestTemplate{

    public enum TEST_TYPE {
        SIGNED_IN,
        ANONYMOUS_USER
    }

    public void testPurchaseCurrencyPackWithInterstitial(ITestContext context, TEST_TYPE type) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        final EntitlementInfo currencyPack = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_18_CURRENCY_PACK);
        final String baseGameName = "FIFA 18";
        final String dlcName = currencyPack.getName();

        if (type == TEST_TYPE.SIGNED_IN) {
            logFlowPoint("Login as a newly registered account"); //1
        }

        logFlowPoint("Navigate to GDP of a Game Detail Page of a currency pack and click buy button"); //2
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logFlowPoint("Verify the login page appears and login to continue"); //3
        }

        logFlowPoint("Verify the 'Don't forget the Base Game' dialog is visible"); //4
        logFlowPoint("Click the continue purchase button and purchase the entitlement"); //5

        //1
        WebDriver driver = startClientObject(context, client);
        if (type == TEST_TYPE.SIGNED_IN) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        //2
        logPassFail(MacroGDP.loadGdpPage(driver, currencyPack), true);
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        gdpActionCTA.clickBuyCTA();

        //3
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        //4
        BaseGameMessageDialog baseGameMessageDialog = new BaseGameMessageDialog(driver, dlcName, baseGameName);
        logPassFail(baseGameMessageDialog.waitIsVisible(), true);
        baseGameMessageDialog.clickContinuePurchaseCTA();

        //5
        new PaymentInformationPage(driver).waitForPaymentInfoPageToLoad();
        logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);

        softAssertAll();
    }
}
