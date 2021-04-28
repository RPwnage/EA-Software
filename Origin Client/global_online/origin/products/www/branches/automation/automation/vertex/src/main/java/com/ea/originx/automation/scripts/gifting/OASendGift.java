package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.EmailFetcher;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.GiftMessageDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import javax.mail.Message;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test sending a gift
 *
 * @author cbalea
 */
public class OASendGift extends EAXVxTestTemplate {
    @TestRail(caseId = 1016696)
    @Test(groups = {"gifting", "release_smoke", "int_only"})
    public void testSendGift(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        // gifter
        final UserAccount gifterAccount = AccountManagerHelper.createNewThrowAwayAccount("Canada");
        final String gifterAccountFirstName = gifterAccount.getFirstName();
        final String gifterAccountUserName = gifterAccount.getUsername();
        final String customMessage = "test";

        // giftee
        final UserAccount gifteeAccount = AccountManagerHelper.createNewThrowAwayAccount("Canada");
        final String gifteeAccountUserName = gifteeAccount.getUsername();

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);

        // for testing for searching friend by first name
        final UserAccount accountWithUniqueFirstName = AccountManagerHelper.getEntitledUserAccountWithCountry("Canada", entitlement);
        final String usernameWithUniqueFirstName = accountWithUniqueFirstName.getUsername();
        final String emailAddrWithUniqueFirstName = accountWithUniqueFirstName.getEmail();
        final String firstNameWithUniqueFirstName = accountWithUniqueFirstName.getFirstName();

        accountWithUniqueFirstName.cleanFriends();

        logFlowPoint("Launch Origin and login");    //1
        logFlowPoint("Navigate to GDP of a multiple edition entitlement");  //2
        logFlowPoint("Select 'Purchase as Gift' from dropdown and verify this brings the user to OSP");   //3
        logFlowPoint("Click the gifting CTA button and verify that this brings up the select a friend modal");   //4
        logFlowPoint("Verify gifter can see the gift receiver from the list and they are eligible for gifting");   //5
        logFlowPoint("In search bar type user name and verify search brings up users with that specific name");   //6
        logFlowPoint("In search bar type full email address of the user and verify search brings up only that user");   //7
        logFlowPoint("In search bar type user's real name and verify the search brings up users with the same real name");   //8
        logFlowPoint("Verify users eligibility appears on the user's tile");   //9
        logFlowPoint("Select the giftable, click next and verify the user can type in a custom user name");   //10
        logFlowPoint("Verify the user can type in a personal message"); // 11
        logFlowPoint("Click next and verify user can complete the purchase flow"); // 12
        logFlowPoint("Verify that user receives an email with the gift order summary"); // 13


        // 1
        // creating new account which will receive a gift from gifter
        WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, gifterAccount);
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.selectSignOut();

        // to get unique first name
        logPassFail(MacroLogin.startLogin(driver, gifteeAccount), true);
        gifteeAccount.addFriend(gifterAccount);
        gifteeAccount.addFriend(accountWithUniqueFirstName);

        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 3
        new GDPActionCTA(driver).selectDropdownPurchaseAsGift();
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        logPassFail(offerSelectionPage.verifyOfferSelectionPageReached(), true);

        // 4
        offerSelectionPage.clickPrimaryButton(entitlement.getOcdPath());
        FriendsSelectionDialog friendsSelectionDialog = new FriendsSelectionDialog(driver);
        logPassFail(friendsSelectionDialog.waitIsVisible(), true);

        // 5
        logPassFail(friendsSelectionDialog.isRecipientPresent(gifterAccount), true);

        // 6
        friendsSelectionDialog.searchFor(usernameWithUniqueFirstName);
        boolean isPresentByUserName = Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(usernameWithUniqueFirstName), 30000, 2000, 0);
        logPassFail(isPresentByUserName, false);

        // 7
        friendsSelectionDialog.clearSearch();
        friendsSelectionDialog.searchFor(emailAddrWithUniqueFirstName);
        boolean isPresentByEmailAddr = Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(usernameWithUniqueFirstName), 30000, 2000, 0);
        logPassFail(isPresentByEmailAddr, false);

        // 8
        friendsSelectionDialog.clearSearch();
        friendsSelectionDialog.searchFor(firstNameWithUniqueFirstName);
        boolean isPresentByFirstName = Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(firstNameWithUniqueFirstName), 30000, 2000, 0);
        logPassFail(isPresentByFirstName, false);

        // 9
        friendsSelectionDialog.clearSearch();
        Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(gifterAccountUserName), 30000, 2000, 0);
        logPassFail(friendsSelectionDialog.recipientIsEligible(gifterAccountUserName), true);

        // 10
        friendsSelectionDialog.selectRecipient(gifterAccountUserName);
        friendsSelectionDialog.clickNext();
        GiftMessageDialog giftMessageDialog = new GiftMessageDialog(driver);
        giftMessageDialog.waitIsVisible();
        giftMessageDialog.enterSenderName(gifterAccountUserName + " : " + gifterAccountFirstName);
        boolean isSenderNameEditable = giftMessageDialog.getSenderName().contains(gifterAccountUserName + " : " + gifterAccountFirstName);
        logPassFail(isSenderNameEditable, false);

        // 11
        giftMessageDialog.enterMessage(customMessage);
        boolean isMeesageEditable = giftMessageDialog.getMessage().contains(customMessage);
        logPassFail(isMeesageEditable, false);

        // 12
        logPassFail(MacroGifting.completePurchaseGiftFromGiftMessageDialog(driver), true);

        // 13
        EmailFetcher gifterEmailFetcher = new EmailFetcher(gifteeAccountUserName);
        Message message = gifterEmailFetcher.getLatestGiftSentEmail();
        logPassFail(message != null, true);
        gifterEmailFetcher.closeConnection();

        softAssertAll();
    }
}