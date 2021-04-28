package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.TryTheGameOutDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.Sims4;
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
 * Test a demo/trial modal is having all the necessaries informations displayed
 *
 * @author cdeaconu
 */
public class OADemoTrialModal extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3065108)
    @Test(groups = {"gdp", "full_regression"})
    public void testDemoTrialModal(ITestContext context) throws Exception{
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getUnentitledUserAccount();
        
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);
        String entitlementTrialOcdPath = Sims4.SIMS4_TRIAL_OCD_PATH;
        String entitlementDemoOcdPath = Sims4.SIMS4_DEMO_OCD_PATH;
        
        logFlowPoint("Log into Origin."); // 1
        logFlowPoint("Navigate to GDP of a Demo/Trial entitlement"); // 2
        logFlowPoint("Click on the 'Try it First' CTA and verify a demo/trial modal appears."); // 3
        logFlowPoint("Verify there is a title on the modal."); // 4
        logFlowPoint("Verify there is a large game card showing for each demo/trial."); // 5
        logFlowPoint("Verify there is a pack art showing in each large game card."); // 6
        logFlowPoint("Verify there is a title string showing in each large game card."); // 7
        logFlowPoint("Verify there is a sub-title string showing in each large game card."); // 8
        logFlowPoint("Verify there is a description string showing in each large game card."); // 9
        logFlowPoint("Verify there is an 'Add to Game Library' button showing in each large game card."); // 10
        logFlowPoint("Verify there is a 'Close' button showing at the bottom right of the modal."); // 11
        logFlowPoint("Verify there is a cross icon showing at the top right of the modal."); // 12
        logFlowPoint("Click on the 'Close' button and verify the modal closes."); // 13
        logFlowPoint("Click on the 'Try it First' CTA and verify a demo/trial modal appears."); // 14
        logFlowPoint("Click on the 'Cross' icon and verify the modal closes"); // 15
           
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        gdpActionCTA.clickTryItFirstCTA();
        TryTheGameOutDialog tryTheGameOutDialog = new TryTheGameOutDialog(driver);
        logPassFail(tryTheGameOutDialog.waitIsVisible(), true);
        
        // 4
        boolean isTitleVisible = tryTheGameOutDialog.verifyTitleVisible();
        boolean isTitleContainsKeywords = tryTheGameOutDialog.verifyTitleContainsIgnoreCase(TryTheGameOutDialog.DIALOG_TITLE_KEYWORDS);
        logPassFail(isTitleVisible && isTitleContainsKeywords, false);
        
        // 5
        boolean isTrialLargeGameCardSectionVisible = tryTheGameOutDialog.verifyLargeGameCardSectionVisible(entitlementTrialOcdPath);
        boolean isDemoLargeGameCardSectionVisible = tryTheGameOutDialog.verifyLargeGameCardSectionVisible(entitlementDemoOcdPath);
        logPassFail(isTrialLargeGameCardSectionVisible && isDemoLargeGameCardSectionVisible, true);
        
        // 6
        boolean isTrialLargeGameCardsImageVisible = tryTheGameOutDialog.verifyLargeGameCardsPackArtVisible(entitlementTrialOcdPath);
        boolean isDemoLargeGameCardsImageVisible = tryTheGameOutDialog.verifyLargeGameCardsPackArtVisible(entitlementDemoOcdPath);
        logPassFail(isTrialLargeGameCardsImageVisible && isDemoLargeGameCardsImageVisible, false);
        
        // 7
        boolean isTrialLargeGameCardTitleVisible = tryTheGameOutDialog.verifyLargeGameCardTitleVisible(entitlementTrialOcdPath);
        boolean isDemoLargeGameCardTitleVisible = tryTheGameOutDialog.verifyLargeGameCardTitleVisible(entitlementDemoOcdPath);
        logPassFail(isTrialLargeGameCardTitleVisible && isDemoLargeGameCardTitleVisible, false);
        
        // 8
        boolean isTrialLargeGameCardSubTitleVisible = tryTheGameOutDialog.verifyLargeGameCardSubTitleVisible(entitlementTrialOcdPath);
        boolean isDemoLargeGameCardSubTitleVisible = tryTheGameOutDialog.verifyLargeGameCardSubTitleVisible(entitlementDemoOcdPath);
        logPassFail(isTrialLargeGameCardSubTitleVisible && isDemoLargeGameCardSubTitleVisible, false);
        
        // 9
        boolean isTrialLargeGameCardDescriptionVisible = tryTheGameOutDialog.verifyLargeGameCardDescriptionVisible(entitlementTrialOcdPath);
        boolean isDemoLargeGameCardDescriptionVisible = tryTheGameOutDialog.verifyLargeGameCardDescriptionVisible(entitlementDemoOcdPath);
        logPassFail(isTrialLargeGameCardDescriptionVisible && isDemoLargeGameCardDescriptionVisible, false);
        
        // 10
        boolean isTrialLargeGameCardCTAVisible = tryTheGameOutDialog.verifyLargeGameCardsAddToGameLibraryCTAVisible(entitlementTrialOcdPath);
        boolean isDemoLargeGameCardCTAVisible = tryTheGameOutDialog.verifyLargeGameCardsAddToGameLibraryCTAVisible(entitlementDemoOcdPath);
        logPassFail(isTrialLargeGameCardCTAVisible && isDemoLargeGameCardCTAVisible, false);
        
        // 11
        logPassFail(tryTheGameOutDialog.verifyCloseButtonVisible(), true);
        
        // 12
        logPassFail(tryTheGameOutDialog.verifyCloseCircleVisible(), true);
        
        // 13
        logPassFail(tryTheGameOutDialog.closeAndWait(), true);
        
        // 14
        gdpActionCTA.clickTryItFirstCTA();
        logPassFail(tryTheGameOutDialog.waitIsVisible(), true);
        
        // 15
        tryTheGameOutDialog.clickCloseCircle();
        logPassFail(tryTheGameOutDialog.waitIsClose(), true);
        
        softAssertAll();
    }
}