package com.ea.originx.automation.scripts.legal;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.pageobjects.dialog.TryTheGameOutDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.UnderAgeWarningDialog;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests Age Gating when Purchasing Entitlements
 *
 * @author nvarthakavi
 */
public class OAEntitlementAgeGate extends EAXVxTestTemplate {

    @TestRail(caseId = 1016685)
    @Test(groups = {"legal", "release_smoke"})
    public void testEntitlementAgeGate(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);
        final UserAccount userAccount = AccountManagerHelper.createNewThrowAwayAccount("Australia", 14);
        EACoreHelper.overrideCountryTo(CountryInfo.CountryEnum.AUSTRALIA, client.getEACore());

        logFlowPoint("Launch Origin with a country override to Australia and login as a new user with an age of 14 and country as Australia"); // 1
        logFlowPoint("Navigate to mature GDP with trial click Buy and Verify a dialog appears stating that the user does not meet the age requirements"); // 2
        logFlowPoint("In the GDP page, click Get Trial and Verify a dialog appears stating the user does not meet the age requirements"); // 3

        //1
        final WebDriver driver = startClientObject(context, client, null, "/aus/en-us");
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        MacroGDP.loadGdpPage(driver, entitlement);
        new GDPActionCTA(driver).clickGetTheGameCTA();
        new AccessInterstitialPage(driver).clickBuyGameOSPCTA();
        new OfferSelectionPage(driver).clickPrimaryButton(entitlement.getOcdPath());

        boolean isUnderAgeForBuy = new UnderAgeWarningDialog(driver).waitIsVisible();
        logPassFail(isUnderAgeForBuy, false);
        new UnderAgeWarningDialog(driver).clickCloseCircle();

        //3
        MacroGDP.loadGdpPage(driver, entitlement);
        new GDPActionCTA(driver).clickTryItFirstCTA();
        new TryTheGameOutDialog(driver).clickAddGameToLibraryCTA();

        boolean isUnderAgeForGetTrial = new UnderAgeWarningDialog(driver).waitIsVisible();
        logPassFail(isUnderAgeForGetTrial, false);

        client.stop();
        softAssertAll();
    }
}
