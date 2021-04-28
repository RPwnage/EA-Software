package com.ea.originx.automation.lib.pageobjects.common;

import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.pageobjects.dialog.AboutDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.pageobjects.template.OAQtSiteTemplate;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.Dimension;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.support.ui.ExpectedConditions;
import org.openqa.selenium.support.ui.WebDriverWait;
import java.util.List;

/**
 * Used to drive the main menu bar on the top of the Origin client
 *
 * @author glivingstone
 */
public class MainMenu extends OAQtSiteTemplate {

    private static final Logger _log = LogManager.getLogger(MainMenu.class);

    protected static final By MENU_CONTAINER_LOCATOR = By.id("menuContainer");
    protected static final By MAIN_MENU_BAR_LOCATOR = By.xpath("//*[@id='menuContainer']/*[@id='mainMenuBar']");
    protected static final By MAIN_MENUS_LOCATOR = By.xpath("//*[@id='menuContainer']/*[@id='mainMenuBar']/QAction");
    protected static final By ORIGIN_MENU_LOCATOR = By.xpath("//*[@id='menuContainer']/*[@id='mainMenuBar']/QAction[1]");
    protected static final By GAMES_MENU_LOCATOR = By.xpath("//*[@id='menuContainer']/*[@id='mainMenuBar']/QAction[2]");
    protected static final By FRIENDS_MENU_LOCATOR = By.xpath("//*[@id='menuContainer']/*[@id='mainMenuBar']/QAction[3]");
    protected static final By VIEW_MENU_LOCATOR = By.xpath("//*[@id='menuContainer']/*[@id='mainMenuBar']/QAction[4]");
    protected static final By HELP_MENU_LOCATOR = By.xpath("//*[@id='menuContainer']/*[@id='mainMenuBar']/QAction[5]");
    protected static final By DEBUG_MENU_LOCATOR = By.xpath("//*[@id='menuContainer']/*[@id='mainMenuBar']/QAction[6]");
    protected static final By VIEW_MENU_UNDERAGE_LOCATOR = By.xpath("//*[@id='menuContainer']/*[@id='mainMenuBar']/QAction[3]");
    protected static final By HELP_MENU_UNDERAGE_LOCATOR = By.xpath("//*[@id='menuContainer']/*[@id='mainMenuBar']/QAction[4]");

    protected static final By ORIGIN_MENU_ITEMS_LOCATOR = By.xpath("//*[@id='mainOriginMenu']/QAction");
    protected static final By APP_SETTINGS_LOCATOR = By.xpath("//*[@id='mainOriginMenu']/QAction[1]");
    protected static final By ACCOUNT_BILLING_LOCATOR = By.xpath("//*[@id='mainOriginMenu']/QAction[2]");
    protected static final By ORDER_HISTORY_LOCATOR = By.xpath("//*[@id='mainOriginMenu']/QAction[3]");
    protected static final By REDEEM_PRODUCT_CODE_LOCATOR = By.xpath("//*[@id='mainOriginMenu']/QAction[4]");
    protected static final By REFRESH_LOCATOR = By.xpath("//*[@id='mainOriginMenu']/QAction[6]");
    protected static final By GO_OFFLINE_ONLINE_LOCATOR = By.xpath("//*[@id='mainOriginMenu']/QAction[8]");
    protected static final By LOGOUT_LOCATOR = By.xpath("//*[@id='mainOriginMenu']/QAction[9]");
    protected static final By EXIT_LOCATOR = By.xpath("//*[@id='mainOriginMenu']/QAction[10]");

    protected static final By UNDERAGE_REDEEM_PRODUCT_CODE_LOCATOR = By.xpath("//*[@id='mainOriginMenu']/QAction[2]");
    protected static final By UNDERAGE_REFRESH_LOCATOR = By.xpath("//*[@id='mainOriginMenu']/QAction[4]");
    protected static final By UNDERAGE_GO_OFFLINE_ONLINE_LOCATOR = By.xpath("//*[@id='mainOriginMenu']/QAction[6]");
    protected static final By UNDERAGE_LOGOUT_LOCATOR = By.xpath("//*[@id='mainOriginMenu']/QAction[7]");
    protected static final By UNDERAGE_EXIT_LOCATOR = By.xpath("//*[@id='mainOriginMenu']/QAction[8]");

    protected static final By GAMES_MENU_ITEMS_LOCATOR = By.xpath("//*[@id='mainGamesMenu']/QAction");
    protected static final By ADD_NON_ORIGIN_GAME_LOCATOR = By.xpath("//*[@id='mainGamesMenu']/QAction[1]");
    protected static final By RELOAD_MY_GAMES_LOCATOR = By.xpath("//*[@id='mainGamesMenu']/QAction[2]");

