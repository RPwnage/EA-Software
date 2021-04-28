package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.settings.VoiceSettings;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindow;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.pageobjects.social.SocialHubMinimized;
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
 * Test the social hub menu at the top of the friends list
 *
 * @author esdecastro
 */
public class OASocialHubTopContextMenu extends EAXVxTestTemplate{

    @TestRail(caseId = 13200)
    @Test(groups = {"social", "client_only", "full_regression"})
    public void testSocialHubTopContextMenu(ITestContext context) throws Exception{

        final OriginClient client = OriginClientFactory.create(context);

        // Setting up user accounts
        UserAccount userAccount = AccountManager.getRandomAccount();
        UserAccount friend1 = AccountManager.getRandomAccount();
        UserAccount friend2 = AccountManager.getRandomAccount();
        UserAccount friend3 = AccountManager.getRandomAccount();
        UserAccount friend4 = AccountManager.getRandomAccount();

        userAccount.cleanFriends();
        friend1.cleanFriends();
        friend2.cleanFriends();
        friend3.cleanFriends();
        friend4.cleanFriends();
        UserAccountHelper.addFriends(userAccount, friend1, friend2, friend3, friend4);

        logFlowPoint("Log into Origin with an account that contains at least 4 friends"); //1
        logFlowPoint("Open the 'Social Hub' and click on the context menu and verify the menu options appear"); //2
        logFlowPoint("Pop out the 'Social Hub' and verify that the 'Chat Settings' and 'Close all Chat Tabs' options appear"); //3
        logFlowPoint("Pop the 'Social Hub' back in by closing the pop up window"); //4
        logFlowPoint("Select 'Chat Settings' and verify the user has navigated to the 'Chat Settings' page"); //5
        logFlowPoint("Open a chat with each of the 4 friends"); //6
        logFlowPoint("Select 'Close All Chat Tabs' and verify all the chat windows are closed"); //7
        logFlowPoint("Open the 'Social Hub' menu and verify that clicking anywhere on the 'Social Hub' closes the menu"); //8
        logFlowPoint("Open the 'Social Hub' menu and verify that clicking anywhere on the client closes the menu"); //9

        //1
        WebDriver driver = startClientObject(context, client);
        if(MacroLogin.startLogin(driver, userAccount)){
            logPass(String.format("Successfully logged into origin as %s",userAccount.getUsername()));
        } else {
            logFailExit(String.format("Could not log in as %s", userAccount.getUsername()));
        }

        //2
        SocialHub socialHub = new SocialHub(driver);
        SocialHubMinimized socialHubMinimized = new SocialHubMinimized(driver);
        socialHubMinimized.restoreSocialHub();
        if(socialHub.verifySocialHubMenuItemsVisible()){
            logPass("Verified that the menu includes 'Minimize', 'Open in a New Window', 'Chat Settings', 'Close All Chat Tabs\'");
        } else{
            logFail("Could not display all the menu items");
        }

        //3
        String currentURL = driver.getCurrentUrl();
        if (socialHub.verifyPopOutSocialHubMenuItemsVisible()){
            logPass("Verified that the popped out menu includes 'Chat Settings' and 'Close All Chat Tabs'");
        } else{
            logFail("Could not display all of the popped out menu items");
        }

        //4
        driver.close();
        Waits.switchToPageThatEquals(driver, currentURL);
        if(socialHub.verifySocialHubVisible()){
            logPass("Verified that the 'Social Hub' is popped back into the client");
        } else {
            logFailExit("Could not re-open 'Social Hub' within the client");
        }

        //5
        socialHub.clickChatSettingsMenuOption();
        VoiceSettings voiceSettings = new VoiceSettings(driver);
        if(voiceSettings.verifyVoiceSettingsReached()){
            logPass("Verified that clicking on the chat settings menu item takes you to the settings page");
        } else{
            logFailExit("Could not reach the Voice Settings page");
        }

        //6
        ChatWindow chatWindow = new ChatWindow(driver);
        socialHub.getFriend(friend1.getUsername()).openChat();
        socialHub.getFriend(friend2.getUsername()).openChat();
        socialHub.getFriend(friend3.getUsername()).openChat();
        socialHub.getFriend(friend4.getUsername()).openChat();
        if(chatWindow.verifyOverflowChatTabExists()) {
            logPass("Verify that all 4 chat windows were opened");
        } else {
            logFail("Could not open multiple chat windows");
        }

        //7
        socialHub.clickCloseAllChatTabsMenuOption();
        if (!chatWindow.verifyChatOpen()){
            logPass("Verified that all user windows were closed");
        } else{
            logFail("Could not close all the chat windows");
        }

        //8
        if(socialHub.verifySocialHubDropdownCloseOnSocialHubClick()){
            logPass("Verified that clicking on other parts of the 'Social Hub' closses the drop down context menu");
        } else {
            logFail("Could not close the drop down menu when clicking on the 'Social Hub'");
        }

        //9
        if(socialHub.verifySocialHubDropdownCloseOnClientClick()){
            logPass("Verified that clicking on other parts of the client closes the drop down context menu");
        } else {
            logFail("Could not close the drop down menu when clicking on the client");
        }
        softAssertAll();
    }
}
