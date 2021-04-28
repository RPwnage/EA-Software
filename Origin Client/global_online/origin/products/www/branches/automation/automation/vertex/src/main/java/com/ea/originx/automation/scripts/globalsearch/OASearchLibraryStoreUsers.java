package com.ea.originx.automation.scripts.globalsearch;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;
import java.util.ArrayList;
import java.util.List;

/**
 * Verifies search for library, store, and users
 *
 * @author caleung
 */
public class OASearchLibraryStoreUsers extends EAXVxTestTemplate {

    @TestRail(caseId = 1016736)
    @Test(groups = {"globalsearch", "services_smoke"})
    public void testSearchLibraryStoreUsers(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_DIGITAL_DELUXE);
        final String entitlementName = entitlement.getName();
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.ENTITLEMENTS_DOWNLOAD_PLAY_TEST_ACCOUNT);
        final UserAccount otherUserAccount = AccountManager.getRandomAccount();
        final String otherUserAccountName = otherUserAccount.getUsername();
        final String otherUserAccountRealName = otherUserAccount.getFirstName() + " " + otherUserAccount.getLastName();

        logFlowPoint("Log into Origin with User"); // 1
        logFlowPoint("Search for User B using their real name and verify that results are shown."); // 2
        logFlowPoint("Search for User B using their Origin ID and verify that results are shown and profile can be selected."); // 3
        logFlowPoint("Search for 'Unravel' and verify 'Unravel' can be searched by title."); // 4
        logFlowPoint("Verify 'Unravel' shows up in the results."); // 5
        logFlowPoint("Search for '" + entitlementName + "'."); // 6
        logFlowPoint("Verify '" + entitlementName + "' is shown under 'Library' results."); // 7
        logFlowPoint("Verify " + entitlementName + "' is shown under 'Store' results."); // 8
        logFlowPoint("Search for 'gibberish' and verify that 'No Results Found' message is shown."); // 9

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        GlobalSearch globalSearch = new GlobalSearch(driver);
        globalSearch.enterSearchText(otherUserAccountRealName);
        GlobalSearchResults searchResults = new GlobalSearchResults(driver);
        logPassFail(searchResults.verifySearchDisplayed(), true);

        // 3
        searchResults.clickViewAllPeopleResults();
        MacroSocial.navigateToUserProfileThruSearch(driver, otherUserAccount);
        ProfileHeader profileHeader = new ProfileHeader(driver);
        boolean onProfilePage = profileHeader.verifyOnProfilePage();
        boolean correctUserProfile = profileHeader.verifyUsername(otherUserAccountName);
        logPassFail(onProfilePage && correctUserProfile, true);

        // 4
        globalSearch.enterSearchText("Unravel");
        logPassFail(searchResults.verifySearchDisplayed(), true);

        // 5
        logPassFail(searchResults.verifyStoreResults("Unravel"), true);

        // 6
        globalSearch.enterSearchText(entitlementName);
        logPassFail(searchResults.verifySearchDisplayed(), true);

        // 7
        logPassFail(searchResults.verifyGameLibraryContainsOffer(entitlementName), true);

        // 8
        List<String> list = new ArrayList<>();
        list.add(entitlementName);
        logPassFail(searchResults.verifyAnyTitleContainAnyOf(list), true);

        // 9
        globalSearch.enterSearchText("gibberishgibberish");
        logPassFail(searchResults.verifyNoResultsDisplayed(), true);

        softAssertAll();
    }
}