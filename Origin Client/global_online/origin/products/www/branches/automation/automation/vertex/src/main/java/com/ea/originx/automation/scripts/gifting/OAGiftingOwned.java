package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test Gifting works when the sender owns the entitlement
 *
 * @author nvarthakavi
 */
public class OAGiftingOwned extends EAXVxTestTemplate {

    @TestRail(caseId = 40227)
    @Test(groups = {"gifting", "full_regression"})
    public void testGiftingOwned(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount gifter = AccountManagerHelper.getEntitledUserAccountWithCountry("Canada", EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD));
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);

        logFlowPoint("Launch Origin and login"); //1
        logFlowPoint("Navigate to GDP of a giftable entitlement"); // 2
        logFlowPoint("Verify the 'Purchase as a Gift' dropdown-menu item appears and click it."); // 3
        logFlowPoint("On 'Offer Selection' page click on 'Purchase as a Gift' and verify 'Friends' dialog appears"); // 4

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, gifter), true);
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyDropdownPurchaseAsGiftVisible(), true);

        // 4
        gdpActionCTA.selectDropdownPurchaseAsGift();
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        offerSelectionPage.verifyOfferSelectionPageReached();
        offerSelectionPage.clickPrimaryButton(entitlement.getOcdPath());
        logPassFail(new FriendsSelectionDialog(driver).waitIsVisible(), true);

        softAssertAll();
    }
}