    protected static final By SUBSCRIBER_ADD_VAULT_GAME_LOCATOR = By.xpath("//*[@id='mainGamesMenu']/QAction[1]");
    protected static final By SUBSCRIBER_ADD_NON_ORIGIN_GAME_LOCATOR = By.xpath("//*[@id='mainGamesMenu']/QAction[2]");
    protected static final By SUBSCRIBER_RELOAD_MY_GAMES_LOCATOR = By.xpath("//*[@id='mainGamesMenu']/QAction[3]");
    protected static final By UNDERAGE_RELOAD_GAME_LIBRARY_LOCATOR = By.xpath("//*[@id='mainGamesMenu']/QAction[2]");

    protected static final By FRIENDS_MENU_ITEMS_LOCATOR = By.xpath("//*[@id='mainFriendsMenu']/QAction");
    protected static final By ADD_FRIEND_LOCATOR = By.xpath("//*[@id='mainFriendsMenu']/QAction[1]");
    protected static final By SET_STATUS_LOCATOR = By.xpath("//*[@id='mainFriendsMenu']/QAction[3]");
    protected static final By SET_STATUS_MENU_ITEMS_LOCATOR = By.xpath("//*[@id='mainSetStatusMenu']/QAction");

    protected static final By SET_STATUS_ONLINE_LOCATOR = By.xpath("//*[@id='mainSetStatusMenu']/QAction[1]");
    protected static final By SET_STATUS_AWAY_LOCATOR = By.xpath("//*[@id='mainSetStatusMenu']/QAction[2]");
    protected static final By SET_STATUS_INVISIBLE_LOCATOR = By.xpath("//*[@id='mainSetStatusMenu']/QAction[3]");

    protected static final By VIEW_MENU_ITEMS_LOCATOR = By.xpath("//*[@id='mainView']/QAction");
    protected static final By VIEW_DISCOVER_LOCATOR = By.xpath("//*[@id='mainView']/QAction[1]");
    protected static final By VIEW_STORE_LOCATOR = By.xpath("//*[@id='mainView']/QAction[2]");
    protected static final By VIEW_GAME_LIBRARY_LOCATOR = By.xpath("//*[@id='mainView']/QAction[3]");
    protected static final By VIEW_DOWNLOAD_QUEUE_LOCATOR = By.xpath("//*[@id='mainView']/QAction[4]");
    protected static final By VIEW_FRIENDS_LIST_LOCATOR = By.xpath("//*[@id='mainView']/QAction[5]");
    protected static final By VIEW_ACTIVE_CHAT_LOCATOR = By.xpath("//*[@id='mainView']/QAction[6]");
    protected static final By VIEW_MY_PROFILE_LOCATOR = By.xpath("//*[@id='mainView']/QAction[7]");
    protected static final By VIEW_ACHIEVEMENTS_LOCATOR = By.xpath("//*[@id='mainView']/QAction[8]");
    protected static final By VIEW_WISHLIST_LOCATOR = By.xpath("//*[@id='mainView']/QAction[9]");
    protected static final By VIEW_SEARCH_LOCATOR = By.xpath("//*[@id='mainView']/QAction[10]");

    protected static final By UNDERAGE_VIEW_GAME_LIBRARY_LOCATOR = By.xpath("//*[@id='mainView']/QAction[1]");
    protected static final By UNDERAGE_VIEW_DOWNLOAD_QUEUE_LOCATOR = By.xpath("//*[@id='mainView']/QAction[2]");
    protected static final By UNDERAGE_VIEW_SEARCH_LOCATOR = By.xpath("//*[@id='mainView']/QAction[4]");

    protected static final By HELP_MENU_ITEMS_LOCATOR = By.xpath("//*[@id='mainHelpMenu']/QAction");
    protected static final By ORIGIN_HELP_LOCATOR = By.xpath("//*[@id='mainHelpMenu']/QAction[1]");
    protected static final By ORIGIN_FORUM_LOCATOR = By.xpath("//*[@id='mainHelpMenu']/QAction[2]");
    protected static final By ORIGIN_ERROR_REPORTER_LOCATOR = By.xpath("//*[@id='mainHelpMenu']/QAction[3]");
    protected static final By SHORTCUTS_LOCATOR = By.xpath("//*[@id='mainHelpMenu']/QAction[5]");
    protected static final By ABOUT_LOCATOR = By.xpath("//*[@id='mainHelpMenu']/QAction[7]");

    protected static final By DEBUG_MENU_ITEMS_LOCATOR = By.xpath("//*[@id='mainDebugMenu']/QAction");
    protected static final By LOG_DIRTY_BITS_LOCATOR = By.xpath("//*[@id='mainDebugMenu']/QAction[4]");
    protected static final By DIRTY_BITS_CONNECT_SAMPLE_HANDLERS_LOCATOR = By.xpath("//*[@id='mainDebugMenu']/QAction[6]");
    protected static final By LOG_DIRTY_BITS_VIEWER_LOCATOR = By.xpath("//*[@id='mainDebugMenu']/QAction[8]");

