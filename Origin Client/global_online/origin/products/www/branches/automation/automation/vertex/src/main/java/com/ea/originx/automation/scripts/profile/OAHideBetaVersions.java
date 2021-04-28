package com.ea.originx.automation.scripts.profile;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileGamesTab;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the profile games tab to ensure that the beta versions of a game are not displayed.
 *
 * @author esdecastro
 */
public class OAHideBetaVersions extends EAXVxTestTemplate{

    @TestRail(caseId = 2224418)
    @Test(groups = {"profile", "full_regression"})
    public void testHideBetaVersions(ITestContext context) throws Exception{

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount accountA = AccountManagerHelper.getTaggedUserAccount(AccountTags.BETA_RELEASE_ENTITLEMENT);
        final UserAccount accountB = AccountManagerHelper.getTaggedUserAccount(AccountTags.BETA_RELEASE_ENTITLEMENT);

        logFlowPoint("Log on to Origin on an account with beta and a full game version of a game"); //1
        logFlowPoint("Search for another user with beta and a full game version of a game"); // 2
        logFlowPoint("Navigate to the other user's profile"); // 3
        logFlowPoint("Navigate to the 'Games' section of the user's profile"); // 4
        logFlowPoint("Verify the full version of the game appears in the list of entitlements"); // 5
        logFlowPoint("Verify the beta version of the game does not appear in the list of entitlements"); // 6
        logFlowPoint("Verify that the X of Y value for games shown does not count the beta entitlements"); // 7

        // Log into beta account and grab data to
        final WebDriver driver = startClientObject(context, client);

        // 1
        logPassFail(MacroLogin.startLogin(driver, accountA),true);

        // 2
        new GlobalSearch(driver).enterSearchText(accountB.getEmail()); //Searching for email because the account is not appearing when searching by username
        GlobalSearchResults globalSearchResults = new GlobalSearchResults(driver);
        logPassFail(globalSearchResults.verifyUserFound(accountB), true);

        // 3
        new GlobalSearchResults(driver).viewProfileOfUser(accountB);
        ProfileHeader profileHeader = new ProfileHeader(driver);
        logPassFail(profileHeader.verifyOnProfilePage(), true);

        // 4
        profileHeader.openGamesTab();
        logPassFail(profileHeader.verifyGamesTabActive(), true);

        // 5
        ProfileGamesTab profileGamesTab = new ProfileGamesTab(driver);
        logPassFail(Waits.pollingWait(() -> profileGamesTab.verifyEntitlementExists(EntitlementId.DIP_SMALL.getName())), true);

        // 6
        logPassFail(Waits.pollingWait(() -> !profileGamesTab.verifyEntitlementExists(EntitlementId.BETA_DIP_SMALL.getName())), true);

        // 7
        logPassFail(Waits.pollingWait(() -> profileGamesTab.verifyGamesYouBothOwnHeaderDisplayCountNumber()), true);

        softAssertAll();
    }
}