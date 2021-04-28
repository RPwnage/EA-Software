package com.ea.originx.automation.scripts.checkout;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.originx.automation.lib.pageobjects.dialog.SofortDialog;
import com.ea.originx.automation.lib.pageobjects.store.*;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests purchasing an entitlement through Sofort: a German/Dutch/Belgian
 * payment processor
 * <p>
 * Does not work in the browser as Selenium does not currently support setting
 * HTTP headers for requests
 *
 * @author svaghayenegar
 */
public class OAPurchaseSofort extends EAXVxTestTemplate {

    @TestRail(caseId = 1016568)
    @Test(groups = {"checkout", "services_smoke", "int_only"})
    public void testPurchaseEntitlementSofort(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BLACK_MIRROR);
        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST("Germany");
        EACoreHelper.overrideCountryTo(CountryInfo.CountryEnum.GERMANY, client.getEACore());

        logFlowPoint("Launch Origin with a country override to one of [DE, BE, NL] and login as a new user " + userAccount.getUsername()); // 1
        logFlowPoint("Navigate to " + entitlement.getName() + "'s GDP and click buy"); // 2
        logFlowPoint("Select Sofort as the payment method verify the amount and product descriptions are correct"); // 3
        logFlowPoint("Complete the Sofort flow and verify you are brought to the Thank You page"); // 4

        // 1
        final WebDriver driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, "/deu/en-us");
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        MacroGDP.loadGdpPage(driver, entitlement);
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        String gdpPrice = gdpActionCTA.getPriceFromBuyButton();
        gdpActionCTA.clickBuyCTA();
        PaymentInformationPage paymentPage = new PaymentInformationPage(driver);
        paymentPage.waitForPaymentInfoPageToLoad();
        logPassFail(paymentPage.verifyPaymentInformationReached(), true);

        // 3
        paymentPage.selectSofort();
        paymentPage.clickContinueToSofort();
        ReviewOrderPage reviewOrderPage = new ReviewOrderPage(driver);
        Waits.pollingWait(() -> reviewOrderPage.verifyReviewOrderPageReached());
        reviewOrderPage.clickPayNow();
        sleep(10000); // added this sleep to wait for the Sofort Dialog to open and load
        SofortDialog sofortDialog = new SofortDialog(driver);
        sofortDialog.acceptCookies();
        String sofortPrice = sofortDialog.getPrice(isClient);
        String sofortProduct = sofortDialog.getProductName(isClient);
        logPassFail(sofortPrice.equals(gdpPrice) && sofortProduct.equals(entitlement.getName()), true);

        // 4
        sofortDialog.completePurchase();
        sleep(25000); // Completing Sofort purchase takes a long time to close
        ThankYouPage thankYouPage = new ThankYouPage(driver);
        thankYouPage.waitForThankYouPageToLoad();
        logPassFail(Waits.pollingWait(() -> thankYouPage.verifyThankYouPageReached()), true);

        softAssertAll();
    }
}
