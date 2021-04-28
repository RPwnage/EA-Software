package com.ea.originx.automation.lib.macroaction;

import com.ea.vx.originclient.account.UserAccount;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.TakeOverPage;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.discover.QuickLaunchTile;
import com.ea.originx.automation.lib.pageobjects.discover.RecFriendsSection;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import java.awt.AWTException;
import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;

/**
 * Macro class containing static methods for multi-step actions related to the 'My Home' page.
 *
 * @author palui
 */
public final class MacroDiscover {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     */
    private MacroDiscover() {
    }

    /**
     * Verify if a given user is recommended as a friend.
     *
     * @param driver Selenium WebDriver
     * @param user User to verify if recommended as a friend
     * @return true if user is recommended as a friend, false otherwise
     */
    public static boolean verifyUserRecommendedAsFriend(WebDriver driver, UserAccount user) {
        DiscoverPage discoverPage = new NavigationSidebar(driver).gotoDiscoverPage();
        new TakeOverPage(driver).closeIfOpen();
        if (!discoverPage.verifyDiscoverPageReached()) {
            throw new RuntimeException("Cannot open 'My Home' page");
        }
        if (discoverPage.verifyRecFriendsSectionVisible()) {
            RecFriendsSection recFriendsSection = discoverPage.getRecFriendsSection();
            recFriendsSection.expandRecFriendsTiles();
            return recFriendsSection.waitVerifyTileExist(user);
        }
        return false;
    }

    /**
     * Navigates to 'My Home' (if not already there) and clicks on the 'Quick Launch' tile
     *
     * @param driver Selenium WebDriver
     * @param entitlement The entitlement's 'Quick Launch' tile
     * @return true if the correct tile is found and clicked on, false otherwise
     */
    private static boolean clickOnQuickLaunchTile(WebDriver driver, EntitlementInfo entitlement) {
        //Check to see if already on My Home, if not attempt to navigate to
        if (!driver.getCurrentUrl().contains("/my-home") || !new NavigationSidebar(driver).gotoDiscoverPage().verifyDiscoverPageReached()) {
            _log.debug("Not able to navigate to My Home");
            return false;
        }
        QuickLaunchTile tile = new QuickLaunchTile(driver, entitlement.getOfferId());
        //Check to see if tile is on My Home
        if (!tile.verifyTileVisible()) {
            _log.debug("Tile is not on My Home");
            return false;
        }
        
        tile.clickTile();
        MacroGameLibrary.handlePlayDialogs(driver);
        
        return true;
    }

    /**
     * Clicks on the 'Quick Launch' tile for a given entitlement and checks if the game is running. If true,
     * it also closes the game. This is used if the entitlement launches a different process name.
     *
     * @param driver the current Web Driver
     * @param entitlement the entitlement Quick Launch tile
     * @return true if correct tile is found and clicked on
     */
    private static boolean clickOnQuickLaunchTile(WebDriver driver, String offerId) {
        //Check to see if already on My Home, if not attempt to navigate to
        if (!driver.getCurrentUrl().contains("/my-home") || !new NavigationSidebar(driver).gotoDiscoverPage().verifyDiscoverPageReached()) {
            _log.debug("Not able to navigate to My Home");
            return false;
        }
        QuickLaunchTile tile = new QuickLaunchTile(driver, offerId);
        //Check to see if tile is on My Home
        if (!tile.verifyTileVisible()) {
            _log.debug("Tile is not on My Home");
            return false;
        }

        tile.clickTile();
        MacroGameLibrary.handlePlayDialogs(driver);

        return true;
    }

    /**
     * Clicks on the 'Quick Launch' tile for a given entitlement and checks if the game is running. If true,
     * it closes the game.
     *
     * @param client The Origin client object
     * @param driver Selenium WebDriver
     * @param entitlement The entitlement that is being launched
     * @return true if process is running after 'Quick Launch' tile is selected, false otherwise
     */
    static public boolean verifyQuickLaunchGameLaunchEntitlement(OriginClient client, WebDriver driver, EntitlementInfo entitlement) {
        _log.debug("Launching " + entitlement.getName() + " via Quick Launch");
        if (!clickOnQuickLaunchTile(driver, entitlement)) {
            return false;
        }
        try {
            boolean gameLaunched = entitlement.isLaunched(client);
            _log.debug("Able to launch " + entitlement.getName() + "?: " + gameLaunched);
            if (gameLaunched) {
                entitlement.killLaunchedProcess(client);
                _log.debug("Successfully closed launched");
            }
            return gameLaunched;
        } catch (IOException | InterruptedException exception) {
            _log.debug("Exception occured: " + exception);
            return false;
        }
    }

