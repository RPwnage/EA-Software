package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGifting;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroProfile;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.NotificationCenter;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileGamesTab;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.AccountService;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test unopened 'Gifts' are not shown as owned outside of 'Notification Center'
 * Receiving a Gift
 *
 * NEEDS UPDATE TO GDP
 *
 * @author sbentley
 */
public class OAGiftHidden extends EAXVxTestTemplate {

    @TestRail(caseId = 36811)
    @Test(groups = {"gifting", "client_only", "full_regression"})
    public void testGiftRNATile(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount senderAccount = AccountManagerHelper.getUnentitledUserAccountWithCountry("Canada");
        final UserAccount receiverAccount = AccountManagerHelper.createNewThrowAwayAccount("Canada");
        AccountService.registerNewUserThroughREST(receiverAccount, null);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BIG_MONEY);
        final String entitlementName = entitlement.getName();

        senderAccount.cleanFriends();

        logFlowPoint("Login to an account with an unopened gift"); // 1
        logFlowPoint("Hover over 'Notification Center' and verify unopened gift appears in 'Notification Center'"); // 2
        logFlowPoint("Navigate to 'Game Library' and verify unopened gift does not appear");    // 3
        logFlowPoint("Navigate to 'Profile' -> 'Games' section and verify unopoened gift does not appear"); // 4
        logFlowPoint("Search for game and verify unopened gift does not appear under 'Game Library' search results");   // 5

        // 1
        final WebDriver driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, "/can/en-us");
        MacroLogin.startLogin(driver, senderAccount);
        senderAccount.addFriend(receiverAccount);
        MacroGifting.purchaseGiftForFriend(driver, entitlement, receiverAccount.getUsername());
        new MiniProfile(driver).selectSignOut();

        if (MacroLogin.startLogin(driver, receiverAccount)) {
            logPass("Successfully logged into a user with pending gift: " + receiverAccount.getUsername());
        } else {
            logFailExit("Could not log into a user with pending gift: " + receiverAccount.getUsername());
        }

        //2
        NotificationCenter notificationCenter = new NotificationCenter(driver);
        notificationCenter.expandNotificationCenter();
        if (notificationCenter.verifyGiftReceivedVisible(senderAccount.getUsername())) {
            logPass("Successfully received gift from: " + senderAccount.getUsername());
        } else {
            logFailExit("Failed to receive gift from: " + senderAccount.getUsername());
        }

        //3
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        if (!gameLibrary.isGameTileVisibleByName(entitlementName)) {
            logPass("Unopened gift not visible in 'Game Library'");
        } else {
            logFail("Unopened gift visible in 'Game Library'");
        }

        //4
        MacroProfile.navigateToGameSection(driver);
        ProfileGamesTab profileGamesTab = new ProfileGamesTab(driver);
        if (profileGamesTab.verifyNoGamesMessage(true) || !profileGamesTab.verifyEntitlementExists(entitlementName)) {
            logPass("Successfully verified unopened gift does not appear in 'Game' section of 'Profile'");
        } else {
            logFail("Unopened gift appears in 'Game' section of 'Profile'");
        }

        //5
        new GlobalSearch(driver).addTextToSearch(entitlementName);
        GlobalSearchResults globalSearchResults = new GlobalSearchResults(driver);
        if (globalSearchResults.verifyGameLibraryResultsNotDisplayed() || !globalSearchResults.verifyGameLibraryResults(entitlementName)) {
            logPass("Successfully verified unopened gift does not appear in Game Library search results");
        } else {
            logFail("Unopened gift appeared in Game Library search results");
        }

        softAssertAll();
    }
}
