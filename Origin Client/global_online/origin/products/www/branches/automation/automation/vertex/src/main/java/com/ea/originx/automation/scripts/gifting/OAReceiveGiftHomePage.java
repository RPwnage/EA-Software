package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGifting;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.TakeOverPage;
import com.ea.originx.automation.lib.pageobjects.discover.OpenGiftPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NotificationCenter;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;
import com.ea.vx.annotations.TestRail;

/**
 * Test the home page when multiple gifts are received
 *
 * @author jmittertreiner
 * @author mdobre
 */
public class OAReceiveGiftHomePage extends EAXVxTestTemplate {

    @TestRail(caseId = 11776)
    @Test(groups = {"gifting", "full_regression", "int_only"})
    public void testReceiveGiftHomePage(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount giver1 = AccountManagerHelper.createNewThrowAwayAccount(CountryInfo.CountryEnum.CANADA.getCountry());
        final UserAccount giver2 = AccountManagerHelper.createNewThrowAwayAccount(CountryInfo.CountryEnum.CANADA.getCountry());
        final UserAccount receiver = AccountManagerHelper.createNewThrowAwayAccount(CountryInfo.CountryEnum.CANADA.getCountry());

        EntitlementInfo game1 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        EntitlementInfo game2 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);


        logFlowPoint("Launch Origin and login as a user A to send gift to user C"); // 1
        logFlowPoint("Logout as user A and login as a user B to send gift to user C"); // 2
        logFlowPoint("Logout as user B and login as a user C who has now pending gifts"); // 3
        logFlowPoint("Verify that in the 'Notifications' there is a message indicating a gift has been received."); // 4
        logFlowPoint("Verify that the tile message will display sender's name."); // 5
        logFlowPoint("Verify that whatever is chosen to display in the 'From Field' is displayed in the gift notification and there is only one."); // 6

        //receiver
        final WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, receiver);
        new MiniProfile(driver).selectSignOut();

        // 1
        MacroLogin.startLogin(driver, giver1);
        giver1.addFriend(receiver);
        boolean isGift1 = MacroGifting.purchaseGiftForFriend(driver, game1, receiver.getUsername());
        if (isGift1) {
            logPass("Successfully logged in as user A: " + giver1.getUsername()+" and sent a gift to user C: " + receiver.getUsername());
        } else {
            logFailExit("Failed to log in as as user A: " + giver1.getUsername()+" or could not send a gift to user C: " + receiver.getUsername());
        }
        new MiniProfile(driver).selectSignOut();

        // 2
        MacroLogin.startLogin(driver, giver2);
        giver2.addFriend(receiver);
        boolean isGift2 = MacroGifting.purchaseGiftForFriend(driver, game2, receiver.getUsername());
        if (isGift2) {
            logPass("Successfully logged in as user B: " + giver1.getUsername()+" and sent a gift to user C: " + receiver.getUsername());
        } else {
            logFailExit("Failed to log in as as user B: " + giver1.getUsername()+" or could not send a gift to user C: " + receiver.getUsername());
        }
        new MiniProfile(driver).selectSignOut();

        // 3
        if (MacroLogin.startLogin(driver, receiver)) {
            logPass("Successfully logged in as " + receiver.getUsername());
        } else {
            logFailExit("Failed to log in as " + receiver.getUsername());
        }

        // 4
        NotificationCenter notificationCenter = new NotificationCenter(driver);
        new TakeOverPage(driver).closeIfOpen();
        if (notificationCenter.verifyGiftReceivedVisible()) {
            logPass("Successfully verified in the Home Page there is a message indicating a gift has been received.");
        } else {
            logFail("Failed to verify in the Home Page there is a message indicating a gift has been received.");
        }

        // 5
        if (notificationCenter.verifyGiftReceivedVisible(giver2.getUsername())) {
            logPass("The tile has the username of one of the givers.");
        } else {
            logFail("The tile does not have the username of one of the givers.");
        }

        // 6
        notificationCenter.clickYouGotGiftNotification();
        OpenGiftPage openGiftPage = new OpenGiftPage(driver);
        if (openGiftPage.verifyMessage(MacroPurchase.DEFAULT_GIFT_MESSAGE)) {
            logPass("The gift message is correct and there is only one.");
        } else {
            logFail("The gift message is incorrect and there is only one.");
        }
        softAssertAll();
    }
}