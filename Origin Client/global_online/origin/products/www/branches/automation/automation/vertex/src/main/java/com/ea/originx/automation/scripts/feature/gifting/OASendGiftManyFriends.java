package com.ea.originx.automation.scripts.feature.gifting;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog.Recipient;
import com.ea.originx.automation.lib.pageobjects.dialog.GiftMessageDialog;
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
import java.util.ArrayList;
import java.util.List;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the loading of the friends selection dialog
 *
 * NEED UPDATE TO GDP
 *
 * @author jmittertreiner
 */
public class OASendGiftManyFriends extends EAXVxTestTemplate {

    @TestRail(caseId = 3087364)
    @Test(groups = {"gifting", "gifting_smoke", "int_only", "allfeaturesmoke"})
    public void testSendGiftManyFriends(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getUnentitledUserAccount();

        final List<UserAccount> friends = new ArrayList<>();
        // We need at least 3 eligible users but the rest can be any user
        for (int i = 0; i < 12; ++i) {
            final UserAccount friend = i < 3
                    ? AccountManager.getUnentitledUserAccount()
                    : AccountManager.getRandomAccount();
            friends.add(friend);
        }
        userAccount.cleanFriends();
        friends.forEach(UserAccount::cleanFriends);
        friends.forEach(userAccount::addFriend);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);
        final String entitlementName = entitlement.getName();

        logFlowPoint("Launch Origin and login as user " + userAccount.getUsername()
                + " with 12 friends and navigate to the PDP " + entitlementName); // 1
        logFlowPoint("From the buy dropdown menu and select 'Gift this game' and"
                + " verify 10 friends show up in the 'Send you Gift' dialog"); // 2
        logFlowPoint("Scroll to the bottom if the dialog and verify the remaining 2 friends load"); // 3
        logFlowPoint("Verify that selecting a friend deselects the previously selected friend"); // 4
        logFlowPoint("Enter " + friends.get(0).getUsername() + " into the search dialog and "
                + "verify it is possible to gift the game to " + friends.get(0).getUsername()); // 5

        // 1
        final WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, userAccount);
//        if (MacroPDP.loadPdpPage(driver, entitlement)) {
//            logPass("Verified login successful to user account: " + userAccount.getUsername());
//        } else {
//            logFailExit("Failed: Cannot login to user account: " + userAccount.getUsername());
//        }

        // 2
        //new PDPHeroActionCTA(driver).selectBuyDropdownPurchaseAsGift();
        FriendsSelectionDialog friendsSelectionDialog = new FriendsSelectionDialog(driver);
        List<Recipient> allRecipients = friendsSelectionDialog.getAllRecipients();
        if (Waits.pollingWait(() -> friendsSelectionDialog.getAllRecipients().size() == 10)) {
            logPass("There are 10 friends in the dialog");
        } else {
            logFailExit("There are " + allRecipients.size() + " friends in the dialog instead of 10");
        }

        // 3
        friendsSelectionDialog.scrollToBottom();
        if (Waits.pollingWait(() -> friendsSelectionDialog.getAllRecipients().size() == 12)) {
            logPass("There are 12 friends in the dialog");
        } else {
            logFail("There are " + allRecipients.size() + " friends in the dialog instead of 12");
        }

        // 4
        List<Recipient> allEligibleRecipients = friendsSelectionDialog.getAllEligibleRecipients();
        final Recipient recipient1 = allEligibleRecipients.get(0);
        final Recipient recipient2 = allEligibleRecipients.get(1);
        recipient1.select();
        recipient2.select();
        if (!recipient1.isSelected() && recipient2.isSelected()) {
            logPass("Selecting a friend deselects the previously selected friend");
        } else {
            logFail("Selecting a friend doesn't deselect the previously selected friend");
        }

        // 5
        friendsSelectionDialog.searchFor(friends.get(0).getUsername());
        Waits.pollingWait(() -> friendsSelectionDialog.getAllRecipients().size() == 1);
        friendsSelectionDialog.selectRecipient(friends.get(0));
        friendsSelectionDialog.clickNext();
        if (new GiftMessageDialog(driver).waitIsVisible()) {
            logPass("It is possible to gift to " + friends.get(0).getUsername());
        } else {
            logFail("It is not possible to gift to " + friends.get(0).getUsername());
        }

        softAssertAll();
    }
}
