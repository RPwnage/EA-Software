package com.ea.originx.automation.scripts.feature.gifting;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test 'Buy as a Gift' CTA is not showing for 3rd party game, vitual currency,
 * demo and entitlement which contains virtual currency.
 *
 * @author svaghayenegar
 */
public class OAGiftingIneligible extends EAXVxTestTemplate {

    @TestRail(caseId = 40230)
    @Test(groups = {"gifting", "gifting_smoke", "allfeaturesmoke", "int_only", "release_smoke"})
    public void testGiftingIneligibleOrigin(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        
        final EntitlementInfo thirdPartyGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.THIS_WAR_OF_MINE);
        final EntitlementInfo virtualCurrency = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_18_CURRENCY_PACK);
        final EntitlementInfo entitlementWithVirtualCurrency = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.PVZ_GW_2_DELUXE_EDITION);
        
        logFlowPoint("Login as newly registered user"); // 1
        logFlowPoint("Load GDP page for 3rd party game " + thirdPartyGame.getName()); // 2
        logFlowPoint("Verify 'Purchase as a gift' button or 'CTA does not appear for 3rd party game"); // 3
        logFlowPoint("Load GDP page for Virtual Currency " + virtualCurrency.getName()); // 4
        logFlowPoint("Verify 'Purchase as a gift' button or CTA does not appear for Virtual Currency such as " + virtualCurrency.getName()); // 5
        logFlowPoint("Load GDP page for entitlement " + entitlementWithVirtualCurrency.getName() + " which contains virtual currency"); // 6
        logFlowPoint("Verify 'Purchase as a gift' button or CTA does not appear for entitlement which contains virtual currency"); // 7

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, thirdPartyGame), true);
        
        // 3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyDropdownPurchaseAsGiftVisible(), false);
        
        // 4
        logPassFail(MacroGDP.loadGdpPage(driver, virtualCurrency), true);

        // 5
        logPassFail(!gdpActionCTA.verifyBuyDropdownPurchaseAsGiftItemAvailable(), false);

        // 6
        logPassFail(MacroGDP.loadGdpPage(driver, entitlementWithVirtualCurrency), true);
        
        // 7
        gdpActionCTA.selectDropdownPurchaseAsGift();
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        offerSelectionPage.verifyOfferSelectionPageReached();
        logPassFail(!offerSelectionPage.verifyPurchaseAsAGiftCTAVisible(entitlementWithVirtualCurrency.getOcdPath()), true);
        
        softAssertAll();
    }
}