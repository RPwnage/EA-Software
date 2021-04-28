package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FileUtil;
import com.ea.vx.originclient.helpers.DesktopHelper;
import com.ea.vx.originclient.utils.Waits; 
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadEULASummaryDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadInfoDialog;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Tests that the options for including a start menu and/or desktop shortcut
 * when installing a game
 *
 * @author lscholte
 */
public abstract class OAShortcut extends EAXVxTestTemplate {

    /**
     * The main test function which all parameterized test cases call.
     *
     * @param context The context we are using
     * @param desktopShortcut True if game should be installed with a desktop
     * shortcut. False if the game should be installed with a start menu
     * shortcut
     * @param testName The name of the test. We need to pass this up so
     * initLogger properly attaches to the CRS test case.
     * @throws Exception
     */
    public void testShortcut(ITestContext context, boolean desktopShortcut, String testName) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        OADipSmallGame entitlement = new OADipSmallGame();
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();
        final String entitlementShortcutName = entitlement.getShortcutName();
        final DownloadOptions optionsWithNoShortcuts = new DownloadOptions().setDesktopShortcut(false).setStartMenuShortcut(false);

        DownloadOptions options;
        String shortcutPath;
        String shortcutType;

        if (desktopShortcut) {
            options = new DownloadOptions().setDesktopShortcut(true).setStartMenuShortcut(false);
            shortcutPath = OriginClientConstants.DESKTOP_PATH + entitlementShortcutName + ".lnk";
            shortcutType = "desktop";
        } else { //Start Menu shortcut
            options = new DownloadOptions().setDesktopShortcut(false).setStartMenuShortcut(true);
            shortcutPath = OriginClientConstants.START_MENU_PATH + entitlementShortcutName + "\\" + entitlementShortcutName + ".lnk";
            shortcutType = "start menu";
        }

        UserAccount user = AccountManager.getEntitledUserAccount(entitlement);

        logFlowPoint("Log into Origin as " + user.getUsername()); //1
        logFlowPoint("Navigate to the 'Game Library' page"); //2
        logFlowPoint("Verify the option 'Create desktop shortcut' exists."); //3
        logFlowPoint("Download " + entitlementName + " with a " + shortcutType + " shortcut"); //4
        logFlowPoint("Verify the shortcut exists"); //5
        logFlowPoint("Launch " + entitlementName + " using the " + shortcutType + " shortcut"); //6
        logFlowPoint("Uninstall " + entitlementName); //7
        logFlowPoint("Verify the " + shortcutType + " shortcut no longer exists"); //8
        logFlowPoint("Download " + entitlementName + " without opting to have any shortcuts"); //9
        logFlowPoint("Verify a " + shortcutType + " shortcut does not exist"); //10

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //3
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        gameTile.startDownload();
        new DownloadEULASummaryDialog(driver).clickNextButton();
        DownloadInfoDialog downloadInfoDialog = new DownloadInfoDialog(driver);
        logPassFail(downloadInfoDialog.verifyDesktopShortcutLabel(), false);

        //4
        downloadInfoDialog.clickCloseCircle();
        boolean downloaded = MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId, options);
        logPassFail(downloaded, true);

        //5
        logPassFail(FileUtil.isFileExist(client, shortcutPath), true);

        //6
        DesktopHelper.open(client, shortcutPath);
        entitlement.waitForGameLaunch(client);
        boolean entitlementRunning = Waits.pollingWaitEx(() -> entitlement.isLaunched(client));
        logPassFail(entitlementRunning, true);

        //7
        sleep(3000); //Let game stabilizes before killing it
        entitlement.killLaunchedProcess(client);

        //Closing a game causes Origin to refresh and navigate back to the
        //Discover Page. The verifyDiscoverPageReached accounts for that by
        //waiting a short period for it to be visible before proceeding
        new DiscoverPage(driver).verifyDiscoverPageReached();
        gameLibrary = navBar.gotoGameLibrary();

        entitlement.enableSilentUninstall(client);
        gameTile.uninstall();
        new MainMenu(driver).selectReloadMyGames();
        logPassFail(gameTile.waitForDownloadable(), true);

        //8
        logPassFail(!FileUtil.isFileExist(client, shortcutPath), false);

        //9
        //Without forcing a refresh, then the game library will automatically
        //refresh at an inopportune moment, so we instead refresh now so that
        //it won't refresh later
        new MainMenu(driver).selectRefresh();
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60);
        downloaded = MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId, optionsWithNoShortcuts);
        logPassFail(downloaded, true);

        //10
        logPassFail(!FileUtil.isFileExist(client, shortcutPath), true);

        softAssertAll();
    }
}