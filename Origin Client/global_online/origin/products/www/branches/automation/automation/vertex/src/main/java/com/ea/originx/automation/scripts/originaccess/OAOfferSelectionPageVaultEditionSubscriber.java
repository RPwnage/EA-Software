package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
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
 * Test verifies OSP is reached when attempting to buy a vault game as a
 * subscriber
 *
 * @author cbalea
 */
public class OAOfferSelectionPageVaultEditionSubscriber extends EAXVxTestTemplate {

    @TestRail(caseId = 3064503)
    @Test(groups = {"originaccess", "full_regression"})
    public void testOfferSelectionPageVaultEditionSubscriber(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.PREMIER_SUBSCRIPTION_ACTIVE_NO_ENTITLEMENTS);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.TITANFALL_2);

        logFlowPoint("Launch and log into 'Origin' with a premier subscriber account"); // 1
        logFlowPoint("Navigate to the GDP of an entitlement with multiple editions"); // 2
        logFlowPoint("Click on the drop-down arrow, select 'Buy' and verify that the user is brought to 'Offer Selection Page"); // 3

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 3
        new GDPActionCTA(driver).selectDropdownBuy();
        logPassFail(new OfferSelectionPage(driver).verifyOfferSelectionPageReached(), true);

        softAssertAll();

    }

}
