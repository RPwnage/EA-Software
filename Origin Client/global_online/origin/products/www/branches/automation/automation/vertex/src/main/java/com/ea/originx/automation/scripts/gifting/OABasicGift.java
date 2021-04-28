package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroGifting;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.NotificationCenter;
import com.ea.originx.automation.lib.pageobjects.discover.OpenGiftPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the basic flow of gifting an entitlement
 *
 * @author cbalea
 */
public class OABasicGift extends EAXVxTestTemplate{
    
    @Test(groups = {"gifting", "long_smoke"})
    public void testBasicGift(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount sender = AccountManagerHelper.getUnentitledUserAccountWithCountry(OriginClientConstants.COUNTRY_CANADA);
        final UserAccount receiver = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);
        final String senderUsername = sender.getUsername();
        final String receiverUsername = receiver.getUsername();
        final EntitlementInfo giftedEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        
        logFlowPoint("Launch Origin and login as sender"); // 1
        logFlowPoint("Send a gift to receiver"); // 2
        logFlowPoint("Logout of sender and login as receiver"); // 3
        logFlowPoint("Verify the user received the gift from sender"); // 4
        logFlowPoint("Open the gift and verify the game has been added to the library"); // 5
        
        WebDriver driver = startClientObject(context, client);
        MiniProfile miniProfile = new MiniProfile(driver);
        sender.cleanFriends();
        sender.addFriend(receiver);
        
        // 1
        if (MacroLogin.startLogin(driver, sender)) {
            logPass("Successfully logged into Origin with user " + senderUsername);
        } else {
            logFailExit("Failed to log into Origin with user " + senderUsername);
        }
        
        // 2
        if(MacroGifting.purchaseGiftForFriend(driver, giftedEntitlement, receiverUsername)){
            logPass("Successfully gifted entitlement to friend");
        } else {
            logFailExit("Failed to gift entitlement to friend");
        }
        
        // 3
        miniProfile.selectSignOut();
        if (MacroLogin.startLogin(driver, receiver)) {
            logPass("Successfully logged out of sender and logged in as receiver " + receiverUsername);
        } else {
            logFailExit("Failed to login as receiver " + receiverUsername);
        }
        
        // 4
        NotificationCenter notificationCenter = new NotificationCenter(driver);
        if (notificationCenter.verifyGiftReceivedVisible()) {
            logPass("Successfully verified the user has a gift received");
        } else {
            logFail("Failed to verify gift notification");
        }
        
        // 5
        notificationCenter.clickYouGotGiftNotification();
        new OpenGiftPage(driver).clickCloseButton();
        new NavigationSidebar(driver).gotoGameLibrary();
        if(MacroGameLibrary.verifyGameInLibrary(driver, giftedEntitlement.getName())){
            logPass("Successfully verified entitlement is present in user library");
        } else {
            logFailExit("Failed to verify entitlement presence in user library");
        }
        
        softAssertAll();
    }
}
