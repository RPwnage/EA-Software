package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.SelectOriginAccessPlanDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.UnderAgeWarningDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
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
 * Test verifies a non-mature user is not able to purchase 'Origin Access' or an
 * entitlement
 * *
 * @author cbalea
 */
public class OAPurchaseFlowLegalDisclaimer extends EAXVxTestTemplate {

    @TestRail(caseId = 11063)
    @Test(groups = {"originaccess", "full_regression", "browser_only"})
    public void testPurchaseFlowLegalDisclaimer(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.createNewThrowAwayAccount(15);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.TITANFALL_2);

        logFlowPoint("Navigate to 'Origin' store and log in with a non-mature account"); // 1
        logFlowPoint("Navigate to 'Origin Access' landing page"); // 2
        logFlowPoint("Click 'Join Premier Today' CTA, select plan and verify modal about age restriction is displayed"); // 3
        logFlowPoint("Navigate to the GDP of a mature entitlement and verify user is unable to purchase"); // 4
        logFlowPoint("Verify modal informs user the reason they are unabe to entitle the game is due to being under age"); // 5

        // 1
        WebDriver driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, "/deu/en-us");
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        new NavigationSidebar(driver).gotoOriginAccessPage();
        OriginAccessPage originAccessPage = new OriginAccessPage(driver);
        logPassFail(originAccessPage.verifyPageReached(), true);

        // 3
        originAccessPage.clickJoinPremierSubscriberCta();
        SelectOriginAccessPlanDialog selectOriginAccessPlanDialog = new SelectOriginAccessPlanDialog(driver);
        logPassFail(selectOriginAccessPlanDialog.waitIsVisible(), true);

        // 4
        selectOriginAccessPlanDialog.clickNext();
        UnderAgeWarningDialog underAgeWarningDialog = new UnderAgeWarningDialog(driver);
        logPassFail(underAgeWarningDialog.waitIsVisible(), true);

        // 5
        underAgeWarningDialog.clickCloseCircle();
        MacroGDP.loadGdpPage(driver, entitlement);
        new GDPActionCTA(driver).clickGetTheGameCTA();
        AccessInterstitialPage accessInterstitialPage = new AccessInterstitialPage(driver);
        accessInterstitialPage.verifyOSPInterstitialPageReached();
        accessInterstitialPage.clickBuyGameOSPCTA();
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        offerSelectionPage.verifyOfferSelectionPageReached();
        offerSelectionPage.clickPrimaryButton(entitlement.getOcdPath());
        underAgeWarningDialog.waitIsVisible();
        logPassFail(underAgeWarningDialog.verifyDialogBodyText(), true);

        softAssertAll();
    }
}
