package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.GiftMessageDialog;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the custom message dialog when sending a gift.
 *
 * NEEDS UPDATE TO GDP
 *
 * @author glivingstone
 */
public class OAGiftingCustomMessage extends EAXVxTestTemplate {

    @TestRail(caseId = 11763)
    @Test(groups = {"gifting", "full_regression", "int_only"})
    public void testGiftingCustomMessage(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount giftSender = AccountManagerHelper.getUserAccountWithCountry("Canada");
        final UserAccount giftReceiver = AccountManagerHelper.getUnentitledUserAccountWithCountry("Canada");
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);
        final String entitlementName = entitlement.getName();
        final String senderName = giftSender.getUsername();
        final String receiverName = giftReceiver.getUsername();
        giftReceiver.cleanFriends();
        giftSender.cleanFriends();
        giftSender.addFriend(giftReceiver);

        int defaultCharacterLimit = GiftMessageDialog.MESSAGE_BOX_LIMIT;
        String SHORT_MESSAGE = "12characters";
        int countAfterShort = defaultCharacterLimit - SHORT_MESSAGE.length();
        String LONG_MESSAGE = "Much Longer; 37ish character message.";
        int countAfterLong = defaultCharacterLimit - LONG_MESSAGE.length();
        // Create a string with the last two characters different
        String maxPlusOne = StringHelper.generateRandomString(defaultCharacterLimit);
        maxPlusOne = maxPlusOne + (char) (((int) maxPlusOne.charAt(maxPlusOne.length() - 1)) + 1);

        logFlowPoint("Login as " + senderName); // 1
        logFlowPoint("Navigate to the User's Profile Page to get the Real Name of the User"); // 2
        logFlowPoint("Navigate to the PDP of " + entitlementName); // 3
        logFlowPoint("From the Buy CTA Dropdown, Select 'Gift this game'"); // 4
        logFlowPoint("Select " + receiverName + " in the Account Selection Screen"); // 5
        logFlowPoint("Verify the Dialog Title is Displayed"); // 6
        logFlowPoint("Verify There is a Back Button"); // 7
        logFlowPoint("Verify the Pack Art for " + entitlementName + " Appears"); // 8
        logFlowPoint("Verify the From Field Contains " + senderName + "'s Real Name or Origin ID"); // 9
        logFlowPoint("Verify " + entitlementName + " and the Receiver's Origin ID are Displayed"); // 10
        logFlowPoint("Click 'Back' and Verify The User is Brought Back to the Friend Selection Dialog"); // 11
        logFlowPoint("Advance Back to the Gift Message Dialog by Pressing 'Next'"); // 12
        logFlowPoint("Verify the User may Enter a Maximum of 256 Characters into the Message Field"); // 13
        logFlowPoint("Verify as the User Types the Remaining Character Count Changes"); // 14
        logFlowPoint("Verify Emptying the 'From' Field Disables The Next Button and Shows an Error Message"); // 15
        logFlowPoint("Verify the User may edit the 'From' Field"); // 16
        logFlowPoint("Verify Clicking 'Next' Brings the User to the Checkout Flow"); // 17

        // 1
        final WebDriver driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, "/can/en-us");
        if (MacroLogin.startLogin(driver, giftSender)) {
            logPass("Successfully logged into " + senderName + " and navigated to " + entitlementName + "'s PDP");
        } else {
            logFailExit("Failed to log into " + senderName + " and navigate to " + entitlementName + "'s PDP");
        }

        // 2
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.selectViewMyProfile();
        String userName = new ProfileHeader(driver).getUserRealName();
        if (userName.isEmpty()) {
            userName = senderName;
        }
        if (!userName.isEmpty()) {
            logPass("Successfully got the user's username or real name.");
        } else {
            logFailExit("Could not get the user's username or real name.");
        }

        // 3
