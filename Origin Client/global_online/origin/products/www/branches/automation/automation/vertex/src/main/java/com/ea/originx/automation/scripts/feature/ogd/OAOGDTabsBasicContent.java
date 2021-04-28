package com.ea.originx.automation.scripts.feature.ogd;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests basic content in the Origin Game Details tab
 *
 * @author jmittertreiner
 */
public class OAOGDTabsBasicContent extends EAXVxTestTemplate {

    @TestRail(caseId = 3103324)
    @Test(groups = {"ogd_smoke", "ogd", "allfeaturesmoke"})
    public void testOGDTabsBasicContent(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final UserAccount friendAccount = AccountManager.getEntitledUserAccount(entitlement);
        userAccount.cleanFriends();
        friendAccount.cleanFriends();
        userAccount.addFriend(friendAccount);

        logFlowPoint("Launch Origin and login as user (" + userAccount.getUsername() + ") entitled to " + entitlement.getName()
                + ", with a friend (" + friendAccount + ") also entitled to " + entitlement.getName()); // 1
        logFlowPoint("Navigate to the game library and open the game slide out for " + entitlement.getName()); // 2
        logFlowPoint("Open the Achievements tab and verify there are achievements"); // 3
        logFlowPoint("Open the Friends Who Play tab and verify the friend exists in that tab"); // 4
        logFlowPoint("Open the extra content tab and verify some addons or expansions exist in the tab"); // 5

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();
        GameSlideout gameSlideout = new GameTile(driver, entitlement.getOfferId()).openGameSlideout();
        logPassFail(gameSlideout.waitForSlideout(), true);

        // 3
        gameSlideout.clickAchievementsTab();
        logPassFail(Waits.pollingWait(gameSlideout::verifyAchievementsInAchievementsTab), false);

        // 4
        gameSlideout.clickFriendsWhoPlayTab();
        logPassFail(Waits.pollingWait(gameSlideout::verifyFriendsInFriendsWhoPlayTab), false);

        // 5
        gameSlideout.clickExtraContentTab();
        logPassFail(Waits.pollingWait(gameSlideout::verifyExtraContentInExtraContentTab), true);

        client.stop();
        softAssertAll();
    }
}