    protected static final By UNDERAGE_ORIGIN_ERROR_REPORTER_LOCATOR = By.xpath("//*[@id='mainHelpMenu']/QAction[1]");
    protected static final By UNDERAGE_SHORTCUTS_LOCATOR = By.xpath("//*[@id='mainHelpMenu']/QAction[3]");
    protected static final By UNDERAGE_ABOUT_LOCATOR = By.xpath("//*[@id='mainHelpMenu']/QAction[5]");

    // All the regex navigators
    protected static final String SIGNIN_URL_REGEX = "https?://signin.*";
    protected static final String REDEEM_CODE_URL_REGEX = "http.*checkout.*redeemcode.*startRedeemCode.*";

    // TODO: fix all of these routes to the correct paths when they are available in the SPA
    protected static final String BROWSE_GAMES_ROUTE_URL_REGEX = ".*store/.*games.*";
    protected static final String FREE_GAMES_ROUTE_URL_REGEX = ".*store/.*games.*";
    protected static final String ACCOUNT_PRIVACY_ROUTE_URL_REGEX = ".*settings.*";
    protected static final String ORDER_HISTORY_ROUTE_URL_REGEX = ".*settings.*";

    protected static final String GAME_LIBRARY_ROUTE_URL_REGEX = ".*game-library.*";
    protected static final String APP_SETTINGS_ROUTE_URL_REGEX = ".*settings.*";
    protected static final String PROFILE_ROUTE_URL_REGEX = ".*profile.*";

    protected static final By WINDOW_TITLE_LABEL_LOCATOR = By.xpath("//*[@id='lblWindowTitle']");
    protected static final By DEBUG_MODE_LABEL_LOCATOR = By.xpath("//*[@id='lblDebugMode']");

    protected static final By OFFLINE_MODE_BUTTON_LOCATOR = By.xpath("//*[@id='btnOfflineMode']");
    protected static final By MINIMIZE_WINDOW_BUTTON_LOCATOR = By.xpath("//*[@id='btnMinimize']");
    protected static final By MAXIMIZE_WINDOW_BUTTON_LOCATOR = By.xpath("//*[@id='btnMaximize']");
    protected static final By RESTORE_WINDOW_BUTTON_LOCATOR = By.xpath("//*[@id='btnRestore']");
    protected static final By CLOSE_WINDOW_BUTTON_LOCATOR = By.xpath("//*[@id='btnClose']");

    protected static final String OFFLINE_MODE_BUTTON_TEXT = "OFFLINE MODE";
    
    // Element used to check Page Refresh against
    protected static final By SEARCH_BOX_LOCATOR = By.cssSelector("origin-global-search .origin-globalsearch > label > div > input");

    private final WebDriver chromeDriver;

    /**
     * Constructor
     *
     * @param webDriver Selenium WebDriver
     */
    public MainMenu(WebDriver webDriver) {
        super(webDriver);
        chromeDriver = OriginClient.getInstance(webDriver).getWebDriver();
    }

    /**
     * Click "Minimize" button to the top right of the main menu, which should
     * minimize the Origin window, although Origin should still be running in
     * the background
     */
    public void clickMinimizeButton() {
        switchToOriginWindow();
        waitForElementClickable(MINIMIZE_WINDOW_BUTTON_LOCATOR).click();
    }

    /**
     * Click "Maximize" button to the top right of the main menu, which should
     * maximize the Origin window. This can be used to maximize screen
     * real-estate for tests<br>
     * NOTE: Cannot maximize a minimized window. Restore the window first
     */
    public void clickMaximizeButton() {
        switchToOriginWindow();
        waitForElementClickable(MAXIMIZE_WINDOW_BUTTON_LOCATOR).click();
    }

    /**
     * Click "Restore" button when Origin has been minimized or maximized, which
     * restores the Origin window to its default size
     */
    public void clickRestoreButton() {
        switchToOriginWindow();
        waitForElementClickable(RESTORE_WINDOW_BUTTON_LOCATOR).click();
    }

    /**
     * Click the Close button, which should close the Origin window, although
     * Origin should still be running in the background
     */
    public void clickCloseButton() {
        switchToOriginWindow();
        waitForElementClickable(CLOSE_WINDOW_BUTTON_LOCATOR).click();
    }

    /**
     * Open the given menu on the main menu bar
     *
     * @param locator The locator of the item to select
     */
    private void openMenu(By locator) {
        switchToOriginWindow();
        waitForElementClickable(locator).click();
    }

    /**
     * Open the Origin Main Menu
     */
    public void openOriginMenu() {
        openMenu(ORIGIN_MENU_LOCATOR);
    }

