package com.ea.originx.automation.scripts.emails;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGifting;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.originx.automation.lib.helpers.EmailFetcher;
import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;

import java.util.Map;

import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;
import com.ea.vx.annotations.TestRail;

/**
 * Test gifting sent email content
 *
 * @author rocky
 */
public class OAGiftSentEmailContent extends EAXVxTestTemplate {

    @TestRail(caseId = 11766)
    @Test(groups = {"gifting", "emails", "full_regression", "int_only"})
    public void testGiftSentEmailContent(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount giftReceiver = AccountManagerHelper.createNewThrowAwayAccount("Canada");
        final String giftReceiverName = giftReceiver.getUsername();

        sleep(5); // Pause to ensure second userName is unique

        final UserAccount giftSender = AccountManagerHelper.createNewThrowAwayAccount("Canada");
        final String giftSenderName = giftSender.getUsername();
        final String giftSenderEmail = giftSender.getEmail();

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);
        final String entitlementName = entitlement.getName();

        logFlowPoint(String.format("Create two accounts %s as gift receiver and %s as gift sender as friends.", giftReceiverName, giftSenderName));//1
        logFlowPoint(String.format("Successfully login in as gift sender %s", giftSenderName)); //2
        logFlowPoint(String.format("%s Successfully purchased %s of gift and got order number for user %s", giftSenderName, entitlementName, giftReceiverName));
        logFlowPoint(String.format("Verified gift sender %s receives email on gift sent", giftSenderName));//4
        logFlowPoint("Verify gift sent email has the original order number"); //5
        logFlowPoint("Verify there is an order date");//6
        logFlowPoint("Verify gift sender email address is in gift sent email"); //7
        logFlowPoint("Verify gift receiver username is in gift sent email"); //8
        logFlowPoint("Verify 'gift was sent to' message is in the gift sent email"); //9
        logFlowPoint("Verify order summary is found in email");//10
        logFlowPoint("Verify entitlement name is in gift sent email");//11
        logFlowPoint("Verify footer appears in the gift sent email");//12
        logFlowPoint("Verify subtotal, tax and total Price appear in the gift sent email");//13
        logFlowPoint("Verify termOfSaleLink in footer does not return '404' http response code"); //14
        logFlowPoint("Verify legalNoticeLink in footer does not return '404' http response code"); //15
        logFlowPoint("Verify privacyPolicyLink in footer does not return '404' http response code"); //16
        logFlowPoint("Verify userAgreementLink in footer does not return '404' http response code"); //17
        logFlowPoint("Verify legalLink in footer does not return '404' http response code"); //18

        //1
        WebDriver driver = startClientObject(context, client);
        boolean giftReceiverRegistered = MacroLogin.startLogin(driver, giftReceiver);
        new MiniProfile(driver).selectSignOut();
        boolean giftSenderRegistered = MacroLogin.startLogin(driver, giftSender);
        new MiniProfile(driver).selectSignOut();
        giftSender.cleanFriends();
        giftReceiver.cleanFriends();
        giftSender.addFriend(giftReceiver);
        if (giftReceiverRegistered && giftSenderRegistered) {
            logPass(String.format("Created two accounts %s as gift receiver and %s as gift sender as friends.", giftReceiverName, giftSenderName));
        } else {
            logFailExit(String.format("Could not create two accounts %s as gift receiver and %s as gift sender as friends.", giftReceiverName, giftSenderName));
        }

        //2
        if (MacroLogin.startLogin(driver, giftSender)) {
            logPass(String.format("Successfully login in as gift sender %s", giftSenderName));
        } else {
            logFailExit(String.format("Could not login as gift sender %s", giftSenderName));
        }

        //3
        String originalOrderNumber = MacroGifting.purchaseGiftForFriendReturnOrderNumber(driver, entitlement, giftReceiverName);
        if (originalOrderNumber != null) {
            logPass(String.format("%s Successfully purchased %s of gift and got order number %s for user %s", giftSenderName, entitlementName, originalOrderNumber, giftReceiverName));
        } else {
            logFailExit(String.format("%s Could not purchase %s of gift and got order number %s for user %s", giftSenderName, entitlementName, originalOrderNumber, giftReceiverName));
        }

        //4
        EmailFetcher recipientEmailFetcher = new EmailFetcher(giftSenderName);
        Map<String, String> giftSentEmailFields = recipientEmailFetcher.getLatestGiftSentEmailFields();
        if (giftSentEmailFields.size() > 0) {
            logPass(String.format("Verified gift sender %s receives email on gift sent", giftSenderName));
        } else {
            logFailExit(String.format("Gift sender %s does not receive email on gift sent", giftSenderName));
        }
        recipientEmailFetcher.closeConnection();
        String orderNumber = giftSentEmailFields.get("Order Number");
        String orderDate = giftSentEmailFields.get("Order Date");
        String senderEmailAddrInEmail = giftSentEmailFields.get("Sender Email");
        String giftReceiverUserName = giftSentEmailFields.get("Gift Receiver");
        String giftSentToMessage = giftSentEmailFields.get("GiftSentTo Message");
        String orderSummary = giftSentEmailFields.get("Order Summary");
        String entitlementNameinEmail = giftSentEmailFields.get("Entitlement Name");
        String footerText = giftSentEmailFields.get("Footer");
        String subtotal = giftSentEmailFields.get("Subtotal");
        String tax = giftSentEmailFields.get("Tax");
        String totalPrice = giftSentEmailFields.get("Total Price");
        String termOfSaleLink = giftSentEmailFields.get("Terms of Sale");
        String legalNoticeLink = giftSentEmailFields.get("Legal Notice");
        String privacyPolicyLink = giftSentEmailFields.get("Privacy Policy");
        String userAgreementLink = giftSentEmailFields.get("User Agreement");
        String legalLink = giftSentEmailFields.get("Legal");

