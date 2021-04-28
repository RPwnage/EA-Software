package com.ea.originx.automation.scripts.feature.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.GiftMessageDialog;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.pageobjects.store.ThankYouPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Testing that if a gift is pending, that the same gift cannot be sent to the
 * same user
 *
 * NEEDS UPDATE TO GDP
 *
 * @author cvanichsarn
 */
public class OAGiftPending extends EAXVxTestTemplate {

    @TestRail(caseId = 3087356)
    @Test(groups = {"gifting", "gifting_smoke", "int_only", "allfeaturesmoke"})
    public void testGiftPending(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount gifter1 = AccountManagerHelper.getUnentitledUserAccountWithCountry(OriginClientConstants.COUNTRY_CANADA);
        final UserAccount gifter2 = AccountManagerHelper.getUnentitledUserAccountWithCountry(OriginClientConstants.COUNTRY_CANADA);
        final UserAccount giftee = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_CANADA);
        final EntitlementInfo entitlement1 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);
        final EntitlementInfo entitlement2 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String gifter1Name = gifter1.getUsername();
        final String gifter2Name = gifter2.getUsername();
        final String gifteeName = giftee.getUsername();
        final String entitlement1Name = entitlement1.getName();
        final String entitlement2Name = entitlement2.getName();

        logFlowPoint("Launch Origin, create " + gifteeName + ", and login as user " + gifter1Name + " and navigate to the PDP of " + entitlement1Name); // 1
        logFlowPoint("From the buy drop down menu, select 'Purchase as a gift'"); // 2
        logFlowPoint("Verify " + gifteeName + " appears as a candidate to gift to"); // 3
        logFlowPoint("Select " + gifteeName + " from the dialog and complete the gifting flow"); // 4
        logFlowPoint("Navigate to the PDP of Star Wars Battlefront again and select 'Purchase as a gift' again"); // 5
        logFlowPoint("Verify " + gifteeName + " is not selectable as a recipient"); // 6
        logFlowPoint("Navigate to the PDP for " + entitlement2Name + " and select 'Purchase as a gift'"); // 7
        logFlowPoint("Verify " + gifteeName + " is selectable as a recipient"); // 8
        logFlowPoint("Log out of " + gifter1Name + " and sign into " + gifter2Name); // 9
        logFlowPoint("Navigate to the PDP of " + entitlement1Name + " and select 'Purchase as a gift'"); // 10
        logFlowPoint("Verify " + gifteeName + " is not selectable as a recipient"); // 11

        //1
        final WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, giftee);
        new MiniProfile(driver).selectSignOut();
        giftee.cleanFriends();
        gifter1.cleanFriends();
        gifter2.cleanFriends();
        gifter1.addFriend(giftee);
        gifter2.addFriend(giftee);
        MacroLogin.startLogin(driver, gifter1);
//        if (MacroPDP.loadPdpPage(driver, entitlement1)) {
//            logPass("Successfully logged into " + gifter1Name + "and navigated to " + entitlement1Name + "'s PDP");
//        } else {
//            logFailExit("Failed to log into " + gifter1Name + " and navigate to " + entitlement1Name + "'s PDP");
//        }

        //2
//        PDPHeroActionCTA pdpHeroCTA = new PDPHeroActionCTA(driver);
//        pdpHeroCTA.selectBuyDropdownPurchaseAsGift();
        FriendsSelectionDialog friendsSelectionDialog = new FriendsSelectionDialog(driver);
        logPassFail(friendsSelectionDialog.waitIsVisible(), true);

        //3
        friendsSelectionDialog.selectRecipient(giftee);
        friendsSelectionDialog.clickNext();
        GiftMessageDialog giftMessageDialog = new GiftMessageDialog(driver);
        logPassFail(giftMessageDialog.waitIsVisible(), true);

        //4
        giftMessageDialog.clickNext();
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        paymentInformationPage.verifyPaymentMethods();
        MacroPurchase.completePurchase(driver);
        ThankYouPage thankYouPage = new ThankYouPage(driver);
        thankYouPage.verifyThankYouPageReached();
        logPassFail(thankYouPage.verifyGiftForUser(gifteeName), true);

        //5
        thankYouPage.clickCloseButton();
        //   pdpHeroCTA.selectBuyDropdownPurchaseAsGift();
        friendsSelectionDialog = new FriendsSelectionDialog(driver);
        logPassFail(friendsSelectionDialog.waitIsVisible(), true);

        //6
        logPassFail(friendsSelectionDialog.recipientIsIneligible(gifteeName), true);

        //7
        friendsSelectionDialog.clickCloseCircle();
        //using a sleep function here for 5 seconds because I've consistently encountered loading issues.  The 2 seconds used by loadPdpPage is not enough
        sleep(5000);
        // pdpHeroCTA.selectBuyDropdownPurchaseAsGift();
        friendsSelectionDialog = new FriendsSelectionDialog(driver);
        logPassFail(friendsSelectionDialog.waitIsVisible(), true);

        //8
        logPassFail(friendsSelectionDialog.recipientIsEligible(gifteeName), true);

        //9
        friendsSelectionDialog.clickCloseCircle();
        new MiniProfile(driver).selectSignOut();
        logPassFail(MacroLogin.startLogin(driver, gifter2), true);

        //10
  //      pdpHeroCTA.selectBuyDropdownPurchaseAsGift();
        friendsSelectionDialog = new FriendsSelectionDialog(driver);
        logPassFail(friendsSelectionDialog.waitIsVisible(), true);

        //11
        logPassFail(friendsSelectionDialog.recipientIsIneligible(gifteeName), true);

        client.stop();
        softAssertAll();
    }
}