    /**
     * Open the Games Main Menu
     */
    public void openGamesMenu() {
        openMenu(GAMES_MENU_LOCATOR);
    }

    /**
     * Open the Friends Main Menu
     */
    public void openFriendsMenu() {
        openMenu(FRIENDS_MENU_LOCATOR);
    }

    /**
     * Open the Friends Main Menu
     *
     * @param underage Whether we are using an underage account or not
     */
    public void openViewMenu(boolean underage) {
        if (underage) {
            openMenu(VIEW_MENU_UNDERAGE_LOCATOR);
        } else {
            openMenu(VIEW_MENU_LOCATOR);
        }
    }

    /**
     * Open the Help Main Menu
     *
     * @param underage Whether we are using an underage account or not
     */
    public void openHelpMenu(boolean underage) {
        if (underage) {
            openMenu(HELP_MENU_UNDERAGE_LOCATOR);
        } else {
            openMenu(HELP_MENU_LOCATOR);
        }
    }

    /**
     * Open the Origin menu and select an item from it
     *
     * @param locator The locator of the item to select
     */
    private void selectOriginItem(By locator) {
        openOriginMenu();
        waitForElementClickable(locator).click();
    }

    /**
     * Select "Application Settings" from the Origin Menu
     *
     * @return Application Settings Page we navigated to
     */
    public AppSettings selectApplicationSettings() {
        selectOriginItem(APP_SETTINGS_LOCATOR);
        Waits.waitForPageThatMatches(chromeDriver, APP_SETTINGS_ROUTE_URL_REGEX, 30);
        AppSettings appSettings = new AppSettings(chromeDriver);
        appSettings.waitForPageToLoad();
        appSettings.waitForAngularHttpComplete();
        return appSettings;
    }

    /**
     * Select "Account Billing" from the Origin Menu
     */
    public void selectAccountBilling() {
        selectOriginItem(ACCOUNT_BILLING_LOCATOR);
    }

    /**
     * Select "Order History" from the Origin Menu
     */
    public void selectOrderHistory() {
        selectOriginItem(ORDER_HISTORY_LOCATOR);
    }

    /**
     * Select "Redeem Product Code" from the Origin menu
     */
    public void selectRedeemProductCode() {
        selectRedeemProductCode(false);
    }

    /**
     * Select "Redeem Product Code" from the Origin menu
     */
    public void selectRedeemProductCode(boolean underage) {
        if (underage) {
            selectOriginItem(UNDERAGE_REDEEM_PRODUCT_CODE_LOCATOR);
        } else {
            selectOriginItem(REDEEM_PRODUCT_CODE_LOCATOR);
        }
    }

    /**
     * Select "Refresh" from the Origin Menu with an of-age account
     */
    public void selectRefresh() {
        selectRefresh(false);
    }
    
    /**
     * Verifies 'Page Refresh' is successful by checking if element becomes stale after refresh
     * then tries to find the same element to verify window is back
     * 
     * @return true if the refresh was successful, false otherwise
     */
    public boolean verifyPageRefreshSuccessful(){
        WebElement element = chromeDriver.findElement(SEARCH_BOX_LOCATOR);
        selectRefresh();
        boolean elementStale = new WebDriverWait(chromeDriver, 10).until(ExpectedConditions.stalenessOf(element));
        boolean elementVisible = Waits.pollingWaitEx(() ->verifyElementVisible(chromeDriver, SEARCH_BOX_LOCATOR));
        return elementStale && elementVisible;
    }
    
    /**
     * Select "Refresh" from the Origin Menu
     *
     * @param underage whether an underage account is being used
     */
    public void selectRefresh(boolean underage) {
        if (underage) {
            selectOriginItem(UNDERAGE_REFRESH_LOCATOR);
        } else {
            selectOriginItem(REFRESH_LOCATOR);
        }
        //sleep because we cannot wait since the active window now is a QT window
        sleep(5000L);
    }

    /**
     * Select "Go Offline" from the Origin Menu with an of-age account
     */
    public void selectGoOffline() {
        selectGoOffline(false);
    }

    /**
     * Select "Go Offline" from the Origin Menu
     *
     * @param underage whether an underage account is being used
     */
    public void selectGoOffline(boolean underage) {
        if (underage) {
            selectOriginItem(UNDERAGE_GO_OFFLINE_ONLINE_LOCATOR);
        } else {
            selectOriginItem(GO_OFFLINE_ONLINE_LOCATOR);
        }
        //sleep because we cannot wait since the active window now is a QT window
        sleep(5000L);
    }

    /**
     * Select "Go Online" from the Origin Menu with an of-age account
     */
    public void selectGoOnline() {
        selectGoOnline(false);
    }

