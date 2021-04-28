package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.OriginAccessService;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test verifies that a user with a lapse subscription no longer has access to
 * games acquired through vault
 *
 * @author cbalea
 */
public class OAVaultContentExpiredSubscriber extends EAXVxTestTemplate {

    @TestRail(caseId = 3064021)
    @Test(groups = {"originaccess", "full_regression"})
    public void testVaultContentExpiredSubscription(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.Orwell);
        final String entitlementOfferID = entitlement.getOfferId();
        final EntitlementInfo vaultEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.ME3_DELUXE);
        final String vaultEntitlementOfferID = vaultEntitlement.getOfferId();
        final EntitlementInfo baseEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String baseEntitlementOfferID = baseEntitlement.getOfferId();

        logFlowPoint("Launch and log into 'Origin' with an account that has an expired subscription"); // 1
        logFlowPoint("Go to 'Game Library' and verify the vault tiles are grayed out"); // 2
        logFlowPoint("Verify vault game cannot be launched"); // 3
        logFlowPoint("Verify vault game can be uninstalled"); // 4
        logFlowPoint("Verify vault games cannot be downloaded"); // 5
        logFlowPoint("Verify vault games have messaging saying subscription has expired"); // 6
        logFlowPoint("Verify vault games can be removed from library"); // 7
        logFlowPoint("Verify lesser edition games cannot be upgraded to vault edition"); // 8

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // setting preconditions
        MacroPurchase.purchaseEntitlement(driver, baseEntitlement);
        MacroOriginAccess.purchaseOriginPremierAccess(driver);
        MacroGDP.loadGdpPage(driver, entitlement);
        MacroGDP.addVaultGameAndCheckViewInLibraryCTAPresent(driver);
        MacroGDP.loadGdpPage(driver, vaultEntitlement);
        MacroGDP.addVaultGameAndCheckViewInLibraryCTAPresent(driver);
        NavigationSidebar navigationSidebar = new NavigationSidebar(driver);
        navigationSidebar.gotoGameLibrary();
        MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferID);

        OriginAccessService.immediateCancelSubscription(userAccount);
        sleep(10000); // waiting for cancellation to take effect

        // 2
        GameTile gameTile = new GameTile(driver, entitlementOfferID);
        GameTile gameTile2 = new GameTile(driver, vaultEntitlementOfferID);
        logPassFail(!gameTile2.verifyTileOpacity(), false);

        // 3
        logPassFail(!gameTile.isReadyToPlay(), false);

        // 4
        logPassFail(gameTile.isUninstallable(), false);

        // 5
        logPassFail(!gameTile2.isDownloadable(), false);

        // 6
        logPassFail(gameTile2.verifyViolatorStatingMembershipIsExpired(), false);

        // 7
        logPassFail(gameTile2.verifyRemoveFromLibrary(), false);

        // 8
        logPassFail(!new GameTile(driver, baseEntitlementOfferID).verifyViolatorPresent(), true);

        softAssertAll();
    }

}
