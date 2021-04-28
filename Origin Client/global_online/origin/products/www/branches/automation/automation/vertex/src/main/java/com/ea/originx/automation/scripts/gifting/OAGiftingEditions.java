package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.EntitlementService;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests gifting different editions of a game
 *
 * NEEDS UPDATE TO GDP
 *
 * @author JMittertreiner
 * @author sbentley
 */
public class OAGiftingEditions extends EAXVxTestTemplate {

    @TestRail(caseId = 11784)
    @Test(groups = {"client", "gifting", "browser", "full_regression", "int_only"})
    public void testGiftingEditions(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo bf4Deluxe = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_DIGITAL_DELUXE);

        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);

        final UserAccount throwaway = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);

        final UserAccount ownsBf4Standard = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);
        EntitlementService.grantEntitlement(ownsBf4Standard, EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD).getOfferId());

        final UserAccount ownsBf4Deluxe = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);
        EntitlementService.grantEntitlement(ownsBf4Deluxe, EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_DIGITAL_DELUXE).getOfferId());

        final UserAccount ownsBf4Premium = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);
        EntitlementService.grantEntitlement(ownsBf4Premium, EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM).getOfferId());

        final UserAccount accessUser = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);

        UserAccount[] friends = {throwaway, ownsBf4Standard, ownsBf4Deluxe, ownsBf4Premium, accessUser};
        UserAccountHelper.addFriends(userAccount, friends);

        logFlowPoint("Open Origin and log in"); // 1
        logFlowPoint("Navigate to a giftable middle edition PDP"); // 2
        logFlowPoint("Click on gift this item");    //3
        logFlowPoint("Verify user that does not own edition can be gifted"); // 4
        logFlowPoint("Verify user that owns lesser edition can be gifted"); // 5
        logFlowPoint("Verify user that owns same edition cannot be gifted and there is an 'Already Owned' message"); // 6
        logFlowPoint("Verify user that owns greater edition cannot be gifted and there is an 'Already Owned' message"); //7
        logFlowPoint("Verify Origin Access user that has edition through Vault can be gifted");    //8
        logFlowPoint("Verify user with pending open gift of edition cannot be gifted"); // 9

        // 1
        final WebDriver driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, "/can/en-us");

        //Setting up the OA account to own BF4 premium
        MacroLogin.startLogin(driver, accessUser);
        MacroOriginAccess.purchaseOriginAccess(driver);
        //    new PDPHeroActionCTA(driver).clickDirectAcquisitionButton();
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        checkoutConfirmation.waitIsVisible();
        checkoutConfirmation.clickCloseCircle();
        checkoutConfirmation.waitIsClose();
        new MiniProfile(driver).selectSignOut();

        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Passed: Open Origin and log in as " + userAccount.getUsername());
        } else {
            logFailExit("Failed: Open Origin and log in as " + userAccount.getUsername());
        }

        // 2
     //   new PDPHeroActionCTA(driver).selectBuyDropdownPurchaseAsGift();
        FriendsSelectionDialog friendsSelectionDialog = new FriendsSelectionDialog(driver);
        if (friendsSelectionDialog.waitIsVisible()) {
            logPass("Navigate to a PDP for Battlefield Digital Deluxe and click on gift this item");
        } else {
            logFailExit("Failed to Navigate to a PDP for Battlefield Digital Deluxe and click on gift this item");
        }

        // 3
        if (friendsSelectionDialog.canGiftTo(throwaway)) {
            logPass("An account which does not own any games (" + throwaway.getUsername() + ") is giftable");
        } else {
            logFail("An account which does not own any games (" + throwaway.getUsername() + ") is not giftable");
        }

        // 4
        if (friendsSelectionDialog.canGiftTo(ownsBf4Standard)) {
            logPass("An account which owns BF4 Standard (" + ownsBf4Standard.getUsername() + ") is giftable");
        } else {
            logFail("An account which owns BF4 Standard (" + ownsBf4Standard.getUsername() + ") is not giftable");
        }

        // 5
        if (!friendsSelectionDialog.canGiftTo(ownsBf4Deluxe) && friendsSelectionDialog.recipientAlreadyOwns(ownsBf4Deluxe.getUsername())) {
            logPass("An account which owns BF4 Digital Deluxe (" + ownsBf4Deluxe.getUsername() + ") is not giftable");
        } else {
            logFail("An account which owns BF4 Digital Deluxe (" + ownsBf4Deluxe.getUsername() + ") is giftable");
        }

        // 6
        if (!friendsSelectionDialog.canGiftTo(ownsBf4Premium) && friendsSelectionDialog.recipientAlreadyOwns(ownsBf4Premium.getUsername())) {
            logPass("An account which owns BF4 Premium (" + ownsBf4Premium.getUsername() + ") is not giftable");
        } else {
            logFail("An account which owns BF4 Premium (" + ownsBf4Premium.getUsername() + ") is giftable");
        }

        // 7
        if (friendsSelectionDialog.canGiftTo(accessUser)) {
            logPass("An account which owns BF4 Premium (" + accessUser.getUsername() + ") through access is giftable");
        } else {
            logFail("An account which owns BF4 Premium (" + accessUser.getUsername() + ") through access is not giftable");
        }

        // 8
        friendsSelectionDialog.closeAndWait();
        if (MacroGifting.purchaseGiftForFriend(driver, bf4Deluxe, throwaway.getUsername())) {
            logPass("Completed the gifting flow to send the bundle as a gift to " + throwaway.getUsername());
        } else {
            logFailExit("Could not complete the gifting flow to send the bundle as a gift to " + throwaway.getUsername());
        }

        // 9
    //    new PDPHeroActionCTA(driver).selectBuyDropdownPurchaseAsGift();
        friendsSelectionDialog.waitIsVisible();
        if (!friendsSelectionDialog.canGiftTo(throwaway)) {
            logPass("An account which has been gifted BF4 Digital Deluxe (" + throwaway.getUsername() + ") is not giftable");
        } else {
            logFail("An account which has been gifted BF4 Digital Deluxe (" + throwaway.getUsername() + ") is giftable");
        }

        softAssertAll();
    }
}