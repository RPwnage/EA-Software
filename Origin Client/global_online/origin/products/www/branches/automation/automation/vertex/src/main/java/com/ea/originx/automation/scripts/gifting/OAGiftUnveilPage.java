package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroGifting;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.NotificationCenter;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadLanguageSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.discover.OpenGiftPage;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPNavBar;
import com.ea.originx.automation.lib.pageobjects.store.StorePage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the content of the page that appears when unveiling a gift.
 *
 * @author glivingstone
 */
public class OAGiftUnveilPage extends EAXVxTestTemplate {

    @TestRail(caseId = 1016697)
    @Test(groups = {"gifting", "full_regression", "release_smoke", "int_only"})
    public void testGiftUnveilPage(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final OriginClient friendClient = OriginClientFactory.create(context);
        UserAccount giftSender;
        UserAccount giftReceiver;
        final String environment = OSInfo.getEnvironment();
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        
        if (environment.equalsIgnoreCase("production")) {
            giftSender = AccountManagerHelper.createNewThrowAwayAccount(CountryInfo.CountryEnum.CANADA.getCountry());
            giftReceiver = AccountManagerHelper.createNewThrowAwayAccount(CountryInfo.CountryEnum.CANADA.getCountry());
        } else {
            giftSender = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(CountryInfo.CountryEnum.CANADA.getCountry());
            giftReceiver = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(CountryInfo.CountryEnum.CANADA.getCountry());
            giftReceiver.addFriend(giftSender);
        }

        final EntitlementInfo firstEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final EntitlementInfo secondEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);
        final String giftSenderName = giftSender.getFirstName() + " " + giftSender.getLastName();
        final String giftSenderUserName = giftSender.getUsername();
        final String giftReceiverUserName = giftReceiver.getUsername();
        final String customMessage = "Received Gift Page Test";

        logFlowPoint("Log into Origin");    //1
        logFlowPoint("Log into Origin with gifted account"); //2
        logFlowPoint("Gift firstEntitlement to an eligible account");  //3
        logFlowPoint("Repeat previous step with a different offer");    //4
        logFlowPoint("Navigate to 'Game Library' and verify the gifted games do not appear until unveiled");  //5
        logFlowPoint("Hover over Notification Center and verify unopened gift appears");   //6
        logFlowPoint("Click on gift notification and verify gift is unveiled to the User");   //7
        logFlowPoint("Verify that the animation will take up the whole window");    //8
        logFlowPoint("Verify that packart for the game is displayed");  //9
        logFlowPoint("Verify that the sender avatar is displayed"); //10
        logFlowPoint("Verify that the gift message is displayed");  //11
        logFlowPoint("Verify messaging that game has been added to game library"); //12
        logFlowPoint("Verify that a close button appears on the top right of the screen"); //13
        if (isClient) {
            logFlowPoint("Verify that there is a download CTA button"); //14
            logFlowPoint("Click on download button and verify that the user is taken to the 'Game Library' and entitlement is visible"); //15
            logFlowPoint("Verify that the download/install flow is initiated"); //16

        } else {
            logFlowPoint("Click close button and verify user is directed to 'Game Library'");  //17
        }
        logFlowPoint("Go to the 'Store Home' page and open another gift in the notification centre");  //18
        logFlowPoint("Click on the close button and verify user is directed back to the 'Store Home' page");   //19
        logFlowPoint("Return to the 'Game Library' and verify that gifted game is now entitled and scrolled to in game library");  //20

        // 1
        final WebDriver driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, "/can/en-us");
        logPassFail(MacroLogin.startLogin(driver, giftSender), true);
        
        // 2
        WebDriver friendDriver = startClientObject(context, friendClient);
        logPassFail(MacroLogin.startLogin(friendDriver, giftReceiver), true);
        if (environment.equalsIgnoreCase("production")) {
            MacroSocial.addFriendThroughUI(friendDriver, driver, giftReceiver, giftSender);
        }

        //3
        if (environment.equalsIgnoreCase("production")) {
            logPassFail(MacroGifting.purchaseGiftForFriendThroughPaypalOffCode(driver, firstEntitlement, giftReceiverUserName, customMessage, giftSender.getEmail()), true);
        } else {
            logPassFail(MacroGifting.purchaseGiftForFriend(driver, firstEntitlement, giftReceiverUserName, customMessage), true);
        }

        //4
        MacroGDP.loadGdpPage(driver, secondEntitlement);
        String packArt = new GDPNavBar(driver).getPackArtSrc();
         if (environment.equalsIgnoreCase("production")) {
            logPassFail(MacroGifting.purchaseGiftForFriendThroughPaypalOffCode(driver, secondEntitlement, giftReceiverUserName, customMessage, giftSender.getEmail()), true);
        } else {
            logPassFail(MacroGifting.purchaseGiftForFriend(driver, secondEntitlement, giftReceiverUserName, customMessage), true);
        }
        
        //5
        NavigationSidebar navSideBar = new NavigationSidebar(friendDriver);
        GameLibrary gameLibrary = navSideBar.gotoGameLibrary();
        logPassFail(!gameLibrary.isGameTileVisibleByName(firstEntitlement.getName()) && !gameLibrary.isGameTileVisibleByName(secondEntitlement.getName()), true);

        //6
        NotificationCenter notificationCenter = new NotificationCenter(friendDriver);
        logPassFail(notificationCenter.verifyGiftReceivedVisible(giftSenderName), true);

        //7
        notificationCenter.clickOnGiftNotification(giftSenderName);
        OpenGiftPage openGift = new OpenGiftPage(friendDriver);
        logPassFail(openGift.waitForVisible(), true);

        //8
        logPassFail(openGift.verifyFullScreenUnveil(), false);

        //9
        logPassFail(openGift.verifyPackArtDisplayed(packArt), false);

        //10
        logPassFail(openGift.verifyAvatarMatchesUser(giftSenderUserName), false);

        //11
//        logPassFail(openGift.verifyMessage(customMessage), false);

        //12
        logPassFail(openGift.verifyAddedToLibraryMessageVisible(), false);

        //13
        logPassFail(openGift.verifyCloseButtonVisible(), false);

        if (isClient) {
            //14
            logPassFail(openGift.verifyPrimaryCTA(), true);

            //15
            openGift.clickDownloadNow();
            logPassFail(gameLibrary.isGameTileVisibleByName(secondEntitlement.getName()), false);
            
            //16
            DownloadLanguageSelectionDialog languageDialog = new DownloadLanguageSelectionDialog(friendDriver);
            logPassFail(languageDialog.waitIsVisible(), true);
            languageDialog.clickCloseCircle();
        } else {
            //17
            openGift.clickCloseButton();
            logPassFail(gameLibrary.verifyGameLibraryPageReached(), false);
        }

        //18
        navSideBar.gotoStorePage();
        notificationCenter.clickOnGiftNotification(giftSenderName);
        logPassFail(openGift.verifyFullScreenUnveil(), true);

        //19
        openGift.clickCloseButton();
        StorePage storePage = new StorePage(friendDriver);
        storePage.waitForStorePageToLoad();
        logPassFail(storePage.verifyStorePageReached(), true);

        //20
        navSideBar.gotoGameLibrary();
        logPassFail(gameLibrary.isGameTileVisibleByName(firstEntitlement.getName()), false);

        softAssertAll();
    }
}