package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeroActionDescriptors;
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
 * Test info block icon and text for an entitlement that has an edition owned
 * through premier subscription
 *
 * @author cdeaconu
 */
public class OAInfoBlockOwnThroughPremierSubscription extends EAXVxTestTemplate {

    @TestRail(caseId = 3486233)
    @Test(groups = {"gdp", "full_regression"})
    public void testInfoBlockIncludedPremier(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();
        
        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Purchase 'Origin Premier Access'."); // 2
        logFlowPoint("Navigate to GDP of a released multiple edition premier vault entitlement."); // 3
        logFlowPoint("Add the entitlement to library through premier subscription."); // 4
        logFlowPoint("Verify an included with premier membership infoblock is showing"); // 5
        logFlowPoint("Verify there is a green check mark on the left of the infoblock text"); // 6
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroOriginAccess.purchaseOriginPremierAccess(driver), true);
        
        // 3
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 4
        new GDPActionCTA(driver).clickAddToLibraryVaultGameCTA();
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);  
        logPassFail(checkoutConfirmation.waitIsVisible(), true);
        
        // 5
        checkoutConfirmation.closeAndWait();
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockTextContainsKeywords(gdpHeroActionDescriptors.INFO_BLOCK_PREMIER_KEYWORDS), true);

        // 6
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockIconGreen(), true);

        softAssertAll();
    }
}