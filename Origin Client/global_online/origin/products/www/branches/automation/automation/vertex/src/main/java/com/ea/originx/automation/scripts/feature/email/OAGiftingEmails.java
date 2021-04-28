package com.ea.originx.automation.scripts.feature.email;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.EmailFetcher;
import com.ea.originx.automation.lib.macroaction.MacroGifting;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
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
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

import javax.mail.Message;

/**
 * Test gifting emails
 *
 * @author palui
 */
public class OAGiftingEmails extends EAXVxTestTemplate {

    @TestRail(caseId = 3068608)
    @Test(groups = {"gifting", "emails", "gifting_smoke", "email_smoke", "int_only", "allfeaturesmoke"})
    public void testGiftingEmails(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount gifter = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_CANADA);
        sleep(1000); // Pause to ensure second userName is unique
        final UserAccount recipient = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_CANADA);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);
        final String entitlementName = entitlement.getName();
        final String gifterName = gifter.getUsername();
        final String recipientName = recipient.getUsername();

        logFlowPoint(String.format("Create newly registered gifter %s and recipient %s as friends", gifterName, recipientName));//1
        logFlowPoint(String.format("Login to Origin as gifter %s", gifterName));//2
        logFlowPoint(String.format("Open PDP page of %s for gifter %s to purchase the game as a gift for recipient %s", entitlementName, gifterName, recipientName));//3
        logFlowPoint(String.format("Verified gifter %s receives email on gift sent", gifterName));//4
        logFlowPoint(String.format("Verified recipient %s receives email on gift received", recipientName));//5

        //1
        final WebDriver driver = startClientObject(context, client);
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
        if (MacroGifting.purchaseGiftForFriend(driver, entitlement, recipientName)) {
            logPass(String.format("Verified %s successfully purchases gift %s for %s", gifterName, entitlementName, recipientName));
        } else {
            logFailExit(String.format("Failed: %s cannot purchase gift %s for %s", gifterName, entitlementName, recipientName));
        }

        //4
        EmailFetcher gifterEmailFetcher = new EmailFetcher(gifterName);
        Message message = gifterEmailFetcher.getLatestGiftSentEmail();
        if (message != null) {
            logPass(String.format("Verified gifter %s receives email on gift sent", gifterName));
        } else {
            logFailExit(String.format("Failed: gifter %s does not receive email on gift sent", gifterName));
        }
        gifterEmailFetcher.closeConnection();

        //5
        EmailFetcher recipientEmailFetcher = new EmailFetcher(recipientName);
        message = recipientEmailFetcher.getLatestGiftReceivedEmail();
        if (message != null) {
            logPass(String.format("Verified recipient %s receives email on gift received", recipientName));
        } else {
            logFailExit(String.format("Failed: recipient %s does not receive email on gift received", recipientName));
        }
        recipientEmailFetcher.closeConnection();

        softAssertAll();
    }
}
