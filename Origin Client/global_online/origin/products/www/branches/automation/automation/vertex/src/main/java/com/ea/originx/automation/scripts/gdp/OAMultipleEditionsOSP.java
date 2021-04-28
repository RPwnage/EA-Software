package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.originx.automation.lib.resources.AccountTags;
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
 * Test clicking primary CTA for a multiple editions game takes the user to OSP
 *
 * @author mdobre
 */
public class OAMultipleEditionsOSP extends EAXVxTestTemplate {

    @TestRail(caseId = 3065111)
    @Test(groups = {"gdp", "full_regression"})
    public void testMultipleEditionsOSP(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE_NO_ENTITLEMENTS);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);
        final String entitlementName = entitlement.getName();

        logFlowPoint("Log into Origin with a subscriber account."); // 1
        logFlowPoint("Navigate to GDP of a multiple edition vault entitlement and verify 'Add to Game Library' CTA is visible."); // 2
        logFlowPoint("Click the 'Buy' option and verify the user is navigated to the OSP Page."); // 3
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        MacroGDP.loadGdpPage(driver, entitlement);
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyAddToLibraryVaultGameCTAVisible(), true);

        //3
        gdpActionCTA.selectDropdownGetTheGame();
        AccessInterstitialPage accessInterstitialPage = new AccessInterstitialPage(driver);
        accessInterstitialPage.verifyOSPInterstitialPageReached();
        accessInterstitialPage.clickBuyGameOSPCTA();
        logPassFail(new OfferSelectionPage(driver).verifyOfferSelectionPageReached(), true);

        softAssertAll();
    }
}