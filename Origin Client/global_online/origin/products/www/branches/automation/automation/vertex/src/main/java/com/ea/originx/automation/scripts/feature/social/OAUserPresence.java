package com.ea.originx.automation.scripts.feature.social;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests various User Presence states
 *
 * @author sbentley
 */
public class OAUserPresence extends EAXVxTestTemplate {

    @TestRail(caseId = 3120665)
    @Test(groups = {"social", "social_smoke", "client_only", "allfeaturesmoke"})
    public void testUserPresence(ITestContext context) throws Exception {

        final OriginClient userClient = OriginClientFactory.create(context);
        final OriginClient friendClient = OriginClientFactory.create(context);
        final String environment = OSInfo.getEnvironment();
        EntitlementInfo entitlement;
        String userName = null;
        String entitlementName;
        UserAccount userAccount = null;
        UserAccount friendAccount = null;
        
        if (environment.equalsIgnoreCase("production")) {
            //Setting up entitlement and accounts on prod
            entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.PEGGLE);
            entitlementName = entitlement.getName();
            userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.ENTITLEMENTS_DOWNLOAD_PLAY_TEST_ACCOUNT);
            userName = userAccount.getUsername();
            friendAccount = AccountManager.getInstance().createNewThrowAwayAccount();
            
        } else {
            //Setting up entitlement and accounts on dev, qa
            entitlement = new OADipSmallGame();
            entitlementName = "DiP Small";
            userAccount = AccountManager.getEntitledUserAccount(entitlement);
            userName = userAccount.getUsername();
            friendAccount = AccountManager.getRandomAccount();
            userAccount.cleanFriends();
            friendAccount.cleanFriends();
            friendAccount.addFriend(userAccount);
        }
        
        final String entitlementOfferId = entitlement.getOfferId();
        final String entitlementProcessName = entitlement.getProcessName();

        logFlowPoint("Login to Origin with User A");    //1
        logFlowPoint("Navigate to the Game Library");   //2
        logFlowPoint("Download " + entitlementName); //3
        logFlowPoint("Login to Origin with User B");    //4
        logFlowPoint("Launch " + entitlementName + " with User A");   //5
        logFlowPoint("Verify User A and User B see User A  as being In-Game"); //6
        logFlowPoint("Quit " + entitlementName); //7
        logFlowPoint("Set & Verify User A's status to Away");   //8
        logFlowPoint("Verify User B sees User A as being Away");//9
        logFlowPoint("Set & Verify User A's status to Invisible");  //10
        logFlowPoint("Verify User B sees User A as being Invisible/Offline");   //11
        logFlowPoint("Set & Verify User A's status to Online"); //12
        logFlowPoint("Verify User B sees User A as being Online");  //13

        //1
        //Log in
        WebDriver userDriver = startClientObject(context, userClient);
        logPassFail(MacroLogin.startLogin(userDriver, userAccount), true);
        // Clean up the friends list
        if (environment.equalsIgnoreCase("production")) {
            MacroSocial.cleanUpAllFriendsThroughUI(userDriver);
        }
        
        //2
        //Go to game library
        GameLibrary gameLibrary = new NavigationSidebar(userDriver).gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //3
        //Download and install
        logPassFail(MacroGameLibrary.downloadFullEntitlement(userDriver, entitlementOfferId), true);
        entitlement.addActivationFile(userClient);
       
        //4
        //Log in
        WebDriver friendDriver = startClientObject(context, friendClient);
        logPassFail(MacroLogin.startLogin(friendDriver, friendAccount), true);
        if (environment.equalsIgnoreCase("production")) {
            MacroSocial.addFriendThroughUI(friendDriver, userDriver, friendAccount, userAccount);
        }
        
        //5
        //Launch
        new GameTile(userDriver, entitlementOfferId).play();
        boolean isGameLaunched = Waits.pollingWaitEx(() ->entitlement.isLaunched(userClient));
        logPassFail(isGameLaunched, true);

        //6
        //Verify User A Status
        MacroSocial.restoreAndExpandFriends(userDriver);
        SocialHub userSocialHub = new SocialHub(userDriver);
        boolean userAInGame = userSocialHub.verifyUserInGame();

        //Verify User A Status via User B
        MacroSocial.restoreAndExpandFriends(friendDriver);
        SocialHub friendSocialHub = new SocialHub(friendDriver);
        boolean userBInGame = friendSocialHub.verifyFriendInGame(userName);
        logPassFail(userAInGame && userBInGame, false);

        //7
        //Close DiP Small
        ProcessUtil.killProcess(userClient, entitlementProcessName);
        logPassFail(Waits.pollingWaitEx(() -> !ProcessUtil.isProcessRunning(userClient, entitlementProcessName)), true);

        //8
        //Set & verify status
        userSocialHub.setUserStatusAway();
        logPassFail(userSocialHub.verifyUserAway(), false);

        //9
        //Verify status
        logPassFail(friendSocialHub.verifyFriendAway(userName), false);

        //10
        //Set & verify status
        userSocialHub.setUserStatusInvisible();
        logPassFail(userSocialHub.verifyUserInvisible(), false);

        //11
        //Verify status
        logPassFail(friendSocialHub.verifyFriendOffline(userName), false);

        //12
        //Set & verify status
        userSocialHub.setUserStatusOnline();
        logPassFail(userSocialHub.verifyUserOnline(), false);

        //13
        //Verify status
        logPassFail(friendSocialHub.verifyFriendOnline(userName), false);
        
        softAssertAll();
    }
}