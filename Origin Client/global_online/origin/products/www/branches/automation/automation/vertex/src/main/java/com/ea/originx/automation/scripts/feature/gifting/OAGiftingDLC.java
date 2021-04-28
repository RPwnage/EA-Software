package com.ea.originx.automation.scripts.feature.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.Battlefield4;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.AccountService;
import com.ea.vx.originclient.utils.EntitlementService;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the eligibility of sending users DLC as a gift
 *
 * NEEDS UPDATE TO GDP
 *
 * @author nvarthakavi
 */
public class OAGiftingDLC extends EAXVxTestTemplate {

    @TestRail(caseId = 11785)
    @Test(groups = {"gifting", "gifting_smoke", "full_regression", "int_only", "allfeaturesmoke"})
    public void testGiftingDLC(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        // Entitlement setup
        final String dlcOfferId = Battlefield4.BF4_FINAL_STAND_OFFER_ID;
        final String dlcName = Battlefield4.BF4_FINAL_STAND_NAME;
        EntitlementInfo baseGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String baseGameOfferId = baseGame.getOfferId();

        //User account setup
        final UserAccount senderAccount = AccountManagerHelper.getUnentitledUserAccountWithCountry("Canada");
        final UserAccount friendOwnBase = AccountManagerHelper.createNewThrowAwayAccount("Canada");
        AccountService.registerNewUserThroughREST(friendOwnBase, null);
        EntitlementService.grantEntitlement(friendOwnBase, baseGameOfferId);
        final UserAccount friendDoesNotOwnBase = AccountManagerHelper.createNewThrowAwayAccount("Canada");
        AccountService.registerNewUserThroughREST(friendDoesNotOwnBase, null);
        final UserAccount friendOwnsDLC = AccountManagerHelper.createNewThrowAwayAccount("Canada");
        AccountService.registerNewUserThroughREST(friendOwnsDLC, null);
        EntitlementService.grantEntitlement(friendOwnsDLC, baseGameOfferId);
        EntitlementService.grantEntitlement(friendOwnsDLC, dlcOfferId);
        final UserAccount receiverOriginAccess = AccountManagerHelper.getTaggedUserAccountWithCountry(AccountTags.SUBSCRIPTION_UPGRADE_TEST, "Canada");

        final String senderAccountName = senderAccount.getUsername();
        final String friendOwnBaseName = friendOwnBase.getUsername();
        final String friendDoesNotOwnBaseName = friendDoesNotOwnBase.getUsername();
        final String friendOwnsDLCName = friendOwnsDLC.getUsername();
        final String receiverOriginAccessName = receiverOriginAccess.getUsername();

        //clean and add all accounts as friends to senderAccount
        senderAccount.cleanFriends();
        receiverOriginAccess.cleanFriends();
        UserAccountHelper.addFriends(senderAccount, friendOwnBase, friendDoesNotOwnBase, friendOwnsDLC, receiverOriginAccess);

        logFlowPoint("Log into Origin with an account that can gift DLC");
        logFlowPoint("Navigate to a PDP with a giftable DLC");
        logFlowPoint("Click on 'Gift this Item' option in the Purchase CTA dropdown menu and verify that friend who own base game but not DLC can be gifted DLC");
        logFlowPoint("Verify friend who does not own base game cannot be gifted DLC");
        logFlowPoint("Verify friend who owns DLC cannot be gifted DLC");
        logFlowPoint("Verify friend who is entitled to DLC through Origin Access can be gifted DLC");
        logFlowPoint("Verify friend who has the DLC pending as a gift cannot be gifted another DLC");

        final WebDriver driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, "/can/en-us");

        // 1
        if (MacroLogin.startLogin(driver, senderAccount)) {
            logPass("Verified login successful to user account: " + senderAccountName);
        } else {
            logFailExit("Failed: Cannot login to user account: " + senderAccountName);
        }

        // 2
//        if (MacroPDP.loadPdpPageBySearch(driver, dlcName)) {
//            logPass("Successfully navigate to a DLC PDP: " + dlcName);
//        } else {
//            logFailExit("Failed to navigate to DLC PDP: " + dlcName);
//        }

        // 3
       // new PDPHeroActionCTA(driver).selectBuyDropdownPurchaseAsGift();
        FriendsSelectionDialog friendsSelectionDialog = new FriendsSelectionDialog(driver);
        Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(friendOwnBase));
        if (Waits.pollingWait(() -> friendsSelectionDialog.recipientIsEligible(friendOwnBaseName))) {
            logPass("Friend with base game is eligible to receive DLC gift: " + friendOwnBaseName);
        } else {
            logFail("Friend with base game is ineligible to receive DLC gift: " + friendOwnBaseName);
        }

        // 4
        Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(friendDoesNotOwnBase));
        if (Waits.pollingWait(() -> friendsSelectionDialog.recipientIsIneligible(friendDoesNotOwnBaseName))) {
            logPass("Friend without base game is ineligible to receive DLC gift: " + friendDoesNotOwnBaseName);
        } else {
            logFail("Friend without base game is eligible to receive DLC gift:" + friendDoesNotOwnBaseName);
        }

        // 5
        Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(friendOwnsDLC));
        if (Waits.pollingWait(() -> friendsSelectionDialog.recipientAlreadyOwns(friendOwnsDLCName))) {
            logPass("Friend with DLC is ineligible to receive DLC gift: " + friendOwnsDLCName);
        } else {
            logFail("Friend with DLC is eligible to receive DLC gift:" + friendOwnsDLCName);
        }

        //6
        Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(receiverOriginAccess));
        if (Waits.pollingWait(() -> friendsSelectionDialog.recipientIsEligible(receiverOriginAccessName))) {
            logPass("Friend with Origin Access DLC is eligible to receive DLC gift: " + receiverOriginAccessName);
        } else {
            logFail("Friend with Origin Access DLC is ineligible to receive DLC gift:" + receiverOriginAccessName);
        }

        //7
        friendsSelectionDialog.close();
        MacroGifting.prepareGiftPurchase(driver, dlcName, dlcOfferId, friendOwnBaseName, "This is a gift");
        MacroPurchase.completePurchaseAndCloseThankYouPage(driver);
        //  new PDPHeroActionCTA(driver).selectBuyDropdownPurchaseAsGift();
        friendsSelectionDialog.waitForVisible();
        Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(friendOwnBase));
        if (Waits.pollingWait(() -> friendsSelectionDialog.recipientIsIneligible(friendOwnBaseName))) {
            logPass("Friend with pending DLC gift is ineligible to receive DLC gift: " + friendOwnBaseName);
        } else {
            logFail("Friend with pending DLC gift is eligible to receive DLC gift: " + friendOwnBaseName);
        }

        softAssertAll();
    }
}
