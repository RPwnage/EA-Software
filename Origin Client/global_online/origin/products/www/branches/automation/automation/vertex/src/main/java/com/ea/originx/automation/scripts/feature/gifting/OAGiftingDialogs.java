package com.ea.originx.automation.scripts.feature.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog.Recipient;
import com.ea.originx.automation.lib.pageobjects.dialog.GiftMessageDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.store.ReviewOrderPage;
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
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test gifting dialogs including 'Friends Selection', 'Gift Message', 'Review
 * Order' and 'Thank You' dialogs
 *
 * @author palui
 */
public class OAGiftingDialogs extends EAXVxTestTemplate {

    @TestRail(caseId = 3087355)
    @Test(groups = {"gifting", "gifting_smoke", "int_only", "allfeaturesmoke"})
    public void testGiftingDialogs(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount gifter = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);
        final UserAccount recipient = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_CANADA);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);
        final String entitlementName = entitlement.getName();
        final String gifterName = gifter.getUsername();
        final String recipientName = recipient.getUsername();

        logFlowPoint(String.format("Create gift recipient %s as newly registered user. Login as gifter %s unentitled to any game", recipientName, gifterName));//1
        logFlowPoint(String.format("Load PDP page for '%s'. Select 'Gift this game' from drop down menu and verify 'Friends Selection' dialog opens",
                entitlementName));//2
        logFlowPoint("Verify 'Next' button at 'Friends Selection' dialog is disabled when no friends are selected");//3
        logFlowPoint(String.format("Verify button with 'plus' sign is visible for recipient entry of %s in the friends list", recipientName));//4
        logFlowPoint(String.format("Select entry for %s in the friends list. Verify entry's button changes from 'plus' sign to 'tick' sign", recipientName));//5
        logFlowPoint(String.format("Verify 'Next' button is enabled when entry %s is selected", recipientName));//6
        logFlowPoint(String.format("Click 'Next'. Verify 'Gift Message' dialog opens with header indicating %s is a gift to %s", entitlementName, recipientName));//7
        logFlowPoint("Verify clearing text input for sender's name results in a 'name is required' error message and a disabled 'Next' button");//8
        logFlowPoint("Verify re-entering sender's name clears the 'name is required' error message and enables the 'Next' button");//9
        logFlowPoint(String.format("Click 'Next' and proceed with the purchase. Verify 'Order Review' page opens showing the purchase is a gift to %s", recipientName));//10
        logFlowPoint(String.format("Click 'Pay Now' button. Verify 'Thank You' page opens showing the purchase is a gift to %s", recipientName));//11

        //1
        WebDriver driver = startClientObject(context, client);
        boolean recipientRegistered = MacroLogin.startLogin(driver, recipient);
        new MiniProfile(driver).selectSignOut();
        recipient.cleanFriends();
        gifter.cleanFriends();
        gifter.addFriend(recipient);
        boolean gifterLoggedIn = MacroLogin.startLogin(driver, gifter);
        logPassFail(recipientRegistered && gifterLoggedIn, true);

        //2
        boolean pdpPageLoaded = MacroGDP.loadGdpPage(driver, entitlement);
        new GDPActionCTA(driver).selectDropdownPurchaseAsGift();
        FriendsSelectionDialog friendsSelectionDialog = new FriendsSelectionDialog(driver);
        boolean friendsSelectionDialogOpened = friendsSelectionDialog.waitIsVisible();
        logPassFail(pdpPageLoaded && friendsSelectionDialogOpened, true);

        //3
        logPassFail(!friendsSelectionDialog.verifyNextButtonEnabled(), true);

        //4
        Recipient recipientEntry = friendsSelectionDialog.getRecipient(recipientName);
        logPassFail(recipientEntry.isPlusButtonVisible(), true);

        //5
        recipientEntry.select();
        logPassFail(Waits.pollingWait(recipientEntry::isCheckCircleButtonVisible), true);

        //6
        logPassFail(friendsSelectionDialog.verifyNextButtonEnabled(), true);

        //7
        friendsSelectionDialog.clickNext();
        GiftMessageDialog giftMessageDialog = new GiftMessageDialog(driver);
        boolean giftMessageDialogOpened = giftMessageDialog.waitIsVisible();
        boolean headerVerified = giftMessageDialog.verifyDialogMessageHeader(entitlementName, recipientName);
        logPassFail(giftMessageDialogOpened && headerVerified, true);

        //8
        giftMessageDialog.clearSenderName();
        giftMessageDialog.clickDialogMessageHeader();
        boolean missingNameError = giftMessageDialog.verifyMissingSenderNameErrorMessage();
        boolean nextButtonEnabled = friendsSelectionDialog.verifyNextButtonEnabled();
        logPassFail(missingNameError && !nextButtonEnabled, true);
        
        //9
        giftMessageDialog.enterSenderName(gifterName);
        giftMessageDialog.clickDialogMessageHeader();
        boolean errorMessageVisible = giftMessageDialog.isErrorMessageVisible();
        nextButtonEnabled = friendsSelectionDialog.verifyNextButtonEnabled();
        logPassFail(!errorMessageVisible && nextButtonEnabled, true);

        //10
        giftMessageDialog.enterMessage("An AWESOME Gift from EA");
        giftMessageDialog.clickNext();
        boolean paymentSucceeded = MacroPurchase.handlePaymentInfoPage(driver);
        ReviewOrderPage reviewOrder = new ReviewOrderPage(driver);
        reviewOrder.waitForPageToLoad();
        boolean reviewOrderPageReached = Waits.pollingWait(() -> reviewOrder.verifyReviewOrderPageReached());
        boolean giftForReceiver = reviewOrder.verifyGiftForUser(recipientName);
        logPassFail(paymentSucceeded && reviewOrderPageReached && giftForReceiver, true);

        //11
        reviewOrder.clickCompleteOrderButton();
        ThankYouPage thankYouPage = new ThankYouPage(driver);
        boolean thankYouPageReached = Waits.pollingWait(() -> thankYouPage.verifyThankYouPageReached());
        giftForReceiver = thankYouPageReached && thankYouPage.verifyGiftForUser(recipientName);
        logPassFail(thankYouPageReached && giftForReceiver, true);

        softAssertAll();
    }
}