package com.ea.originx.automation.scripts.checkout;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.SelectOriginAccessPlanDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessNUXFullPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the NUX dialog that appears after purchasing the subscription
 *
 * @author nvarthakavi
 * @author mdobre
 */
public class OAPurchaseSubscriptionNUX extends EAXVxTestTemplate {

    @TestRail(caseId = 1016690)
    @Test(groups = {"checkout", "release_smoke", "int_only"})
    public void testPurchaseSubscriptionNUX(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        logFlowPoint("Login as a newly registered user"); // 1
        logFlowPoint("Navigate to the 'Origin Access' landing page"); // 2
        logFlowPoint("Click the 'Join Premier Today' CTA and verify purchase flow starts with plan selection modal"); // 3
        logFlowPoint("Select monthly subscription, complete the purchase flow and verify purchase was successful"); // 4
        logFlowPoint("Verify NUX is displayed after the purchase"); // 5
        logFlowPoint("Click on 'Skip and go to Vault' and entitle a vault game"); // 6

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        new NavigationSidebar(driver).gotoOriginAccessPage();
        OriginAccessPage originAccessPage = new OriginAccessPage(driver);
        logPassFail(originAccessPage.verifyPageReached(), true);

        // 3
        originAccessPage.clickJoinPremierButton();
        SelectOriginAccessPlanDialog selectOriginAccessPlanDialog = new SelectOriginAccessPlanDialog(driver);
        logPassFail(selectOriginAccessPlanDialog.waitIsVisible(), true);

        // 4
        selectOriginAccessPlanDialog.selectPlan(SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.MONTHLY_PLAN);
        selectOriginAccessPlanDialog.clickNext();
        logPassFail(MacroPurchase.completePurchase(driver), true);

        // 5
        OriginAccessNUXFullPage originAccessNUXFullPage = new OriginAccessNUXFullPage(driver);
        logPassFail(originAccessNUXFullPage.verifyPageReached(), true);

        // 6
        originAccessNUXFullPage.clickSkipToOriginAccessCollection();
        VaultPage vaultPage = new VaultPage(driver);
        vaultPage.waitForGamesToLoad();
        String firstEntitlementOfferId = vaultPage.getFirstVaultTileOfferId();
        MacroOriginAccess.addEntitlementAndHandleDialogs(driver, firstEntitlementOfferId);
        new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(new GameTile(driver, firstEntitlementOfferId).isGameTileVisible(), true);

        softAssertAll();
    }

}
