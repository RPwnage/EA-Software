package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindow;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import java.util.concurrent.Callable;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Tests that a conversation banner appears in the chat window when that user is
 * playing a game.
 *
 * @author lscholte
 */
public abstract class OAChatGameBanner extends EAXVxTestTemplate {

    /**
     * The main test function which all parameterized test cases call.
     *
     * @param context       The context we are using
     * @param ownsGame      Whether User B owns the game that appears in the chat
     *                      game banner
     * @param chatPoppedOut Whether the chat window should be popped out
     *                      initLogger properly attaches to the CRS test case.
     * @throws Exception
     */
    public void testChatGameBanner(ITestContext context, boolean ownsGame, boolean chatPoppedOut) throws Exception {

        final OriginClient clientA = OriginClientFactory.create(context);
        final OriginClient clientB = OriginClientFactory.create(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BIG_MONEY);
        final String processName = entitlement.getProcessName();
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        //Regex for URL that contains social-chatwindow but does not contain oigContext
        final String chatWindowPoppedOutRegex = ".*social-chatwindow((?!oigContext).)*";

        UserAccount userA = AccountManager.getEntitledUserAccount(entitlement);
        UserAccount userB = ownsGame
                ? AccountManager.getEntitledUserAccount(entitlement)
                : AccountManager.getUnentitledUserAccount();

        userA.cleanFriends();
        userB.cleanFriends();
        userA.addFriend(userB);

        logFlowPoint("Log into Origin with " + userA.getUsername() + " as User A"); //1
        logFlowPoint("Log into Origin with " + userB.getUsername() + " as User B"); //2
        logFlowPoint("As User B, start a chat with User A"); //3
        if (chatPoppedOut) {
            logFlowPoint("Pop out the chat window"); //4
        }
        logFlowPoint("Navigate User A to the game library page"); //5
        logFlowPoint("Download " + entitlementName); //6
        logFlowPoint("Launch " + entitlementName); //7
        logFlowPoint("Verify that a game banner appears in the chat window"); //8
        logFlowPoint("Verify that the game title in the banner is " + entitlementName); //9
        logFlowPoint("Verify that the game banner does not block any of the chat history"); //10
        if (ownsGame) {
            logFlowPoint("Verify that clicking game title in the game banner opens the game's details slideout"); //11a
        } else {
            logFlowPoint("Verify that clicking game title in the game banner opens the store page"); //11b
        }
        logFlowPoint("Close " + entitlementName); //12
        logFlowPoint("Verify that the game banner disappears from the chat window"); //13
        logFlowPoint("Verify that the chat history refills the space where the game banner used to be"); //14

        //1
        WebDriver driverA = startClientObject(context, clientA);
        if (MacroLogin.startLogin(driverA, userA)) {
            logPass("Successfully logged into Origin as User A");
        } else {
            logFailExit("Could not log into Origin as User A");
        }

        //2
        WebDriver driverB = startClientObject(context, clientB);
        if (MacroLogin.startLogin(driverB, userB)) {
            logPass("Successfully logged into Origin as User B");
        } else {
            logFailExit("Could not log into Origin as User B");
        }

        //3
        MacroSocial.restoreAndExpandFriends(driverB);
        SocialHub socialHub = new SocialHub(driverB);
        socialHub.verifyFriendVisible(userA.getUsername()); //Should give time for social hub to restore
        socialHub.getFriend(userA.getUsername()).openChat();
        ChatWindow chatWindow = new ChatWindow(driverB);
        if (chatWindow.verifyChatOpen()) {
            logPass("Successfully started a chat with User A");
        } else {
            logFailExit("Failed to start a chat with User A");
        }

        //4
        if (chatPoppedOut) {
            chatWindow.popoutChatWindow();

            Callable<Boolean> switchToPoppedOutCallable = () -> {
                try {
                    Waits.switchToPageThatMatches(driverB, chatWindowPoppedOutRegex);
                } catch (RuntimeException e) {
                    return false;
                }
                return true;
            };

            boolean chatWindowPoppedOut = Waits.pollingWaitEx(switchToPoppedOutCallable);
            boolean chatWindowDisplayed = chatWindow.verifyChatOpen();
            if (chatWindowPoppedOut && chatWindowDisplayed) {
                logPass("Successfully popped out the chat window");
            } else {
                logFailExit("Failed to pop out the chat window");
            }
        }

        //5
        NavigationSidebar navBar = new NavigationSidebar(driverA);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigated User A to the game library page");
        } else {
            logFailExit("Failed to navigate User A to the game library page");
        }

        //6
        boolean downloaded = MacroGameLibrary.downloadFullEntitlement(driverA, entitlementOfferId);
        if (downloaded) {
            logPass("Successfully downloaded " + entitlementName);
        } else {
            logFailExit("Failed to download " + entitlementName);
        }

        //7
        new GameTile(driverA, entitlementOfferId).play();
        boolean entitlementRunning = Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(clientA, processName));
        if (entitlementRunning) {
            logPass("Successfully launched " + entitlementName);
        } else {
            logFailExit("Failed to launch " + entitlementName);
        }

        //8
        if (chatWindow.verifyBannerVisible()) {
            logPass("The game banner is displayed in the chat window");
        } else {
            logFailExit("The game banner is not displayed in the chat window");
        }

        //9
        if (chatWindow.verifyBannerGameTitle(entitlementName)) {
            logPass("The game banner displays the correct game title");
        } else {
            logFail("The game banner does not display the correct game title");
        }

        //10
        if (chatWindow.verifyChatHistoryPosition()) {
            logPass("The game banner is not blocking a portion of the conversation area");
        } else {
            logFail("The game banner is blocking a portion of the conversation area");
        }

        chatWindow.clickGameBannerTitleLink();
        if (chatPoppedOut) {
            Waits.switchToPageThatMatches(driverB, OriginClientData.MAIN_SPA_PAGE_URL);
        }
        if (ownsGame) {
            //11a
            GameSlideout gameSlideout = new GameSlideout(driverB);
            if (gameSlideout.verifyGameTitle(entitlementName)) {
                logPass("Successfully opened the game details slideout");
            } else {
                logFail("Failed to open the game details slideout");
            }
        } else //11b
//            if (new PDPHero(driverB).verifyPDPHeroReached()) {
        //                logPass("Successfully opened the PDP of " + entitlementName);
        //            } else {
        //                logFail("Failed to open the PDP of " + entitlementName);
        //            }
        if (chatPoppedOut) {
            Waits.switchToPageThatMatches(driverB, chatWindowPoppedOutRegex);
        }

        //12
        ProcessUtil.killProcess(clientA, processName);
        if (!ProcessUtil.isProcessRunning(clientA, processName)) {
            logPass("Successfully killed " + entitlementName);
        } else {
            logFailExit("Failed to kill " + entitlementName);
        }

        //13
        boolean chatBannerNotVisible = Waits.pollingWait(() -> !chatWindow.verifyBannerVisible());
        if (chatBannerNotVisible) {
            logPass("The game banner is no longer visible in the chat window");
        } else {
            logFailExit("The game banner is still visible in the chat window");
        }

        //14
        if (chatWindow.verifyChatHistoryPosition()) {
            logPass("The chat history was restored to its original size");
        } else {
            logFail("The chat history was not restored to its original size");
        }

        //TODO: Verify that twitch broadcasting changes the chat game banner to
        //say that the user is broadcasting. This can't be done right now because
        //we are unable to broadcast BigMoney from the OIG menu
        softAssertAll();
    }
}
