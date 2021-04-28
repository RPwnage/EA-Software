package com.ea.originx.automation.scripts.gamelibrary;

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
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test OGD 'Remove from Library' flow
 *
 * @author cdeaconu
 */
public class OAVaultTitleRemoval extends EAXVxTestTemplate{
    
    @TestRail(caseId = 10954)
    @Test(groups = {"gamelibrary", "full_regression"})
    public void testVaultTitleRemoval(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
        final String offerId = entitlement.getOfferId();
        Criteria criteria = new Criteria.CriteriaBuilder().tag(AccountTags.SUBSCRIPTION_ACTIVE.name()).build();
        criteria.addEntitlement(entitlement.getName());
        final UserAccount userAccount = AccountManager.getInstance().requestWithCriteria(criteria);
        
        logFlowPoint("Log into with a subscriber account."); // 1
        logFlowPoint("Navigate to 'Game Library'."); // 2
        logFlowPoint("Select a vault game and open OGD."); // 3
        logFlowPoint("Click on gear icon and verify there is an option to remove the game from library."); // 4
        logFlowPoint("Select 'Remove from Library' and verify a dialog that confirms the action appears."); // 5
        logFlowPoint("Verify that there is an option to remove the specific game from entitlements"); // 6
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);
        
        // 3
        GameSlideout gameSlideout = new GameTile(driver, offerId).openGameSlideout();
        logPassFail(gameSlideout.waitForSlideout(), true);
        
        // 4
        GameSlideoutCogwheel cogwheel = gameSlideout.getCogwheel();
        logPassFail(cogwheel.verifyRemoveFromLibraryVisible(), true);
        
        // 5
        cogwheel.removeFromLibrary();
        RemoveFromLibrary removeFromLibrary = new RemoveFromLibrary(driver);
        logPassFail(removeFromLibrary.waitIsVisible(), true);
        
        // 6
        logPassFail(removeFromLibrary.verifyRemoveGameButtonVisible(), true);
        
        softAssertAll();
    }
}