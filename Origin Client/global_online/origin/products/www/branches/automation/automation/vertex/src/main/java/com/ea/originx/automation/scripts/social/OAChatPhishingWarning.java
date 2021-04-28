package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindow;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test if the phishing warning is visible and if it disappears after sending
 * some messages
 *
 * @author mdobre
 */
public class OAChatPhishingWarning extends EAXVxTestTemplate {

    @TestRail(caseId = 12641)
    @Test(groups = {"client", "full_regression", "client_only"})
    public void testChatPhishingWarning(ITestContext context) throws Exception {

        final OriginClient clientA = OriginClientFactory.create(context);
        final OriginClient clientB = OriginClientFactory.create(context);
        final String MESSAGE = "Hello";

        // Setup Friends
        UserAccount userAccount = AccountManager.getRandomAccount();
        UserAccount friendAccount = AccountManager.getRandomAccount();
        userAccount.cleanFriends();
        friendAccount.cleanFriends();
        UserAccountHelper.addFriends(friendAccount, userAccount);
        Thread.sleep(5000);
        
        logFlowPoint("Log into Origin with " + userAccount.getUsername()); // 1
        logFlowPoint("Open Chat Window with " + friendAccount.getUsername() + " and verify the message 'Never share your password with anyone' is displayed"); // 2
        logFlowPoint("Send a Message to " + friendAccount.getUsername() + " and verify the warning scrolls away"); // 3

        // 1
        WebDriver driver = startClientObject(context, clientA);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        //2
        MacroSocial.restoreAndExpandFriends(driver);
        SocialHub socialHub = new SocialHub(driver);
        Thread.sleep(4000);
        socialHub.getFriend(friendAccount.getUsername()).openChat();
        Thread.sleep(5000);
        ChatWindow chatWindow = new ChatWindow(driver);
        if (chatWindow.verifyPhishingWarning()) {
            logPass("Successfully verified that when users open a new chat window there's a message which warns them 'Never share your password with anyone'");
        } else {
            logFailExit("Could not verify that the warning is displayed when opening a new chat window");
        }

        //3
        WebDriver driver2 = startClientObject(context, clientB);
        MacroLogin.startLogin(driver2, friendAccount);

        for (int i = 0; i < 15; i++) {
            chatWindow.sendMessage(MESSAGE);
        }
        if (Waits.pollingWait(() -> chatWindow.verifyPhishingWarningNotVisible())) {
            logPass("Successfully verified that the warning is no longer visible");
        } else {
            logFailExit("Failed to verify the warning is not displayed when sending more messages");
        }

    }

}
