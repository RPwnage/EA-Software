package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.EntitlementAwarenessDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that a game that has country restrictions shows a modal
 * letting the user know the offer is not available in their country
 * <p>
 * Does not work in the browser as Selenium does not currently support setting
 * HTTP headers for requests
 *
 * @author alcervantes
 */
public class OAVaultGamesCountryRestrictions extends EAXVxTestTemplate {

    @TestRail(caseId = 1887678)
    @Test(groups = {"originaccess", "full_regression", "client_only"})
    public void testVaultGamesCountryRestrictions(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();

        final String cambodiaIp = "36.37.255.255";
        final EntitlementInfo vaultGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.NEED_FOR_SPEED_PAYBACK);

        logFlowPoint("Open Origin with your IP set to one of the banned countries, log in to an account with Origin Access Subscription (Basic/Premier)"); //1
        logFlowPoint("Navigate to the vault page"); //2
        logFlowPoint("Locate a Base Game or Add-on that has country restrictions, click on it go to the GDP"); // 3
        logFlowPoint("Click the 'Get it now/add to library' button, verify that an error modal appears "); // 4

        //1
        EACoreHelper.overrideCountryTo(cambodiaIp, client.getEACore());
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        logPassFail(MacroGDP.loadGdpPage(driver, vaultGame), true);

        //3
        new GDPActionCTA(driver).clickGetTheGameCTA();
        AccessInterstitialPage accessInterstitialPage = new AccessInterstitialPage(driver);
        accessInterstitialPage.scrollByVerticalOffset(10);
        accessInterstitialPage.clickBuyGameOSPCTA();
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        logPassFail(offerSelectionPage.verifyOfferSelectionPageReached(), true);

        //4
        offerSelectionPage.clickPrimaryButton(vaultGame.getOcdPath());
        EntitlementAwarenessDialog entitlementAwarenessDialog  = new EntitlementAwarenessDialog(driver);
        entitlementAwarenessDialog.waitIsVisible();
        logPassFail(entitlementAwarenessDialog.isOpen(), true);

        softAssertAll();
    }
}