        //5
        if (orderNumber != null && StringHelper.containsIgnoreCase(orderNumber, originalOrderNumber)) {
            logPass(String.format("Verified gift sent email has the original order number %s", orderNumber));
        } else {
            logFail(String.format("Gift sent email has missing/wrong order number %s not matching original order number %s", orderNumber, originalOrderNumber));
        }

        //6
        if (TestScriptHelper.isValidDateFormat(orderDate)) {
            logPass(String.format("Verified date %s is valid date format", orderNumber));
        } else {
            logFail(String.format("Date %s is invalid date format", orderNumber));
        }

        //7
        if (senderEmailAddrInEmail != null && StringHelper.containsIgnoreCase(senderEmailAddrInEmail, giftSenderEmail)) {
            logPass(String.format("Verified gift sender email address %s is in gift sent email", giftSenderEmail));
        } else {
            logFail(String.format("Could not find gift sender email address %s in gift sent email", giftSenderEmail));
        }

        //8
        if (giftReceiverUserName != null && StringHelper.containsIgnoreCase(giftReceiverUserName, giftReceiverName)) {
            logPass(String.format("Verified gift receiver username %s is in gift sent email", giftReceiverName));
        } else {
            logFail(String.format("Could not find gift sender email %s in gift sent email", giftReceiverName));
        }

        //9
        if (giftSentToMessage != null) {
            logPass("Verified 'gift was sent to' message in the gift sent email");
        } else {
            logFail("Could not find 'gift was sent to' message in the gift sent email");
        }

        //10
        if (orderSummary != null) {
            logPass("Verified 'Order Summary' text is found in email");
        } else {
            logFail("Could not find 'Order Summary' text in email");
        }

        //11
        if (entitlementNameinEmail != null && StringHelper.containsIgnoreCase(entitlementNameinEmail, entitlementName)) {
            logPass(String.format("Verified entitlement name %s is in gift sent email", entitlementName));
        } else {
            logFail(String.format("Could not find entitlement name %s in gift sent email", entitlementName));
        }

        //12
        if (footerText != null) {
            logPass("Verified footer appears in the gift sent email");
        } else {
            logFail("Could not find footer in the gift sent email");
        }

        //13
        if (subtotal != null && tax != null && totalPrice != null) {
            logPass(String.format("Verified subtotal: %s, tax: %s, total Price: %s appear in the gift sent email", subtotal, tax, totalPrice));
        } else {
            logFail("Could not find subtotal/tax/total price in the gift sent email");
        }

        //14
        String httpResponse = TestScriptHelper.getHttpResponse(termOfSaleLink);
        if (!httpResponse.equals("404") && !httpResponse.equals("Server DNS not found")) {
            logPass("Verified termOfSaleLink in footer does not return '404' http response code or 'Server DNS not found'");
        } else {
            logFail("termOfSaleLink in footer returned '404' http response code or 'Server DNS not found'");
        }
        //15
        httpResponse = TestScriptHelper.getHttpResponse(legalNoticeLink);
        if (!httpResponse.equals("404") && !httpResponse.equals("Server DNS not found")) {
            logPass("Verified legalNoticeLink in footer does not return '404' http response code or 'Server DNS not found'");
        } else {
            logFail("legalNoticeLink in footer returned '404' http response code or 'Server DNS not found'");
        }

        //16
        httpResponse = TestScriptHelper.getHttpResponse(privacyPolicyLink);
        if (!httpResponse.equals("404") && !httpResponse.equals("Server DNS not found")) {
            logPass("Verified privacyPolicyLink in footer does not return '404' http response code or 'Server DNS not found'");
        } else {
            logFail("privacyPolicyLink in footer returned '404' http response code or 'Server DNS not found'");
        }

        //17
        httpResponse = TestScriptHelper.getHttpResponse(userAgreementLink);
        if (!httpResponse.equals("404") && !httpResponse.equals("Server DNS not found")) {
            logPass("Verified userAgreementLink in footer does not return '404' http response code or 'Server DNS not found'");
        } else {
            logFail("userAgreementLink in footer returned '404' http response code or 'Server DNS not found'");
        }

        //18
        httpResponse = TestScriptHelper.getHttpResponse(legalLink);
        if (!httpResponse.equals("404") && !httpResponse.equals("Server DNS not found")) {
            logPass("Verified legalLink in footer does not return '404' http response code or 'Server DNS not found'");
        } else {
            logFail("legalLink in footer returned '404' http response code or 'Server DNS not found'");
        }

        softAssertAll();
    }
}
