package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog.Recipient;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;

import java.util.ArrayList;
import java.util.List;

import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test adding to the wishlist through the Origin Game Details page
 *
 * @author jmittertreiner
 */
public class OAGiftingFriendsList extends EAXVxTestTemplate {

    @TestRail(caseId = 40231)
    @Test(groups = {"gifting", "full_regression"})
    public void testGiftingFriendsList(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final UserAccount noFriends = AccountManager.getRandomAccount();
        final UserAccount manyFriends = AccountManager.getRandomAccount();

        // Origin shows up to 10 friends by default. If we want to guarantee that
        // at least two of each type (eligible, on wishlist, ineligible) show up
        // we need at least three of each type if we have 11 friends
        List<UserAccount> friends = new ArrayList<>();
        for (int i = 0; i < 11; ++i) {
            if (i < 5) {
                friends.add(AccountManager.getRandomAccount());
            } else if (i < 8) {
                friends.add(AccountManager.getEntitledUserAccount(entitlement));
            } else {
                friends.add(AccountManagerHelper.getTaggedUserAccount(AccountTags.BF4_ON_WISHLIST));
            }
        }

        noFriends.cleanFriends();
        manyFriends.cleanFriends();
        friends.forEach(UserAccount::cleanFriends);
        UserAccountHelper.addFriends(manyFriends, friends.toArray(new UserAccount[0]));

        logFlowPoint("Launch Origin and login as " + manyFriends.getUsername() + " who has 11 friends "
                + "some of which have " + entitlement.getName() + " on their wishlist and so which own " + entitlement.getName()); // 1
        logFlowPoint("Navigate to the GDP of BF4 and Click Gift this item"); // 2
        logFlowPoint("Verify: that the " + manyFriends.getUsername() + " friends list is displayed"); // 3
        logFlowPoint("Verify: that the " + manyFriends.getUsername() + " friends list is sorted in alphabetical order"); // 4
        logFlowPoint("Verify: that long friends list will lazy load the rest of their friends as the user scrolls down the modal"); // 5
        logFlowPoint("Verify: that ineligible users are not clickable"); // 6
        logFlowPoint("Verify: that if a user has the game on their wishlist, messaging appears beside the + button"); // 7
        logFlowPoint("Verify: that only one user can be selected at a time"); // 8
        logFlowPoint("Logout of " + manyFriends.getUsername() + " and Login as " + noFriends.getUsername() + ", an account with no friends"); // 9
        logFlowPoint("Navigate to the GDP Page of BF4 and Click Gift this item CTA"); // 10
        logFlowPoint("Verify: that no friends appear"); // 11

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, manyFriends), true);

        // 2
        MacroGDP.loadGdpPage(driver, entitlement);
        new GDPActionCTA(driver).selectDropdownPurchaseAsGift();
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        offerSelectionPage.verifyOfferSelectionPageReached();
        offerSelectionPage.clickPrimaryButton(entitlement.getOcdPath());
        FriendsSelectionDialog selectionDialog = new FriendsSelectionDialog(driver);
        logPassFail(selectionDialog.waitIsVisible(), true);

        // 3
        logPassFail(selectionDialog.verifyRecipientsListExists(), false);

        // 4
        logPassFail(selectionDialog.verifyRecipientsSorted(), false);

        // 5
        selectionDialog.scrollToBottom();
        logPassFail(Waits.pollingWait(() -> selectionDialog.getAllRecipients().size() > 10), false);

        // 6
        Recipient ineligibleRecipient = selectionDialog.getAllIneligibleRecipients().get(0);
        ineligibleRecipient.toggleSection();
        logPassFail(!ineligibleRecipient.isSelected(), false);

        // 7
        Recipient wishlistBf4Tile = selectionDialog.getAllOnWishlistRecipients().get(0);
        logPassFail(wishlistBf4Tile.getStatusDetails().isPresent(), false);

        // 8
        List<Recipient> eligibleRecipients = selectionDialog.getAllEligibleRecipients();
        Recipient r1 = eligibleRecipients.get(0);
        Recipient r2 = eligibleRecipients.get(1);
        r1.select();
        r2.select();
        logPassFail(!r1.isSelected() && r2.isSelected(), false);
        selectionDialog.closeAndWait();

        // 9
        new MiniProfile(driver).selectSignOut();
        logPassFail(MacroLogin.startLogin(driver, noFriends), true);

        // 10
        MacroGDP.loadGdpPage(driver, entitlement);
        new GDPActionCTA(driver).selectDropdownPurchaseAsGift();
        offerSelectionPage.verifyOfferSelectionPageReached();
        offerSelectionPage.clickPrimaryButton(entitlement.getOcdPath());
        logPassFail(selectionDialog.waitIsVisible(), true);

        // 11
        logPassFail(Waits.pollingWait(() -> selectionDialog.getAllRecipients().size() == 0), true);

        softAssertAll();
    }
}
