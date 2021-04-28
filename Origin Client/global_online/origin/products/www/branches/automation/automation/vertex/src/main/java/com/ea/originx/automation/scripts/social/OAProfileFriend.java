package com.ea.originx.automation.scripts.social;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOfflineMode;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindow;
import com.ea.originx.automation.lib.pageobjects.social.Friend;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import java.util.Arrays;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test for profile friend navigation<br>
 * - View stranger's profile as either inviting (outgoing) or invited (incoming)
 * friend request<br>
 * - View friend's profile<br>
 * - Chat with friend and open own profile from chat window avatar<br>
 * - Login as friend and open own profile from chat window avatar
 *
 * @author palui
 */
public class OAProfileFriend extends EAXVxTestTemplate {

    @TestRail(caseId = 13050)
    @Test(groups = {"social", "client_only", "full_regression"})
    public void testProfileFriend(ITestContext context) throws Exception {

        final OriginClient userClient = OriginClientFactory.create(context);
        final OriginClient friendClient = OriginClientFactory.create(context);
        final String MESSAGE = "Chat with a friend";

        UserAccount userAccount = AccountManager.getRandomAccount();
        UserAccount friendAccount = AccountManager.getRandomAccount();
        UserAccount outgoingFriend = AccountManager.getRandomAccount();
        UserAccount incomingFriend = AccountManager.getRandomAccount();

        // Clean/add friend, send a friend request and receive a friend request
        userAccount.cleanFriends();
        friendAccount.cleanFriends();
        outgoingFriend.cleanFriends();
        incomingFriend.cleanFriends();
        userAccount.addFriend(friendAccount);
        UserAccountHelper.inviteFriend(userAccount, outgoingFriend);
        UserAccountHelper.inviteFriend(incomingFriend, userAccount);

        logFlowPoint("Launch Origin and login with user A"); //1
        logFlowPoint("Click on the avatar of user A and verify the user's profile can be accessed"); //2
        logFlowPoint("Click on the username of user A and verify the user's profile can be accessed by selecting 'View My Profile'"); //3
        logFlowPoint("Open a 'Chat Window' with user B and verify own profile can be accessed by clicking the user A's avatar in the user A's 'Chat Window'"); //4
        logFlowPoint("Verify friend's profile can be accessed by clicking the user A's avator in user B's 'Chat Window'"); //5
        logFlowPoint("Right click on user B, select 'View Profile' and verify the friend's profile can be accessed"); //6
        logFlowPoint("Send and receive a friend request and verify a stranger's profile can be accessed for incoming request"); //7
        logFlowPoint("Verify there are no performace issues viewing the profile"); //8
        logFlowPoint("Click on 'Go Offline' and verify the profile has an offline state"); //9
        logFlowPoint("Click on 'Go Onine' and verify coming back online restores the online profile"); //10

        //1
        WebDriver driver = startClientObject(context, userClient);
        WebDriver driver2 = startClientObject(context, friendClient);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.clickProfileAvatar();
        logPassFail(miniProfile.verifyPopoutMenuItemsExist(Arrays.asList("View My Profile"), new boolean[]{true}), true);
        
        //3
        miniProfile.clickProfileUsername();
        logPassFail(miniProfile.verifyPopoutMenuItemsExist(Arrays.asList("View My Profile"), new boolean[]{true}), true);

        //4
        MacroSocial.restoreAndExpandFriends(driver);
        SocialHub socialHub = new SocialHub(driver);
        socialHub.getFriend(friendAccount.getUsername()).openChat();
        ChatWindow chatWindow = new ChatWindow(driver);
        chatWindow.sendMessage(MESSAGE);
        String userAvatar = miniProfile.getAvatarSource();
        sleep(2000); //pause for Chat window to stablize
        chatWindow.clickUserAvatar(userAvatar);
        ProfileHeader profileHeader = new ProfileHeader(driver);
        logPassFail(profileHeader.verifyUsername(userAccount.getUsername()), true);
        
        //5
        MacroLogin.startLogin(driver2, friendAccount);
        ChatWindow chatWindowFriend = new ChatWindow(driver2);
        chatWindowFriend.sendMessage(MESSAGE);
        sleep(2000); //pause for Chat window to stablize
        chatWindowFriend.clickUserAvatar(userAvatar);
        logPassFail(new ProfileHeader(driver2).verifyUsername(userAccount.getUsername()), true);
        
        //6
        Friend friendUser = socialHub.getFriend(friendAccount.getUsername());
        friendUser.viewProfile();
        sleep(2000); //pause for profile window to stablize
        logPassFail(profileHeader.verifyUsername(friendAccount.getUsername()), true);
        
        //7
        Friend incomingUser = socialHub.getFriend(incomingFriend.getUsername());
        logPassFail(incomingUser.verifyViewAndClickProfileVisible(), true);
        
        //8
        incomingUser.viewProfile();
        sleep(2000); //pause for profile window to stablize
        logPassFail(profileHeader.verifyUsername(incomingFriend.getUsername()), true);

        //9
        logPassFail(MacroOfflineMode.goOffline(driver), true);
        
        //10
        sleep(4000); //waiting for profile to stabilize
        logPassFail(MacroOfflineMode.goOnline(driver), true);
        softAssertAll();
    }
}
