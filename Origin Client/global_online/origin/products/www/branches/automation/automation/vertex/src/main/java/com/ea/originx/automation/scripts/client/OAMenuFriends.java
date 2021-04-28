package com.ea.originx.automation.scripts.client;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.dialog.SearchForFriendsDialog;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.pageobjects.social.SocialHubMinimized;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the items present in Friends Menu
 *
 * @author cbalea
 */
public class OAMenuFriends extends EAXVxTestTemplate{
    
    @TestRail(caseId = 9761)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testMenuFriends (ITestContext context)throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getRandomAccount();
        
        logFlowPoint("Launch and log into Origin"); //1
        logFlowPoint("Verify 'Add a Friend' is in the Friends drop down and clicking on it displays a search dialog"); //2
        logFlowPoint("Close Search for friends dialog"); //3
        logFlowPoint("Verify 'Set Status' is in the Friends drop down"); //4
        logFlowPoint("Set a new status and verify in friends list the status changed"); //5
        
        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        //2
        MainMenu mainMenu = new MainMenu(driver);
        boolean addFriend = mainMenu.verifyItemEnabledInFriends("Add a Friend…");
        mainMenu.selectAddFriend();
        SearchForFriendsDialog searchForFriendsDialog = new SearchForFriendsDialog(driver);
        boolean searchFriendDialog = searchForFriendsDialog.waitIsVisible();     
        logPassFail(addFriend && searchFriendDialog, true);
        
        //3
        searchForFriendsDialog.close();
        logPassFail(!searchForFriendsDialog.waitIsVisible(), true);
        
        //4
        logPassFail(mainMenu.verifyItemEnabledInFriends("Set Status"), true);
        
        //5
        mainMenu.selectStatusAway();
        new SocialHubMinimized(driver).restoreSocialHub();
        logPassFail(new SocialHub(driver).verifyPresenceDotAway(), true);
        
        softAssertAll();
    }  
}