    /**
     * Select "Go Online" from the Origin Menu
     *
     * @param underage whether an underage account is being used
     */
    public void selectGoOnline(boolean underage) {
        _log.debug("selecting go online");
        if (underage) {
            selectOriginItem(UNDERAGE_GO_OFFLINE_ONLINE_LOCATOR);
        } else {
            selectOriginItem(GO_OFFLINE_ONLINE_LOCATOR);
        }
        _log.debug("sleep to yield cycle to go to online, cannot use wait "
                + "function because active window now is on QT window");
        sleep(5000L);
    }

    /**
     * Select "Log Out" from the Origin Menu with an of-age account
     *
     * @return Login Page we navigated to
     */
    public LoginPage selectLogOut() {
        return selectLogOut(false);
    }

    /**
     * Select "Log Out" from the Origin Menu
     *
     * @param underage whether an underage account is being used
     * @return Login Page we navigated to
     */
    public LoginPage selectLogOut(boolean underage) {
        if (underage) {
            selectOriginItem(UNDERAGE_LOGOUT_LOCATOR);
        } else {
            selectOriginItem(LOGOUT_LOCATOR);
        }
        // Waits.waitForPageThatMatches() calls driver.getWindowHandles().
        // If these are obtained before the client is actually logged out, then
        // they will become invalid once the client does successfully log out.
        // Instead, we sleep for 8 seconds to give the client a chance to logout
        // before proceeding
        sleep(8000);
        Waits.waitForWindowHandlesToStabilize(chromeDriver, 30);
        Waits.waitForPageThatMatches(chromeDriver, SIGNIN_URL_REGEX, 30);
        LoginPage loginPage = new LoginPage(chromeDriver);
        loginPage.waitForPageToLoad();
        loginPage.waitForJQueryAJAXComplete();
        return loginPage;
    }

    /**
     * Select "Exit" from the Origin Menu with an of-age account
     */
    public void selectExit() {
        selectExit(false);
    }

    /**
     * Select "Exit" from the Origin Menu
     *
     * @param underage whether an underage account is being used
     */
    public void selectExit(boolean underage) {
        if (underage) {
            selectOriginItem(UNDERAGE_EXIT_LOCATOR);
        } else {
            selectOriginItem(EXIT_LOCATOR);
        }
    }

    /**
     * Open the Games menu and select an item from it
     *
     * @param locator The locator of the item to select
     */
    private void selectGamesItem(By locator) {
        openGamesMenu();
        waitForElementClickable(locator).click();
    }

    /**
     * Select "Add Vault Game" from the Games menu (only available to
     * subscribers)
     */
    public void selectAddVaultGame() {
        selectGamesItem(SUBSCRIBER_ADD_VAULT_GAME_LOCATOR);
    }

    /**
     * Select "Add Non Origin Game" from the Games menu
     */
    public void selectAddNonOriginGame() {
        selectAddNonOriginGame(false);
    }

    /**
     * Select "Add Non Origin Game" from the Games menu for a subscriber
     */
    public void selectAddNonOriginGameSubscriber() {
        selectAddNonOriginGame(true);
    }

    /**
     * Select "Add Non Origin Game" from the Games menu
     *
     * @param subscriber if using subscriber account or not
     */
    protected void selectAddNonOriginGame(boolean subscriber) {
        if (subscriber) {
            selectGamesItem(SUBSCRIBER_ADD_NON_ORIGIN_GAME_LOCATOR);
        } else {
            selectGamesItem(ADD_NON_ORIGIN_GAME_LOCATOR);
        }
    }

    /**
     * Select "Reload My Games" from the Games menu
     *
     * @param underAge whether an underage account is being used
     * @param subscriber whether a subscriber account is being used
     *
     * @return The game library
     */
    protected GameLibrary selectReloadMyGames(boolean underAge, boolean subscriber) {
        if (underAge) {
            selectGamesItem(UNDERAGE_RELOAD_GAME_LIBRARY_LOCATOR);
        } else if (subscriber) {
            selectGamesItem(SUBSCRIBER_RELOAD_MY_GAMES_LOCATOR);
        } else {
            selectGamesItem(RELOAD_MY_GAMES_LOCATOR);
        }

        Waits.waitForPageThatMatches(chromeDriver, GAME_LIBRARY_ROUTE_URL_REGEX, 0);
        return new GameLibrary(chromeDriver);
    }

    /**
     * Select "Reload Game Library" from the Games menu
     *
     * @return Game Library Page we refreshed to
     */
    public GameLibrary selectReloadMyGames() {
        return selectReloadMyGames(false, false);
    }

    /**
     * Select "Reload My Games" from the Games menu for under aged users
     *
     * @return Game Library Page we refreshed to
     */
    public GameLibrary selectReloadMyGamesUnderAge() {
        return selectReloadMyGames(true, false);
    }

    /**
     * Select "Reload My Games" from the Games menu for subscribers
     *
     * @return Game Library Page we refreshed to
     */
    public GameLibrary selectReloadMyGamesSubscriber() {
        return selectReloadMyGames(false, true);
    }

