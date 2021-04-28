package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test verifies the navigation flow of a non subscriber when attempting to
 * purchase a vault entitlement
 *
 * @author cbalea
 */
public class OAOfferSelectionPageVaultEditionNonSubscriber extends EAXVxTestTemplate {

    @TestRail(caseId = 3064504)
    @Test(groups = {"full_regression", "gdp"})
    public void testOfferSelectionPageVaultEditionNonSubscriber(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.TITANFALL_2);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        logFlowPoint("Log into Origin with a non subscriber account"); // 1
        logFlowPoint("Navigate to the GDP of a vault game with multiple editions"); // 2
        logFlowPoint("Click on the CTA and verify interstitial page is loaded"); // 3
        logFlowPoint("Click on the 'Buy the Game' CTA and verify OSP loads"); // 4

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 3
        new GDPActionCTA(driver).clickGetTheGameCTA();
        AccessInterstitialPage accessInterstitialPage = new AccessInterstitialPage(driver);
        accessInterstitialPage.waitForPageToLoad();
        logPassFail(accessInterstitialPage.verifyOSPInterstitialPageReached(), true);

        // 4
        accessInterstitialPage.clickBuyGameOSPCTA();
        logPassFail(new OfferSelectionPage(driver).verifyOfferSelectionPageReached(), true);

        softAssertAll();
    }
}
