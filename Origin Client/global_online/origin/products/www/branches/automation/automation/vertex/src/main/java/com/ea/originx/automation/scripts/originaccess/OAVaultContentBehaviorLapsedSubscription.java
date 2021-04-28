package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.RemoveFromLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideoutCogwheel;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
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
 * Test OGD informations for an expired subscriber account
 *
 * @author cdeaconu
 */
public class OAVaultContentBehaviorLapsedSubscription extends EAXVxTestTemplate{
    
    @TestRail(caseId = 10953)
    @Test(groups = {"originaccess", "browser_only", "full_regression"})
    public void testOAVaultContentBehaviorLapsedSubscription(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_EXPIRED_OWN_ENT);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
        final String offerId = entitlement.getOfferId();
        
        logFlowPoint("Log into with an expired subscription."); // 1
        logFlowPoint("Navigate to 'Game Library'."); // 2
        logFlowPoint("Select a vault game, open OGD and verify 'Origin Access' badge is visible."); // 3
        logFlowPoint("Verify there is message stating 'Origin Access' membership has expired for the vault entitlement."); // 4
        logFlowPoint("Verify there is a link to resubscribe."); // 5
        logFlowPoint("Verify there is a link to buy the game."); // 6
        logFlowPoint("Verify the lapsed subscriber no longer has an option to upgrade their owned title to the edition offered in the 'Vault'."); // 7
        logFlowPoint("Verify there is an option to remove the game from library."); // 8
        logFlowPoint("Verify that the user can remove vault content from 'Game library'."); // 9
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);
        
        // 3
        GameSlideout gameSlideout = new GameTile(driver, offerId).openGameSlideout();
        logPassFail(gameSlideout.verifyOriginAccessBadgeVisible(), false);
        
        // 4
        logPassFail(gameSlideout.verifyOriginAccessExpiredTextVisible(), false);
        
        // 5
        logPassFail(gameSlideout.verifyRenewMembershipLinkVisible(), false);
        
        // 6
        logPassFail(gameSlideout.verifyBuyTheGameLinkVisible(), false);
        
        // 7
        logPassFail(!gameSlideout.verifyUpgradeNowLinkVisible(), false);
        
        // 8
        GameSlideoutCogwheel cogwheel = gameSlideout.getCogwheel();
        logPassFail(cogwheel.verifyRemoveFromLibraryVisible(), false);
        
        // 9
        cogwheel.removeFromLibrary();
        logPassFail(new RemoveFromLibrary(driver).waitIsVisible(), true);
        
        softAssertAll();
    }
}