package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
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
 * Test the content of the Origin Access Interstitial comparison table
 *
 * @author mdobre
 */
public class OAAccessInterstitialComparisonTable extends EAXVxTestTemplate {

    @TestRail(caseId = 3000753)
    @Test(groups = {"gdp", "full_regression"})
    public void testAccessInterstitialComparisonTable(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BATTLEFIELD_1_STANDARD);

        logFlowPoint("Log into Origin with a non-subscriber account."); // 1
        logFlowPoint("Navigate to the 'Game Details' Page of a vault game with more editions."); // 2
        logFlowPoint("Click on the primary CTA."); //3
        logFlowPoint("Verify there is a 'Compare Editions' title above the comparison table."); //4
        logFlowPoint("Verify the table shows both editions names and box arts at the top."); //5
        logFlowPoint("Verify the title that is included in Origin Access has a badge above the box art."); //6
        logFlowPoint("Verify the green checks and the grey x's for the comparison table are showing."); //7

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        //3
        new GDPActionCTA(driver).clickGetTheGameCTA();
        AccessInterstitialPage accessInterstitialPage = new AccessInterstitialPage(driver);
        logPassFail(accessInterstitialPage.verifyOSPInterstitialPageReached(), true);

        //4
        accessInterstitialPage.scrollToComparisonTable();
        logPassFail(accessInterstitialPage.verifyComparisonTableTitle(), true);

        //5
        logPassFail(accessInterstitialPage.verifyEditionsVisible(), true);

        //6
        logPassFail(accessInterstitialPage.verifyIncludedInOriginAccessBadgeVisible(), true);

        //7
        logPassFail(accessInterstitialPage.verifyGreenAndGreyChecksShowing(), true);

        softAssertAll();
    }
}