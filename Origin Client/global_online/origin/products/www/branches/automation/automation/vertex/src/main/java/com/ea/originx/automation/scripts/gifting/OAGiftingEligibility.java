package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Testing the multiple states of gifting eligibility
 *
 * NEEDS UPDATE TO GDP
 *
 * @author cvanichsarn
 */
public class OAGiftingEligibility extends EAXVxTestTemplate {

    @TestRail(caseId = 40233)
    @Test(groups = {"gifting", "full_regression", "int_only"})
    public void testGiftingEligibility(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount gifter = AccountManagerHelper.getUnentitledUserAccountWithCountry(OriginClientConstants.COUNTRY_CANADA);
        final UserAccount accountAlreadyOwns = AccountManagerHelper.getEntitledUserAccountWithCountry("Canada", EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD));
        final UserAccount deactivatedAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.DEACTIVATED);
        final UserAccount delectedAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.DELETED);
        final UserAccount bannedAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.BANNED);
        final UserAccount blocksGifterAccount = AccountManagerHelper.getUnentitledUserAccountWithCountry(OriginClientConstants.COUNTRY_CANADA);
        final UserAccount foreignCountryAccount = AccountManagerHelper.getUnentitledUserAccountWithCountry("United States");
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String gifterName = gifter.getUsername();
        final String accountAlreadyOwnsName = accountAlreadyOwns.getUsername();
        final String deactivatedAccountName = deactivatedAccount.getUsername();
        final String delectedAccountName = delectedAccount.getUsername();
        final String bannedAccountName = bannedAccount.getUsername();
        final String blocksGifterAccountName = blocksGifterAccount.getUsername();
        final String foreignCountryAccountName = foreignCountryAccount.getUsername();
        final String entitlementName = entitlement.getName();

        logFlowPoint("Launch Origin and login as user " + gifterName); // 1
        logFlowPoint("Navigate to the PDP of " + entitlementName); // 2
        logFlowPoint("From the buy drop down menu, select 'Gift this game'"); // 3
        logFlowPoint("Verify " + accountAlreadyOwnsName + " is listed to already own " + entitlementName); // 4
        logFlowPoint("Verify " + deactivatedAccountName + " doesn't appear as a candidate when searched for because it's deactivated"); // 5
        logFlowPoint("Verify " + delectedAccountName + " doesn't appear as a candidate when searched for because it's deleted"); // 6
        logFlowPoint("Verify " + bannedAccountName + "(banned account) is listed as ineligible"); // 7
        logFlowPoint("Verify " + blocksGifterAccountName + " is listed as ineligible due to the account blocking " + gifterName); // 8
        logFlowPoint("Verify " + foreignCountryAccountName + " is listed as ineligible to be gifted as it has a different country of residence"); // 9

        //1
        final WebDriver driver = startClientObject(context, client);
        gifter.cleanFriends();
        blocksGifterAccount.cleanFriends();
        UserAccountHelper.addFriends(gifter, blocksGifterAccount);
        UserAccountHelper.blockFriend(blocksGifterAccount, gifter);

        MacroLogin.startLogin(driver, gifter);

//        if (MacroPDP.loadPdpPage(driver, entitlement)) {
//            logPass("Successfully logged into " + gifterName);
//        } else {
//            logFailExit("Failed to log into " + gifterName);
//        }
//
//        //2
//        if (MacroPDP.loadPdpPage(driver, entitlement)) {
//            logPass("Successfully navigated to the PDP of " + entitlementName);
//        } else {
//            logFailExit("Failed to navigate to the PDP of " + entitlementName);
//        }

        //3
       // new PDPHeroActionCTA(driver).selectBuyDropdownPurchaseAsGift();
        FriendsSelectionDialog friendsSelectionDialog = new FriendsSelectionDialog(driver);
        if (friendsSelectionDialog.waitIsVisible()) {
            logPass("Successfully selected 'Gift this game' from the dropdown menu for " + entitlementName);
        } else {
            logFailExit("Failed to select 'Gift this game' from the dropdown menu for " + entitlementName);
        }

        //4
        friendsSelectionDialog.searchFor(accountAlreadyOwnsName);
        friendsSelectionDialog.verifyRecipientsListExists();
        boolean isPresent = Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(accountAlreadyOwnsName), 30000, 2000, 0);
        if (isPresent && friendsSelectionDialog.recipientJustAlreadyOwns(accountAlreadyOwnsName)) {
            logPass(accountAlreadyOwnsName + "'s eligibility was listed as 'Already Owned'");
        } else {
            logFailExit(accountAlreadyOwnsName + "'s eligibility was not listed as 'Already Owned'");
        }

        //5
        friendsSelectionDialog.clearSearch();
        friendsSelectionDialog.searchFor(deactivatedAccountName);
        if (friendsSelectionDialog.verifyNoUserFoundExists() || !friendsSelectionDialog.isRecipientPresent(deactivatedAccountName)) {
            logPass(deactivatedAccountName + " was not found when searched for due to being a deactivated account");
        } else {
            logFailExit(deactivatedAccountName + " was found when searched for despite being a deactivated account");
        }

        //6
        friendsSelectionDialog.clearSearch();
        friendsSelectionDialog.verifyRecipientsListExists();  //needed to ensure verifyNoUserFoundExists checks against the new search and doens't use the last results
        friendsSelectionDialog.searchFor(delectedAccountName);
        if (friendsSelectionDialog.verifyNoUserFoundExists() || !friendsSelectionDialog.isRecipientPresent(deactivatedAccountName)) {
            logPass(delectedAccountName + " was not found when searched for due to being a deleted account");
        } else {
            logFailExit(delectedAccountName + " was found when searched for despite to being a deleted account");
        }

        //7
        friendsSelectionDialog.clearSearch();
        friendsSelectionDialog.searchFor(bannedAccountName);
        friendsSelectionDialog.verifyRecipientsListExists();
        isPresent = Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(bannedAccountName), 30000, 2000, 0);
        if (isPresent && friendsSelectionDialog.recipientIsIneligible(bannedAccountName)) {
            logPass(bannedAccountName + "'s eligibility was listed as 'Ineligible' due to being a banned account");
        } else {
            logFailExit(bannedAccountName + "'s eligibility  not listed as 'Ineligible' despite to being a banned account");
        }

        //8
        friendsSelectionDialog.clearSearch();
        friendsSelectionDialog.verifyRecipientsListExists();
        isPresent = Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(blocksGifterAccountName), 30000, 2000, 0);
        if (isPresent && friendsSelectionDialog.recipientIsIneligible(blocksGifterAccountName)) {
            logPass(blocksGifterAccountName + "'s eligibility was listed as 'Ineligible' due to it blocking " + gifterName);
        } else {
            logFailExit(blocksGifterAccountName + "'s eligibility was not listed as 'Ineligible' despite to blocking " + gifterName);
        }

        //9
        friendsSelectionDialog.clearSearch();
        friendsSelectionDialog.searchFor(foreignCountryAccountName);
        friendsSelectionDialog.verifyRecipientsListExists();
        isPresent = Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(foreignCountryAccountName), 30000, 2000, 0);
        if (isPresent && friendsSelectionDialog.recipientIsIneligible(foreignCountryAccountName)) {
            logPass(foreignCountryAccountName + "'s eligibility was listed as 'Ineligible' due to a different country of residence");
        } else {
            logFailExit(foreignCountryAccountName + "'s eligibility was not listed as 'Ineligible despite having a different country of residence'");
        }

        client.stop();
        softAssertAll();
    }
}
