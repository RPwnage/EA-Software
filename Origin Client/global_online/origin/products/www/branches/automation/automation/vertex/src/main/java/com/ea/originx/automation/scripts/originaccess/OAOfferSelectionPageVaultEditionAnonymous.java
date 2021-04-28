package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test verifies OSP is reached when attempting to buy a vault game as an
 * anonymous user
 *
 * @author cbalea
 */
public class OAOfferSelectionPageVaultEditionAnonymous extends EAXVxTestTemplate {

    @TestRail(caseId = 3064507)
    @Test(groups = {"originaccess", "full_regression", "browser_only"})
    public void testOfferSelectionPageVaultEditionAnonymous(ITestContext context) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.TITANFALL_2);

        logFlowPoint("Navigate to 'Origin' store"); // 1
        logFlowPoint("Navigate to the GDP of an entitlement with multiple editions"); // 2
        logFlowPoint("Click on the primary CTA and verify it navigates 'Access Interstitial' page"); // 3
        logFlowPoint("Click on the 'Buy Now' CTA and vereify it navigates to OSP"); // 4

        // 1
        final WebDriver driver = startClientObject(context, client);

        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 3
        new GDPActionCTA(driver).clickGetTheGameCTA();
        AccessInterstitialPage accessInterstitialPage = new AccessInterstitialPage(driver);
        logPassFail(accessInterstitialPage.verifyOSPInterstitialPageReached(), true);

        // 4
        accessInterstitialPage.clickBuyGameOSPCTA();
        logPassFail(new OfferSelectionPage(driver).verifyOfferSelectionPageReached(), true);

        softAssertAll();
    }
}
