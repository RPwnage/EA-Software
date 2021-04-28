package com.ea.originx.automation.scripts.profile;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
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
 * Tests a user can view another user, who have privacy settings set to
 * 'Everyone', owned games
 *
 * @author cdeaconu
 */
public class OAOwnedGamesApppearInProfile extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3250253)
    @Test(groups = {"profile", "full_regression", "release_smoke"})
    public void testOwnedGamesApppearInProfile(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userA = AccountManager.getRandomAccount();
        final UserAccount userB = AccountManagerHelper.getTaggedUserAccount(AccountTags.ENTITLEMENTS_DOWNLOAD_PLAY_TEST_ACCOUNT);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_DIGITAL_DELUXE);
        String offerId = entitlement.getOfferId();
        
        logFlowPoint("Login with Account B."); // 1
        logFlowPoint("Go to 'Game Library' and verify Account B owns some games."); // 2
        logFlowPoint("Log out of Account B and login with Account A."); // 3
        logFlowPoint("Go to account B's profile and click the games tab."); // 4
        logFlowPoint("Verify games that Account B owns appear in this section."); // 5
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userB), true);
        
        // 2
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        gameLibrary.verifyGameLibraryPageReached();
        logPassFail(gameLibrary.isGameTileVisibleByOfferID(offerId), true);
        
        // 3
        new MiniProfile(driver).selectSignOut();
        logPassFail(MacroLogin.startLogin(driver, userA), true);
        
        // 4
        MacroSocial.navigateToUserProfileThruSearch(driver, userB);
        ProfileHeader profileHeaderAccountA = new ProfileHeader(driver);
        profileHeaderAccountA.openGamesTab();
        logPassFail(profileHeaderAccountA.verifyGamesTabActive(), true);
        
        // 5
        logPassFail(profileHeaderAccountA.isGameTileVisibleByOfferID(offerId), true);
        
        softAssertAll();
    }
}