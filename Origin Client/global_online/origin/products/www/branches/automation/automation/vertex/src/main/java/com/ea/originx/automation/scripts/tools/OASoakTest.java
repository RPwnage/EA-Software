package com.ea.originx.automation.scripts.tools;

import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.TakeOverPage;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindow;
import com.ea.originx.automation.lib.pageobjects.social.Friend;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.pageobjects.store.BrowseGamesPage;
import com.ea.originx.automation.lib.pageobjects.store.StorePage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import java.util.HashMap;
import java.util.Map;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Soaks the test for a few hours.
 *
 * @author glivingstone
 */
public class OASoakTest extends EAXVxTestTemplate {

    @Test(groups = {"soaktest"})
    public void testSoak(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final int SLEEP_MINUTES = 460;
        final EntitlementInfo dipSmall = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        final UserAccount user = AccountManager.getEntitledUserAccount(dipSmall);
        final UserAccount friendAccount = AccountManager.getEntitledUserAccount(dipSmall);
        user.cleanFriends();
        friendAccount.cleanFriends();
        user.addFriend(friendAccount);

        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        // Weights for the random number generator, and value in case-switch.
        // These are the same for both browser and client
        Map<String, Integer> pageWeight = new HashMap<>();
        pageWeight.put("Discover Page", 2);
        pageWeight.put("Store Page", 3);
        pageWeight.put("Browse Games Page", 2);
        pageWeight.put("Deals & Exclusives Page", 1);
        pageWeight.put("Free Games Page", 1);
        pageWeight.put("Origin Access Page", 1);
        pageWeight.put("Game Library Page", 3);
        pageWeight.put("Game Details Page", 1);

        // These are different for client and browser
        if (isClient) {
            pageWeight.put("Login Page", 2);
            pageWeight.put("OIG", 1);
            pageWeight.put("Chat Window", 1);
        } else {
            pageWeight.put("Login Page", 0);
            pageWeight.put("OIG", 0);
            pageWeight.put("Chat Window", 0);
        }

        double totalWeight = 0;
        for (Map.Entry<String, Integer> weight : pageWeight.entrySet()) {
            totalWeight += weight.getValue();
        }

        double rand = Math.random() * totalWeight;
        int randInt = (int) rand;
        String soakTarget = null;
        for (Map.Entry<String, Integer> weight : pageWeight.entrySet()) {
            randInt -= weight.getValue();
            if (randInt <= 0) {
                soakTarget = weight.getKey();
                break;
            }
        }

        logFlowPoint("Login to Origin with " + user.getUsername()); // 1
        logFlowPoint("Soak Testing Target: " + soakTarget); // 2
        logFlowPoint("Check if Origin is Running After " + SLEEP_MINUTES / 4 + " Minutes"); // 3
        logFlowPoint("Check if Origin is Running After " + 2 * SLEEP_MINUTES / 4 + " Minutes"); // 4
        logFlowPoint("Check if Origin is Running After " + 3 * SLEEP_MINUTES / 4 + " Minutes"); // 5
        logFlowPoint("Check if Origin is Running After " + SLEEP_MINUTES + "  Minutes"); // 6
        logFlowPoint("Navigate to the Game Library Page"); // 7
        logFlowPoint("Navigate to the Discover Page"); // 8
        logFlowPoint("Navigate to the Browse Games Page"); // 9

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin with the user");
        } else {
            logFailExit("Could not log into Origin with the user");
        }

