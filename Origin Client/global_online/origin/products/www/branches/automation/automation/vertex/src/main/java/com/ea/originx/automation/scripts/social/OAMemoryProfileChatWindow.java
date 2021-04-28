package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindow;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the memory profile of the Chat Window in order to expose memory leaks
 *
 * @author mdobre
 */
public class OAMemoryProfileChatWindow extends EAXVxTestTemplate {

    @Test(groups = {"social", "memory_profile"})
    public void testPerformanceChatWindow(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getRandomAccount();
        userAccount.cleanFriends();
        final UserAccount firstFriendAccount = AccountManager.getRandomAccount();
        final UserAccount secondFriendAccount = AccountManager.getRandomAccount();
        UserAccountHelper.addFriends(userAccount, firstFriendAccount, secondFriendAccount);

        logFlowPoint("Login to Origin with an existing account."); //1
        for (int i = 0; i < 15; i++) {
            logFlowPoint("Open a chat window with a friend, close the chat window and perform garbage collection."); //2-16
        }

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user " + userAccount.getUsername());
        } else {
            logFailExit("Could not log into Origin with the user " + userAccount.getUsername());
        }

        //2-16
        SocialHub socialHub = new SocialHub(driver);
        MacroSocial.restoreAndExpandFriends(driver);
        ChatWindow chatWindow = new ChatWindow(driver);
        boolean isChatWindowOpen = false;
        boolean isChatWindowClosed = false;
        boolean isGarbageCollectionSuccesful = false;

        for (int i = 0; i < 15; i++) {

            //Open Chat Window with a friend
            socialHub.getFriend(firstFriendAccount.getUsername()).openChat();
            isChatWindowOpen = chatWindow.verifyChatOpen();

            //Close Chat Window
            chatWindow.closePoppedInChatWindow();
            isChatWindowClosed = !chatWindow.verifyChatOpen();

            //Perform Garbage Collection
            isGarbageCollectionSuccesful = garbageCollect(driver, context);

            if (isChatWindowOpen && isChatWindowClosed && isGarbageCollectionSuccesful) {
                logPass("Successfully opened a chat window with a friend, closed the chat window and performed garbage collection.");
            } else {
                logFailExit("Failed to open a chat window with a friend, close the chat window or perform garbage collection.");
            }
        }
        softAssertAll();
    }
}
