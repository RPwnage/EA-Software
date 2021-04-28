package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import java.util.List;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the Gift User Dialog and the search bar
 *
 * NEEDS UPDATE TO GDP
 *
 * @author nvarthakavi
 */
public class OAGiftingSelectUserModal extends EAXVxTestTemplate {

    @TestRail(caseId = 40229)
    @Test(groups = {"gifting", "full_regression", "int_only"})
    public void testGiftingSelectUserModal(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount gifter = AccountManagerHelper.getUserAccountWithCountry("Canada");
        final UserAccount giftee = AccountManagerHelper.getTaggedUserAccountWithCountry(AccountTags.GIFTING_KNOWN_NAME, "Canada");
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);
        final String entitlementName = entitlement.getName();
        final String gifterName = gifter.getUsername();
        final String gifteeName = giftee.getUsername();

        logFlowPoint(String.format("Launch Origin and login as user gifter '%s' and navigate to the PDP of '%s'", gifterName, entitlementName)); // 1
        logFlowPoint("From the buy drop down menu, select 'Gift this game' and verify the friends selection dialog appears"); // 2
        logFlowPoint("Verify the title as 'Who would you like to send your gift to', the search bar and the user's friends list appears"); // 3
        logFlowPoint(String.format("Search for gifter '%s' by id in the search bar and verify only user giftee '%s' is displayed in friends list", gifteeName, gifteeName));// 4
        logFlowPoint(String.format("Search for gifter '%s' by name in the search bar and verify only user giftee '%s' is displayed in friends list", gifteeName, gifteeName));// 5
        logFlowPoint("Search for a user that does not exist('nouserfound') in the search bar and verify only 'no results are found' message is displayed in friends list");// 6
        logFlowPoint(String.format("Search and select the user giftee '%s' to send a gift and verify the next button is enabled", gifteeName)); // 7

        // 1
        // Create the giftee and Login to gifter
        final WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, giftee);
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.selectViewMyProfile();
        final String userFirstName = new ProfileHeader(driver).getUserRealName().split(" ")[0];
        miniProfile.selectSignOut();
        giftee.cleanFriends();
        gifter.cleanFriends();
        Criteria criteria = new Criteria.CriteriaBuilder().country("Canada").build();
        List<UserAccount> friends = UserAccountHelper.addMultipleFriendsWithCriteria(gifter, 10, criteria);

        MacroLogin.startLogin(driver, gifter);

//        if (MacroPDP.loadPdpPage(driver, entitlement)) {
//            logPass("Successfully logged into " + gifterName + " and navigated to " + entitlementName + "'s PDP");
//        } else {
//            logFailExit("Failed to log into " + gifterName + " and navigate to " + entitlementName + "'s PDP");
//        }

        // 2
       // new PDPHeroActionCTA(driver).selectBuyDropdownPurchaseAsGift();
        FriendsSelectionDialog friendsSelectionDialog = new FriendsSelectionDialog(driver);
        if (friendsSelectionDialog.waitIsVisible()) {
            logPass("Successfully selected 'Gift this game' from the dropdown menu");
        } else {
            logFailExit("Failed to select 'Gift this game' from the dropdown menu");
        }

        // 3
        boolean titleCheck = friendsSelectionDialog.verifyFriendsDialogTitle();
        boolean searchBarCheck = friendsSelectionDialog.verifySearchBarExists();
        boolean friendsListCheck = friendsSelectionDialog.verifyRecipientsListExists();
        if (titleCheck && searchBarCheck && friendsListCheck) {
            logPass("Verified the title as 'Who would you like to send your gift to', the search bar and the user's friends list appears");
        } else {
            logFailExit("Failed : The title as 'Who would you like to send your gift to'or the search bar or the user's friends list did not appear");
        }

        // 4
        friendsSelectionDialog.searchFor(giftee.getUsername());
        boolean friendsSearchById = Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(giftee), 30000, 2000, 0);
        if (friendsSearchById) {
            logPass(String.format("Verified the '%s' appears in the friends list", gifteeName));
        } else {
            logFail(String.format("Failed : the '%s' appears in the friends list does not appear", gifteeName));
        }

        // 5
        friendsSelectionDialog.clearSearch();
        friendsSelectionDialog.searchFor(userFirstName);
        boolean friendsSearchByName = Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(giftee), 30000, 2000, 0);
        if (friendsSearchByName) {
            logPass(String.format("Verified the '%s' appears in the friends list", gifteeName));
        } else {
            logFail(String.format("Failed : the '%s' appears in the friends list does not appear", gifteeName));
        }

        // 6
        friendsSelectionDialog.clearSearch();
        friendsSelectionDialog.searchFor("nouserfound");
        boolean friendsSearchNoUser = Waits.pollingWait(friendsSelectionDialog::verifyNoUserFoundExists, 30000, 2000, 0);
        if (friendsSearchNoUser) {
            logPass(String.format("Verified on searching a user that does not exist('nouserfound'), a 'no results are found' message is displayed in friends list"));
        } else {
            logFail(String.format("Failed : on searching a user that does not exist('nouserfound'), a 'no results are found' message is not displayed in friends list"));
        }

        // 7
        friendsSelectionDialog.clearSearch();
        friendsSelectionDialog.searchFor(giftee.getUsername());
        Waits.pollingWait(() -> friendsSelectionDialog.getAllRecipients().size() == 1);
        friendsSelectionDialog.selectRecipient(giftee);
        boolean friendsSearchGiftee = Waits.pollingWait(friendsSelectionDialog::isNextButtonEnabled, 30000, 2000, 0);
        if (friendsSearchGiftee) {
            logPass(String.format("Verified On clicking the '%s' the next button is enabled", gifteeName));
        } else {
            logFail(String.format("Failed : On clicking the '%s' the next button is disabled", gifteeName));
        }

        client.stop();
        softAssertAll();

    }
}