        // 2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        // Based on index, determine which soak test to do.
        switch (soakTarget) {
            case "Discover Page":
                navBar.gotoDiscoverPage();
                new TakeOverPage(driver).closeIfOpen();
                if (new DiscoverPage(driver).verifyDiscoverPageReached()) {
                    logPass("Navigated to the Discover Page.");
                } else {
                    logFailExit("Could not navigate to the Discover Page.");
                }
                break;
            case "Store Page":
                navBar.gotoStorePage();
                new TakeOverPage(driver).closeIfOpen();
                if (new StorePage(driver).verifyStorePageReached()) {
                    logPass("Navigated to the Store Page.");
                } else {
                    logFailExit("Could not navigate to the Store Page.");
                }
                break;
            case "Browse Games Page": // Browse Games
                navBar.gotoBrowseGamesPage();
                if (new BrowseGamesPage(driver).verifyBrowseGamesPageReached()) {
                    logPass("Navigated to the Browse Games Page.");
                } else {
                    logFailExit("Could not navigate to the Browse Games Page.");
                }
                break;
            case "Deals & Exclusives Page":
                navBar.clickDealsExclusivesLink();
                if (Waits.pollingWait(() -> navBar.verifyDealsExclusivesActive())) {
                    logPass("Navigated to the Deals & Exclusives Page.");
                } else {
                    logFailExit("Could not navigate to the Deals & Exclusives Page.");
                }
                break;
            case "Free Games Page":
                break;
            case "Origin Access Page":
                navBar.clickOriginAccessLink();
                if (Waits.pollingWait(() -> navBar.verifyOriginAccessActive())) {
                    logPass("Navigated to the Origin Access Page.");
                } else {
                    logFailExit("Could not navigate to the Origin Access Page.");
                }
                break;
            case "Game Library Page":
                navBar.gotoGameLibrary();
                if (new GameLibrary(driver).verifyGameLibraryPageReached()) {
                    logPass("Navigated to the Game Library Page.");
                } else {
                    logFailExit("Could not navigate to the Game Library Page.");
                }
                break;
            case "Login Page":
                new MainMenu(driver).selectLogOut();
                if (new LoginPage(driver).isOnLoginPage()) {
                    logPass("Navigated to the Login Page.");
                } else {
                    logFailExit("Could not navigate to the Login Page.");
                }
                break;
            case "OIG":
                navBar.gotoGameLibrary();
                MacroGameLibrary.downloadFullEntitlement(driver, dipSmall.getOfferId());
                GameTile gameTile = new GameTile(driver, dipSmall.getOfferId());
                gameTile.play();
                if (Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, dipSmall.getProcessName()))) {
                    logPass("Successfully launched DiP Small for OIG.");
                } else {
                    logFailExit("Could not launch DiP Small to open OIG.");
                }
                break;
            case "Chat Window":
                String[] message = {"Hello from the soak test!"};
                MacroSocial.restoreAndExpandFriends(driver);
                SocialHub socialHub = new SocialHub(driver);
                socialHub.verifyFriendVisible(friendAccount.getUsername());
                Friend friendTile = socialHub.getFriend(friendAccount.getUsername());
                friendTile.openChat();
                new ChatWindow(driver).sendMessage(message[0]);
                final OriginClient client2 = OriginClientFactory.create(context);
                sleep(2000);
                WebDriver driver2 = client2.getWebDriver();
                MacroLogin.startLogin(driver2, friendAccount);
                ChatWindow friendChat = new ChatWindow(driver);
                boolean sentMessage = new ChatWindow(driver).verifyMessagesInBubble(message);
                boolean receivedMessage = Waits.pollingWait(() -> friendChat.verifyMessagesInBubble(message));
                if (sentMessage && receivedMessage) {
                    logPass("Successfully opened a chat window.");
                } else {
                    logFailExit("Could not open a chat window.");
                }
                client2.stop();
                break;
            case "Game Details Page":
                navBar.gotoGameLibrary();
                GameTile gameTileGD = new GameTile(driver, dipSmall.getOfferId());
                GameSlideout gameSlideout = gameTileGD.openGameSlideout();
                if (gameSlideout.verifyGameTitle(dipSmall.getName())) {
                    logPass("Successfully opened the DiP Small slideout.");
                } else {
                    logFailExit("Could not open the DiP Small slideout.");
                }
                break;
        }

        String testExe = "chromedriver-2.19-win32.exe";
        if (isClient) {
            testExe = "Origin.exe";
        }

        // 3
        Waits.sleepMinutes(SLEEP_MINUTES / 4);
        if (ProcessUtil.isProcessRunning(client, testExe)) {
            logPass("Successfully waited for " + SLEEP_MINUTES / 4 + " minutes.");
        } else {
            logFailExit("Origin is no longer running.");
        }

        // 4
        Waits.sleepMinutes(SLEEP_MINUTES / 4);
        if (ProcessUtil.isProcessRunning(client, testExe)) {
            logPass("Successfully waited for " + 2 * SLEEP_MINUTES / 4 + " minutes.");
        } else {
            logFailExit("Origin is no longer running.");
        }

        // 5
        Waits.sleepMinutes(SLEEP_MINUTES / 4);
        if (ProcessUtil.isProcessRunning(client, testExe)) {
            logPass("Successfully waited for " + 3 * SLEEP_MINUTES / 4 + " minutes.");
        } else {
            logFailExit("Origin is no longer running.");
        }

        // 6
        Waits.sleepMinutes(SLEEP_MINUTES / 4);
        if (ProcessUtil.isProcessRunning(client, testExe)) {
            logPass("Successfully waited for " + SLEEP_MINUTES + " minutes.");
        } else {
            logFailExit("Origin is no longer running.");
        }

        // Login if we are stress testing the login page
        if (new LoginPage(driver).isOnLoginPage()) {
            new LoginPage(driver).quickLogin(user);
        }
        // Kill DiP Small if we are testing OIG
        if (ProcessUtil.isProcessRunning(client, dipSmall.getProcessName())) {
            ProcessUtil.killProcess(client, dipSmall.getProcessName());
        }

        // 7
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (Waits.pollingWait(() -> gameLibrary.verifyGameLibraryPageReached())) {
            logPass("Navigated to the Game Library Page.");
        } else {
            logFailExit("Could not navigate to the Game Library Page.");
        }

        // 8
        DiscoverPage discoverPage = navBar.gotoDiscoverPage();
        new TakeOverPage(driver).closeIfOpen();
        if (Waits.pollingWait(() -> discoverPage.verifyDiscoverPageReached())) {
            logPass("Navigated to the Discover Page.");
        } else {
            logFailExit("Could not navigate to the Discover Page.");
        }

        // 9
        navBar.gotoBrowseGamesPage();
        BrowseGamesPage browseGames = new BrowseGamesPage(driver);
        if (Waits.pollingWait(() -> browseGames.verifyBrowseGamesPageReached())) {
            logPass("Navigated to the Browse Games Page.");
        } else {
            logFailExit("Could not navigate to the Browse Games Page.");
        }

        softAssertAll();
    }
}
