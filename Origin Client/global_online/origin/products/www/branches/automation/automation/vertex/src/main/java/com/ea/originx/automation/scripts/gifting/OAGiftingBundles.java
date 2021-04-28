package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroGifting;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import static com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog.Eligibility.*;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
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
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the eligibility of users when receiving bundles
 *
 * NEEDS UPDATE TO GDP
 *
 * @author jmittertreiner
 */
public class OAGiftingBundles extends EAXVxTestTemplate {

    @TestRail(caseId = 11786)
    @Test(groups = {"gifting", "full_regression", "int_only"})
    public void testGiftingBundles(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo theSims3 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS3_STARTER_PACK);
        final EntitlementInfo popcapBundle = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.POPCAP_HERO_BUNDLE);

        final UserAccount currentUser = AccountManager.getInstance().requestWithCriteria(new Criteria.CriteriaBuilder().country("Canada").build());
        final UserAccount newAccount = AccountManagerHelper.createNewThrowAwayAccount("Canada");
        final UserAccount accessAccount = AccountManagerHelper.getTaggedUserAccountWithCountry(AccountTags.SUBSCRIPTION_ACTIVE, "Canada");

        final UserAccount threeGameAccount = AccountManagerHelper.getTaggedUserAccountWithCountry(AccountTags.THREE_OF_THREE_BUNDLE, "Canada");
        final UserAccount twoGameAccount = AccountManagerHelper.getTaggedUserAccountWithCountry(AccountTags.TWO_OF_THREE_BUNDLE, "Canada");
        final UserAccount oneGameAccount = AccountManagerHelper.getTaggedUserAccountWithCountry(AccountTags.ONE_OF_THREE_BUNDLE, "Canada");

        // 1
        final WebDriver driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, "/can/en-us");
        MacroLogin.startLogin(driver, newAccount);
        new MiniProfile(driver).selectSignOut();

        currentUser.cleanFriends();
        accessAccount.cleanFriends();
        threeGameAccount.cleanFriends();
        twoGameAccount.cleanFriends();
        oneGameAccount.cleanFriends();
        UserAccountHelper.addFriends(currentUser, newAccount, accessAccount, threeGameAccount, twoGameAccount, oneGameAccount);

        logFlowPoint("Launch Origin and login as user " + currentUser.getUsername()); // 1
        logFlowPoint("Navigate to the PDP of " + theSims3.getName() + " and click gift this game"); // 2
        logFlowPoint("Verify an account with no games is giftable (" + newAccount.getUsername() + ")"); // 3
        logFlowPoint("Verify an account with origin access ( " + accessAccount.getUsername() + " ) is giftable"); // 4
        logFlowPoint("Navigate to the PDP of " + popcapBundle.getName() + " and click gift this game"); // 5
        logFlowPoint("Verify an account which owns just Bejeweled 3 (" + oneGameAccount.getUsername() + ") is giftable"); // 6
        logFlowPoint("Verify an account which owns just Bejeweled 3 and Peggle (" + twoGameAccount.getUsername() + ")is giftable"); // 7
        logFlowPoint("Verify an account which owns Bejeweled 3, Plants Versus Zombies, and Peggle (" + threeGameAccount.getUsername() + ")is not giftable"); // 8
        logFlowPoint("Complete the gifting flow to send the bundle as a gift to the new account (" + newAccount.getUsername() + ")"); // 9
        logFlowPoint("Navigate to the PDP of " + theSims3.getName() + " and click gift this game"); // 10
        logFlowPoint("Verify the new account(" + newAccount.getUsername() + ") is not giftable"); // 11

        // 1
        if (MacroLogin.startLogin(driver, currentUser)) {
            logPass("Verified login successful to user account: " + currentUser.getUsername());
        } else {
            logFailExit("Failed: Cannot login to user account: " + currentUser.getUsername());
        }

        // 2
      //  new PDPHeroActionCTA(driver).selectBuyDropdownPurchaseAsGift();
        FriendsSelectionDialog friendsSelectionDialog = new FriendsSelectionDialog(driver);
        if (friendsSelectionDialog.waitIsVisible()) {
            logPass("Successfully clicked gift this game");
        } else {
            logFailExit("Failed to click gift this game");
        }

        // 3
        if (Waits.pollingWait(() -> friendsSelectionDialog.getRecipient(newAccount).getEligibility() == ELIGIBLE)) {
            logPass("A user with no games (" + newAccount.getUsername() + ") is eligible");
        } else {
            logFail("A user with no games (" + newAccount.getUsername() + ") is not eligible");
        }

        // 4
        if (Waits.pollingWait(() -> friendsSelectionDialog.getRecipient(accessAccount).getEligibility() == ELIGIBLE)) {
            logPass("A user with origin access (" + accessAccount.getUsername() + ") is eligible");
        } else {
            logFail("A user with origin access (" + accessAccount.getUsername() + ") is not eligible");
        }
        friendsSelectionDialog.closeAndWait();

        // 5
      //  PDPHeroActionCTA popcapPDPCTA = new PDPHeroActionCTA(driver);
        //  popcapPDPCTA.selectBuyDropdownPurchaseAsGift();
        if (friendsSelectionDialog.waitIsVisible()) {
            logPass("Successfully clicked gift this game");
        } else {
            logFailExit("Failed to click gift this game");
        }

        // 6
        if (Waits.pollingWait(() -> friendsSelectionDialog.getRecipient(oneGameAccount).getEligibility() == ELIGIBLE)) {
            logPass("A user with one game of a bundle (" + oneGameAccount.getUsername() + ") is eligible");
        } else {
            logFail("A user with one game of a bundle (" + oneGameAccount.getUsername() + ") is not eligible");
        }

        // 7
        if (Waits.pollingWait(() -> friendsSelectionDialog.getRecipient(twoGameAccount).getEligibility() == ELIGIBLE)) {
            logPass("A user with two games of a bundle (" + twoGameAccount.getUsername() + ") is eligible");
        } else {
            logFail("A user with two games of a bundle (" + twoGameAccount.getUsername() + ") is not eligible");
        }

        // 8
        if (Waits.pollingWait(() -> friendsSelectionDialog.getRecipient(threeGameAccount).getEligibility() == ALREADY_OWNS)) {
            logPass("A user with all games of a bundle (" + threeGameAccount.getUsername() + ") is not eligible");
        } else {
            logFail("A user with all games of a bundle (" + threeGameAccount.getUsername() + ") is eligible");
        }

        // 9
        friendsSelectionDialog.close();
        if (MacroGifting.purchaseGiftForFriend(driver, popcapBundle, newAccount.getUsername())) {
            logPass("Successfully gifted " + popcapBundle.getName() + " to " + newAccount.getUsername());
        } else {
            logFailExit("Failed to gift " + popcapBundle.getName() + " to " + newAccount.getUsername());
        }

        // 10
     ///   popcapPDPCTA.selectBuyDropdownPurchaseAsGift();
        if (friendsSelectionDialog.waitIsVisible()) {
            logPass("Successfully clicked gift this game");
        } else {
            logFailExit("Failed to click gift this game");
        }

        // 11
        if (Waits.pollingWait(() -> friendsSelectionDialog.getRecipient(newAccount).getEligibility() == INELIGIBLE)) {
            logPass("Buying the game for a user makes them ineligible");
        } else {
            logFail("Buying the game for a user does not make them eligible");
        }
        softAssertAll();
    }
}
