package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.settings.VoiceSettings;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindow;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * A test to verify the functionality of the cog button in a social chat button
 *
 * @author cvanichsarn
 */
public class OAChatWindowCog extends EAXVxTestTemplate  {
    
    @TestRail(caseId = 12649)
    @Test(groups = {"social", "client_only"})
    public void testChatWindowCog(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        
        final UserAccount userAccount = AccountManager.getRandomAccount();
        final UserAccount friendAccount = AccountManager.getRandomAccount();
        
        userAccount.cleanFriends();
        userAccount.addFriend(friendAccount);
        
        logFlowPoint("Log into Origin with an accout that has at least one friend"); // 1
        logFlowPoint("Expand the account's friends list"); // 2
        logFlowPoint("Open a chat window with a friend"); // 3
        logFlowPoint("Verify the cog icon appears in the chat window"); // 4
        logFlowPoint("Click the cog icon and verify that the user is directed to the 'Voice Settings' page"); // 5
        logFlowPoint("Navigate to the 'Game Library' page"); // 6
        logFlowPoint("In the chat window, click on the undocking icon"); // 7
        logFlowPoint("Click on the undocked window's cog icon"); // 8
        
        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        //2
        logPassFail(MacroSocial.restoreAndExpandFriends(driver), true);
        
        //3
        new SocialHub(driver).getFriend(friendAccount.getUsername()).openChat();
        ChatWindow chatWindow = new ChatWindow(driver);
        logPassFail(chatWindow.verifyChatOpen(), true);
        
        //4
        logPassFail(chatWindow.verifyVoiceChatSettingsButtonIsVisible(driver), true);
        
        //5
        chatWindow.clickVoiceChatSettingsButton(driver);
        VoiceSettings voiceSettings = new VoiceSettings(driver);
        logPassFail(voiceSettings.verifyVoiceSettingsReached(), true);
        
        //6
        new NavigationSidebar(driver).gotoGameLibrary();
        GameLibrary gameLibrary = new GameLibrary(driver);
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);
        
        //7
        chatWindow.popoutChatWindow();
        chatWindow.switchToPoppedOutChatWindow(driver);
        logPassFail(chatWindow.verifyChatWindowPoppedOut(driver), true);
        
        //8
        chatWindow.clickVoiceChatSettingsButton(driver);
        Waits.switchToPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL);
        logPassFail(voiceSettings.verifyVoiceSettingsReached(), true);
        
        softAssertAll();
    }
}