    /**
     * Open the Friends menu and select an item from it
     *
     * @param locator The locator of the item to select
     */
    private void selectFriendsItem(By locator) {
        openFriendsMenu();
        waitForElementClickable(locator).click();
    }

    /**
     * Select "Add Friend" from the Friends menu
     */
    public void selectAddFriend() {
        selectFriendsItem(ADD_FRIEND_LOCATOR);
    }

    /**
     * Select "Set Status" from the Friends menu
     */
    public void selectSetStatus() {
        selectFriendsItem(SET_STATUS_MENU_ITEMS_LOCATOR);
    }

    /**
     * Open the Set Status menu and select an item from it
     *
     * @param locator The locator of the item to select
     */
    private void setStatus(By locator) {
        selectSetStatus();
        waitForElementClickable(locator).click();
    }

    /**
     * Set the status to Online through the Friends menu
     */
    public void selectStatusOnline() {
        setStatus(SET_STATUS_ONLINE_LOCATOR);
    }

    /**
     * Set the status to Away through the Friends menu
     */
    public void selectStatusAway() {
        setStatus(SET_STATUS_AWAY_LOCATOR);
    }

    /**
     * Set the status to Invisible through the Friends menu
     */
    public void selectStatusInvisible() {
        setStatus(SET_STATUS_INVISIBLE_LOCATOR);
    }

    /**
     * Open the View menu and select an item from it
     *
     * @param locator The locator of the item to select
     * @param underage Whether we are in an underage user or not
     */
    private void selectViewItem(By locator, boolean underage) {
        openViewMenu(underage);
        waitForElementClickable(locator).click();
    }

    /**
     * Select Discover from the View menu
     */
    public void selectDiscover() {
        selectViewItem(VIEW_DISCOVER_LOCATOR, false);
    }

    /**
     * Select Store from the View menu
     */
    public void selectStore() {
        selectViewItem(VIEW_STORE_LOCATOR, false);
    }

    /**
     * Select Game Library from the View menu with an of-age account
     */
    public void selectGameLibrary() {
        selectGameLibrary(false);
    }

    /**
     * Select Game Library from the View menu
     *
     * @param underage Whether the account is underage or not
     */
    public void selectGameLibrary(boolean underage) {
        if (underage) {
            selectViewItem(UNDERAGE_VIEW_GAME_LIBRARY_LOCATOR, underage);
        } else {
            selectViewItem(VIEW_GAME_LIBRARY_LOCATOR, underage);
        }
    }

    /**
     * Select Download Queue from the View menu with an of-age account
     */
    public void selectDownloadQueue() {
        selectDownloadQueue(false);
    }

    /**
     * Select Download Queue from the View menu
     *
     * @param underage Whether the account is underage or not
     */
    public void selectDownloadQueue(boolean underage) {
        if (underage) {
            selectViewItem(VIEW_DOWNLOAD_QUEUE_LOCATOR, underage);
        } else {
            selectViewItem(VIEW_DOWNLOAD_QUEUE_LOCATOR, underage);
        }
    }

    /**
     * Select Friends List from the View menu
     */
    public void selectFriendsList() {
        selectViewItem(VIEW_FRIENDS_LIST_LOCATOR, false);
    }

    /**
     * Select Active Chat from the View menu
     */
    public void selectActiveChat() {
        selectViewItem(VIEW_ACTIVE_CHAT_LOCATOR, false);
    }

    /**
     * Select My Profile from the View menu
     */
    public void selectMyProfile() {
        selectViewItem(VIEW_MY_PROFILE_LOCATOR, false);
    }

    /**
     * Select Achievements from the View menu
     */
    public void selectAchievements() {
        selectViewItem(VIEW_ACHIEVEMENTS_LOCATOR, false);
    }

    /**
     * Select Wishlist from the View menu
     */
    public void selectWishlist() {
        selectViewItem(VIEW_WISHLIST_LOCATOR, false);
    }

    /**
     * Select Search from the View menu with an of-age account
     */
    public void selectSearch() {
        selectSearch(false);
    }

    /**
     * Select Search from the View menu
     *
     * @param underage Whether the account is underage or not
     */
    public void selectSearch(boolean underage) {
        if (underage) {
            selectViewItem(VIEW_SEARCH_LOCATOR, underage);
        } else {
            selectViewItem(UNDERAGE_VIEW_SEARCH_LOCATOR, underage);
        }
    }

    /**
     * Open the Help menu and select an item from it
     *
     * @param locator The locator of the item to select
     * @param underage Whether we are in an underage user or not
     */
    private void selectHelpItem(By locator, boolean underage) {
        openHelpMenu(underage);
        waitForElementClickable(locator).click();
    }

