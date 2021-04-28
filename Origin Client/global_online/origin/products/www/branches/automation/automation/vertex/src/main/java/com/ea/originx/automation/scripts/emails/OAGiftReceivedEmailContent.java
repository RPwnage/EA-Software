package com.ea.originx.automation.scripts.emails;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGifting;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.originx.automation.lib.helpers.EmailFetcher;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.webdrivers.BrowserType;

import java.util.Map;

import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test gifting received email content
 *
 * @author palui
 */
public class OAGiftReceivedEmailContent extends EAXVxTestTemplate {

    @TestRail(caseId = 11767)
    @Test(groups = {"gifting", "emails", "full_regression", "int_only"})
    public void testGiftReceivedEmailContent(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount gifter = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_CANADA);
        sleep(5); // Pause to ensure second userName is unique
        final UserAccount recipient = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_CANADA);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();
        final String gifterName = gifter.getUsername();
        final String recipientName = recipient.getUsername();

        logFlowPoint(String.format("Create newly registered gifter %s and recipient %s as friends", gifterName, recipientName));//1
        logFlowPoint(String.format("Login to Origin as gifter %s", gifterName));//2
        logFlowPoint(String.format("Open PDP page of %s for gifter %s to purchase the game as a gift for recipient %s", entitlementName, gifterName, recipientName));//3
        logFlowPoint(String.format("Verified recipient %s receives email on gift received", recipientName));//4
        logFlowPoint("Verify recipient email contains the original order number");//5
        logFlowPoint("Verify recipient email contains a gifting message with the sender's name");//6
        logFlowPoint("Verify recipient email contains a 'Open Your Gift' link");//7
        logFlowPoint(String.format("Verify browser login successful as recipient %s", recipient)); //8
        logFlowPoint("Verify clicking email's 'Open Your Gift' link navigates to the 'Gift Received Overlay' page for " + entitlementName); //9

        //1
        WebDriver driver = startClientObject(context, client);
        boolean gifterRegistered = MacroLogin.startLogin(driver, gifter);
        new MiniProfile(driver).selectSignOut();
        boolean recipientRegistered = MacroLogin.startLogin(driver, recipient);
        new MiniProfile(driver).selectSignOut();
        gifter.cleanFriends();
        recipient.cleanFriends();
        gifter.addFriend(recipient);
        if (gifterRegistered && recipientRegistered) {
            logPass(String.format("Verified registration successful for gifter %s and recipient %s",
                    gifterName, recipientName));
        } else {
            logFailExit(String.format("Failed: registration failed for one or both gifter %s and recipient %s",
                    gifterName, recipientName));
        }

        //2
        if (MacroLogin.startLogin(driver, gifter)) {
            logPass(String.format("Verified login successful as gifter %s", gifterName));
        } else {
            logFailExit(String.format("Failed: Cannot login as gifter %s", gifterName));
        }

        //3
        String originalOrderNumber = MacroGifting.purchaseGiftForFriendReturnOrderNumber(driver, entitlement, recipientName);
        if (originalOrderNumber != null) {
            logPass(String.format("Verified %s successfully purchases order %s of gift %s for user %s", gifterName, originalOrderNumber, entitlementName, recipientName));
        } else {
            logFailExit(String.format("Failed: %s cannot purchase gift %s for user %s", gifterName, entitlementName, recipientName));
        }

        //4
        EmailFetcher recipientEmailFetcher = new EmailFetcher(recipientName);
        Map<String, String> giftReceivedEmailFields = recipientEmailFetcher.getLatestGiftReceivedEmailFields();
        if (giftReceivedEmailFields.size() > 0) {
            logPass(String.format("Verified recipient %s receives email on gift received", recipientName));
        } else {
            logFailExit(String.format("Failed: recipient %s does not receive email on gift received", recipientName));
        }
        recipientEmailFetcher.closeConnection();
        String orderNumber = giftReceivedEmailFields.get("Order Number");
        String sender = giftReceivedEmailFields.get("Sender");
        String openYourGiftLink = giftReceivedEmailFields.get("Open Your Gift Link");

        //5
        if (orderNumber != null && orderNumber.equals(originalOrderNumber)) {
            logPass(String.format("Verified recipient email has the original order number %s", orderNumber));
        } else {
            logFailExit(String.format("Failed: recipient email has missing/wrong order number %s not matching original order number %s",
                    orderNumber, originalOrderNumber));
        }

        //6
        if (sender != null && sender.equalsIgnoreCase(gifterName)) {
            logPass(String.format("Verified recipient email has gifting message matching sender for gifter %s", gifterName));
        } else {
            logFailExit(String.format("Failed: Recipient email has sender %s not matching gifter %s", sender, gifterName));
        }

        //7
        if (openYourGiftLink != null) {
            logPass(String.format("Verified recipient %s email has 'Open Your Gift' link %s", recipientName, openYourGiftLink));
        } else {
            logFailExit(String.format("Verified recipient %s email does not have 'Open Your Gift' link", recipientName));
        }

        //8
        WebDriver browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        if (MacroLogin.startLogin(browserDriver, recipient)) {
            logPass(String.format("Verified browser login successful as recipient %s", recipientName));
        } else {
            logFailExit(String.format("Failed: Cannot login to browser as recipient %s", recipientName));
        }

        //9
        browserDriver.get(openYourGiftLink);
        String giftReceivedURLRegex = String.format(OriginClientData.GIFT_RECEIVED_URL_REGEX_TMPL, entitlementOfferId);
        if (Waits.waitIsPageThatMatchesOpen(browserDriver, giftReceivedURLRegex, 10)) {
            logPass("Verified clicking email's 'Open Your Gift' link navigates to the 'Gift Received Overlay' page for " + entitlementName);
        } else {
            logFailExit("Failed: Clicking email's 'Open Your Gift' link does not navigate to the 'Gift Received Overlay' page for " + entitlementName);
        }
        softAssertAll();
    }
}