    /**
     * Clicks on Quick Launch tile and checks if the game is running. If true,
     * closes the game.
     *
     * @param client the Origin client object
     * @param driver the current WebDriver
     * @param entitlementName the name of the game to be launched
     * @param entitlementOfferId the offer ID of the game to be launched
     * @param entitlementProcessName the process name that shows in task manager
     * of the game to be launched
     * @return true if process is running after Quick Launch tile is selected
     */
    static public boolean verifyQuickLaunchGameLaunchEntitlement(OriginClient client, WebDriver driver, String entitlementName, String entitlementOfferId, String entitlementProcessName) {
        _log.debug("Launching " + entitlementName + " via Quick Launch");
        if (!clickOnQuickLaunchTile(driver, entitlementOfferId)) {
            return false;
        }

        boolean gameLaunched = Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, entitlementProcessName));
        _log.debug("Able to launch " + entitlementName + "?: " + gameLaunched);
        if (gameLaunched) {
            ProcessUtil.killProcess(client, entitlementProcessName);
            _log.debug("Successfully closed launched game");
        }

        return gameLaunched;
    }
    
    /**
     * Handle download dialogs including: language selection, EULA, download
     * info, warning and error dialogs
     *
     * @param driver                        Selenium WebDriver for the client
     * @param offerId                       the offer ID to download
     * @param options                       {@link DownloadOptions}<br>
     *                                      UI options:<br>
     *                                      - desktopShortcut if true, check the 'Desktop Shortcut' check box. If
     *                                      false, un-check<br>
     *                                      - startMenuShortcut if true, check the 'Start Menu Shortcut' check box.
     *                                      If false, un-check<br>
     *                                      - uncheckExtraContent if true, un-check the 'Extra Content' check boxes
     *                                      (can use UncheckExtraContentExceptions parameter to check some back). If
     *                                      false, do not un-check (and the UncheckExtraContentExceptions parameter
     *                                      is ignored)<br>
     *                                      Flow options:<br>
     *                                      - installFromDisc if true, we are installing from (ITO) disc and a
     *                                      download preparation dialog should have already been opened. If false,
     *                                      initiate download from the game tile's context menu <br>
     *                                      - stopAtEULA if true, stop downlaod when the EULA dialog appears. If
     *                                      false, continue to end of install<br>
     * @param UncheckExtraContentExceptions if options specify all extra
     *                                      contents to be unchecked, this list of offer-ids will then be checked
     *                                      back. Useful when only a subset (often one) of the extra contents should
     *                                      be checked
     * @return true if the download preparation dialogs were successfully
     * handled. If stopAtEula is true, then this will return true once the EULA
     * dialog is open, or false if is never opened. If stopAtEula is false, then
     * this will return true once all the download preparation dialogs have been
     * handled and are closed, or false if something goes wrong
     * @throws AWTException
     */
    public static boolean handleQuickLaunchTileDownloadDialogs(WebDriver driver, String offerId,
                                        DownloadOptions options, List<String> UncheckExtraContentExceptions)
            throws AWTException {

        QuickLaunchTile quickLaunchTile = new QuickLaunchTile(driver, offerId);
        if (!quickLaunchTile.verifyEyebrowStatePreparingPresent()) {
            _log.debug("Never reached the preparing phase, trying to download anyway");
        }

        if (MacroGameLibrary.handleDialogs(driver, offerId, options)) {
            if (quickLaunchTile.verifyEyebrowStatePreparingPresent()) {
                _log.debug("The game is still preparing to download, preparation dialog handling failed");
                return false;
            }
        }

        return true;
    }
}