//        if (MacroPDP.loadPdpPage(driver, entitlement)) {
//            logPass("Successfully navigate to " + entitlementName + "'s the PDP.");
//        } else {
//            logFailExit("Could not navigate to " + entitlementName + "'s PDP.");
//        }

        // 4
       // new PDPHeroActionCTA(driver).selectBuyDropdownPurchaseAsGift();
        FriendsSelectionDialog friendsSelectionDialog = new FriendsSelectionDialog(driver);
        if (friendsSelectionDialog.waitIsVisible()) {
            logPass("Successfully selected 'Gift this game' from the dropdown menu");
        } else {
            logFailExit("Failed to select 'Gift this game' from the dropdown menu");
        }

        // 5
        Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(giftReceiver));
        friendsSelectionDialog.selectRecipient(giftReceiver);
        friendsSelectionDialog.clickNext();
        GiftMessageDialog giftMessageDialog = new GiftMessageDialog(driver);
        if (giftMessageDialog.waitIsVisible()) {
            logPass("Successfully selected a user");
        } else {
            logFailExit("Failed to select a user");
        }

        // 6
        if (giftMessageDialog.verifyTitleText()) {
            logPass("Dialog title text is correct.");
        } else {
            logFail("Dialog title text is not correct.");
        }

        // 7
        if (giftMessageDialog.verifyBackButtonExists()) {
            logPass("Dialog has a back button.");
        } else {
            logFail("Dialog does not have a back button.");
        }

        // 8
        if (giftMessageDialog.verifyPackArtExists()) {
            logPass("Pack art exists on the dialog.");
        } else {
            logFail("There is no pack art on the dialog.");
        }

        // 9
        if (giftMessageDialog.verifySenderName(senderName)) {
            logPass("The default entry in the 'From:' box is the user sending the gift.");
        } else {
            logFail("The default entry in the 'From:' box is not the user sending the gift.");
        }

        // 10
        boolean hasEntitlementName = giftMessageDialog.verifyMessageHeaderContains(entitlementName);
        boolean hasReceiverName = giftMessageDialog.verifyMessageHeaderContains(receiverName);
        if (hasEntitlementName && hasReceiverName) {
            logPass("The decription correctly states the gifted entitlement and gift receiver.");
        } else {
            logFail("The description does not correctly state the gifted entitlement and the gift receiver.");
        }

        // 11
        giftMessageDialog.clickBack();
        boolean closedGiftDialog = giftMessageDialog.waitIsClose();
        boolean friendSelectDialogOpen = friendsSelectionDialog.waitIsVisible();
        if (closedGiftDialog && friendSelectDialogOpen) {
            logPass("Successfully navigated to the Friend Selection Dialog using the back button.");
        } else {
            logFailExit("Could not navigate back to the Friend Selection Dialog using the back button.");
        }

        // 12
        Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(giftReceiver));
        friendsSelectionDialog.selectRecipient(giftReceiver);
        friendsSelectionDialog.clickNext();
        if (giftMessageDialog.waitIsVisible()) {
            logPass("Successfully advanced to the Gift Message Dialog.");
        } else {
            logFailExit("Failed to advance back to the Gift Message Dialog.");
        }

        // 13
        System.out.println("Message Entered is: " + maxPlusOne);
        boolean correctDefault = giftMessageDialog.verifyCharacterRemainingCount(defaultCharacterLimit);
        giftMessageDialog.enterMessage(maxPlusOne);
        boolean correctAfterMax = giftMessageDialog.verifyCharacterRemainingCount(0);
        String enteredMessage = giftMessageDialog.getMessage();
        boolean sameLastCharacter = enteredMessage.charAt(defaultCharacterLimit - 1) == maxPlusOne.charAt(defaultCharacterLimit);
        if (correctDefault && correctAfterMax && !sameLastCharacter) {
            logPass("Maximum number of characters for the custom message is 256.");
        } else {
            logFail("Maximum number of characters for the custom message is not 256");
        }

        // 14
        giftMessageDialog.enterMessage(SHORT_MESSAGE);
        boolean correctAfterShort = giftMessageDialog.verifyCharacterRemainingCount(countAfterShort);
        giftMessageDialog.enterMessage(LONG_MESSAGE);
        boolean correctAfterLong = giftMessageDialog.verifyCharacterRemainingCount(countAfterLong);
        if (correctAfterShort && correctAfterLong) {
            logPass("Remaining character count is correctly updating.");
        } else {
            logFail("Remaining character count does not adjust correctly.");
        }

        // 15
        giftMessageDialog.enterSenderName("");
        giftMessageDialog.clickMessageBox(); // Click off to complete text entry
        boolean nameChanged = giftMessageDialog.getSenderName().isEmpty();
        boolean nextEnabled = giftMessageDialog.isNextEnabled();
        boolean errorAppears = giftMessageDialog.verifyFromErrorMessageVisible();
        if (nameChanged && !nextEnabled && errorAppears) {
            logPass("Emptying the 'From' box disabled the 'Next' button and surfaces an error.");
        } else {
            logFail("Could not empty the 'From' box, or it does not disable the 'Next' button.");
        }

        // 16
        giftMessageDialog.enterSenderName("Mr Automation");
        giftMessageDialog.clickMessageBox(); // Click off to complete text entry
        nameChanged = giftMessageDialog.getSenderName().equals("Mr Automation");
        nextEnabled = giftMessageDialog.isNextEnabled();
        if (nameChanged && nextEnabled) {
            logPass("Changing the 'From' box works correctly.");
        } else {
            logFail("Unable to change the 'From' box, or it disables the 'Next' button.");
        }

        // 17
        giftMessageDialog.clickNext();
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        if (paymentInformationPage.verifyPaymentMethods()) {
            logPass("Successfully entered a message and verified payments page");
        } else {
            logFailExit("Failed to verify payments page");
        }
        client.stop();
        softAssertAll();
    }
}