    /**
     * Select "Origin Help" from the Help menu
     */
    public void selectOriginHelp() {
        selectHelpItem(ORIGIN_HELP_LOCATOR, false);
    }

    /**
     * Select "Origin Forum" from the Help menu
     */
    public void selectOriginForum() {
        selectHelpItem(ORIGIN_FORUM_LOCATOR, false);
    }

    /**
     * Select "Origin Error Reporter" from the Help Menu with an of-age account
     */
    public void selectOriginErrorReporter() {
        selectOriginErrorReporter(false);
    }

    /**
     * Select "Origin Error Reporter" from the Help menu
     *
     * @param underage whether an underage account is being used
     */
    public void selectOriginErrorReporter(boolean underage) {
        if (underage) {
            selectHelpItem(UNDERAGE_ORIGIN_ERROR_REPORTER_LOCATOR, underage);
        } else {
            selectHelpItem(ORIGIN_ERROR_REPORTER_LOCATOR, underage);
        }
    }

    /**
     * Select "Shortcuts" from the Help menu
     *
     * @param underage whether an underage account is being used
     */
    public void selectShortcuts(boolean underage) {
        if (underage) {
            selectHelpItem(UNDERAGE_SHORTCUTS_LOCATOR, underage);
        } else {
            selectHelpItem(SHORTCUTS_LOCATOR, underage);
        }
    }

    /**
     * Select "About" from the Help Menu with an of-age account
     *
     * @return About Dialog we opened
     */
    public AboutDialog selectAbout() {
        return selectAbout(false);
    }

    /**
     * Select "About" from the Help menu
     *
     * @param underage whether an underage account is being used
     * @return About Dialog we opened
     */
    public AboutDialog selectAbout(boolean underage) {
        if (underage) {
            selectHelpItem(UNDERAGE_ABOUT_LOCATOR, underage);
        } else {
            selectHelpItem(ABOUT_LOCATOR, underage);
        }
        AboutDialog aboutDialog = new AboutDialog(driver);
        return aboutDialog;
    }

    /**
     * Verify an item exists, or is enabled in the given menu list
     *
     * @param menu The menu that we want to look in
     * @param itemText The text we are looking for in the menu
     * @param enabled if true, we check for the item to be enabled; if false we
     * check if the item exists
     * @return true if the text exists in the menu selections, false otherwise
     */
    private boolean verifyItemExistsEnabledInMenu(By menu, String itemText, boolean enabled) {
        switchToOriginWindow();
        List<WebElement> menuItems = driver.findElements(menu);

        WebElement foundItem = null;
        for (WebElement item : menuItems) {
            //Workaround as the item text occasionally contains an erroneous
            //'&' character. It doesn't appear to show up to when looking at
            //Origin manually, but Selenium sees it. This workaround could be
            //problematic for any menu items that are supposed to contain
            //an '&' character, but luckily none of them currently do.
            if (item.getText().replace("&", "").trim()
                    .equalsIgnoreCase(itemText.replace("&", "").trim())) {
                foundItem = item;
            }
        }
        if (enabled) {
            return foundItem != null && foundItem.isEnabled();
        }
        return foundItem != null;
    }

    /**
     * Verify an item exists in the Origin menu.
     *
     * @param itemText The text of the item we are looking for
     * @return True if the item exists, false otherwise
     */
    public boolean verifyItemExistsInOrigin(String itemText) {
        return verifyItemExistsEnabledInMenu(ORIGIN_MENU_ITEMS_LOCATOR, itemText, false);
    }

    /**
     * Verify an item exists in the Games menu.
     *
     * @param itemText The text of the item we are looking for
     * @return True if the item exists, false otherwise
     */
    public boolean verifyItemExistsInGames(String itemText) {
        return verifyItemExistsEnabledInMenu(GAMES_MENU_ITEMS_LOCATOR, itemText, false);
    }

    /**
     * Verify an item exists in the Friends menu.
     *
     * @param itemText The text of the item we are looking for
     * @return True if the item exists, false otherwise
     */
    public boolean verifyItemExistsInFriends(String itemText) {
        return verifyItemExistsEnabledInMenu(FRIENDS_MENU_ITEMS_LOCATOR, itemText, false);
    }

    /**
     * Verify an item exists in the View menu.
     *
     * @param itemText The text of the item we are looking for
     * @return True if the item exists, false otherwise
     */
    public boolean verifyItemExistsInView(String itemText) {
        return verifyItemExistsEnabledInMenu(VIEW_MENU_ITEMS_LOCATOR, itemText, false);
    }

    /**
     * Verify an item exists in the Help menu.
     *
     * @param itemText The text of the item we are looking for
     * @return True if the item exists, false otherwise
     */
    public boolean verifyItemExistsInHelp(String itemText) {
        return verifyItemExistsEnabledInMenu(HELP_MENU_ITEMS_LOCATOR, itemText, false);
    }

    /**
     * Verify an item is enabled in the Origin menu.
     *
     * @param itemText The text of the item we are looking for
     * @return True if the item exists, false otherwise
     */
    public boolean verifyItemEnabledInOrigin(String itemText) {
        return verifyItemExistsEnabledInMenu(ORIGIN_MENU_ITEMS_LOCATOR, itemText, true);
    }

    /**
     * Verify an item is enabled in the Games menu.
     *
     * @param itemText The text of the item we are looking for
     * @return True if the item exists, false otherwise
     */
    public boolean verifyItemEnabledInGames(String itemText) {
        return verifyItemExistsEnabledInMenu(GAMES_MENU_ITEMS_LOCATOR, itemText, true);
    }

    /**
     * Verify an item is enabled in the Friends menu.
     *
     * @param itemText The text of the item we are looking for
     * @return True if the item exists, false otherwise
     */
    public boolean verifyItemEnabledInFriends(String itemText) {
        return verifyItemExistsEnabledInMenu(FRIENDS_MENU_ITEMS_LOCATOR, itemText, true);
    }

    /**
     * Verify an item is enabled in the View menu.
     *
     * @param itemText The text of the item we are looking for
     * @return True if the item is enabled, false otherwise
     */
    public boolean verifyItemEnabledInView(String itemText) {
        return verifyItemExistsEnabledInMenu(VIEW_MENU_ITEMS_LOCATOR, itemText, true);
    }

    /**
     * Verify an item is enabled in the Help menu.
     *
     * @param itemText The text of the item we are looking for
     * @return True if the item exists, false otherwise
     */
    public boolean verifyItemEnabledInHelp(String itemText) {
        return verifyItemExistsEnabledInMenu(HELP_MENU_ITEMS_LOCATOR, itemText, true);
    }

    /**
     * Verifies the given menu exists in the main menu bar
     *
     * @param menuText The text of the menu we are looking for
     * @return True if the menu exists, false otherwise
     */
    public boolean verifyMenuExists(String menuText) {
        switchToOriginWindow();
        List<WebElement> menus = driver.findElements(MAIN_MENUS_LOCATOR);
        return getElementWithText(menus, menuText) != null;
    }

    /**
     * Verify 'Offline Mode' button text matches the expected text
     *
     * @return true if button text matches, false otherwise
     */
    public boolean verifyOfflineModeButtonText() {
        WebElement offlineModeButton;
        switchToOriginWindow();
        try {
            offlineModeButton = waitForElementVisible(OFFLINE_MODE_BUTTON_LOCATOR, 10);
        } catch (Exception e) {
            _log.warn(String.format("Exception: Cannot locate 'Offline Mode' button text message - %s", e.getMessage()));
            return false;
        }
        return offlineModeButton.getText().equals(OFFLINE_MODE_BUTTON_TEXT); // check exact match
    }

    /**
     * Verify 'Offline Mode' button is visible
     *
     * @return true if button is visible, false otherwise
     */
    public boolean verifyOfflineModeButtonVisible() {
        switchToOriginWindow();
        return waitIsElementVisible(OFFLINE_MODE_BUTTON_LOCATOR);
    }

    /**
     * Verify 'Offline Mode' button is visible
     *
     * @param sec seconds to wait
     * @return true if button is visible, false otherwise
     */
    public boolean verifyOfflineModeButtonVisible(int sec) {
        switchToOriginWindow();
        return waitIsElementVisible(OFFLINE_MODE_BUTTON_LOCATOR, sec);
    }

    /**
     * Click the 'Offline Mode' button
     */
    public void clickOfflineModeButton() {
        switchToOriginWindow();
        waitForElementClickable(OFFLINE_MODE_BUTTON_LOCATOR).click();
    }

    /**
     * Select submenu from debug menu
     *
     * @param locator locator for submenu
     */
    private void selectDebugItem(By locator) {
        openDebugMenu();
        waitForElementClickable(locator).click();
    }

    /**
     * Open debug menu
     */
    public void openDebugMenu() {
        openMenu(DEBUG_MENU_LOCATOR);
    }

    /**
     * Select Dirty Bits connect sample handler from debug menu
     */
    public void selectDirtyBitsConnectSampleHandlersMenu(){
        selectDebugItem(DIRTY_BITS_CONNECT_SAMPLE_HANDLERS_LOCATOR);
    }

    /**
     * Verifies that the client window's maximize button is visible
     */
    public boolean verifyMaximizeButtonVisible() {
        return waitIsElementVisible(MAXIMIZE_WINDOW_BUTTON_LOCATOR, 5);
    }

    /**
     * Verifies that the client window's restore button is visible
     */
    public boolean verifyRestoreButtonVisible() {
        return waitIsElementVisible(RESTORE_WINDOW_BUTTON_LOCATOR, 5);
    }
}