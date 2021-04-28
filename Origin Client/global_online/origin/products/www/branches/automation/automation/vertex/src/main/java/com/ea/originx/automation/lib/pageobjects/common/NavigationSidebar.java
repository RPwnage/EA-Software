package com.ea.originx.automation.lib.pageobjects.common;

import com.ea.originx.automation.lib.helpers.I18NUtil;
import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
import com.ea.originx.automation.lib.pageobjects.store.AboutStorePage;
import com.ea.originx.automation.lib.pageobjects.store.BrowseGamesPage;
import com.ea.originx.automation.lib.pageobjects.store.DealsPage;
import com.ea.originx.automation.lib.pageobjects.store.DemosAndBetasPage;
import com.ea.originx.automation.lib.pageobjects.store.DownloadOriginPage;
import com.ea.originx.automation.lib.pageobjects.store.OriginAccessFaqPage;
import com.ea.originx.automation.lib.pageobjects.store.OriginAccessTrialPage;
import com.ea.originx.automation.lib.pageobjects.store.StorePage;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * The navigation bar that is an always visible frame on the left side.
 *
 * @author glivingstone
 */
public class NavigationSidebar extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(NavigationSidebar.class);

    // URL regexes for page navigation
    protected static final String HOME_ROUTE_URL_REGEX = ".*(home|my-home).*";
    protected static final String STORE_MAIN_ROUTE_URL_REGEX = ".*store.*";
    protected static final String BROWSE_GAMES_ROUTE_URL_REGEX = ".*store/browse.*";
    protected static final String FREE_GAMES_ROUTE_URL_REGEX = ".*store/.*games.*";
    protected static final String GAME_LIBRARY_ROUTE_URL_REGEX = ".*game-library.*";
    protected static final String DEALS_ROUTE_URL_REGEX = ".*store/deals.*";
    protected static final String ORIGIN_ACCESS_ROUTE_URL_REGEX = ".*store/origin-access.*";

    protected static final String ON_THE_HOUSE_ROUTE_URL_REGEX = ".*on-the-house.*";
    protected static final String FREE_GAME_TRIALS_ROUTE_URL_REGEX = ".*trials.*";
    protected static final String DEMOS_BETAS_ROUTE_URL_REGEX = ".*demos-and-betas.*";

    protected static final String VAULT_GAMES_ROUTE_URL_REGEX = ".*origin-access/vault-games.*";
    protected static final String ORIGIN_ACCESS_TRIAL_URL_REGEX = ".*origin-access/trials.*";
    protected static final String ORIGIN_ACCESS_FAQ_ROUTE_URL_REGEX = ".*origin-access/faq.*";

    protected static final String DOWNLOADS_ROUTE_URL_REGEX = ".*store/download.*";
    protected static final String ABOUT_ROUTE_URL_REGEX = ".*store/about.*";

    protected static final By ORIGIN_NAV_SIDEBAR_CONTAINER_LOCATOR = By.cssSelector("nav.origin-navigation");
    protected static final By ORIGIN_NAV_SIDEBAR_HEADER = By.cssSelector("nav[ng-class=\"{'origin-navigation-isvisible': states.mainMenuVisible}\"]");

    protected static final String ORIGIN_NAV_CSS = ".origin-nav.origin-main-nav-stacked";
    protected static final String STORE_SUBMENU_CSS = ORIGIN_NAV_CSS + " > li[href='store'] > ul";

    protected static final By GLOBAL_BACK_BUTTON_LOCATOR = By.cssSelector(".origin-browsebtn-back");
    protected static final By GLOBAL_FORWARD_BUTTON_LOCATOR = By.cssSelector(".origin-browsebtn-forwards");

    protected static final By DISCOVER_BUTTON_LOCATOR = By.cssSelector(ORIGIN_NAV_CSS + " > li[section='homeloggedin']");
    protected static final By STORE_BUTTON_LOCATOR = By.cssSelector(ORIGIN_NAV_CSS + " > li[section='store'] > a[ng-href*='store']");
    protected static final By ORIGIN_ACCESS_NAV_LOCATOR = By.cssSelector(ORIGIN_NAV_CSS + " > li[section='originaccess'] > a[ng-href*='store/origin-access']");
    protected static final By BROWSE_GAMES_LOCATOR = By.cssSelector(STORE_SUBMENU_CSS + " > li[section='browse'] > a[ng-href*='store/browse']");
    protected static final By FREE_GAMES_LOCATOR = By.cssSelector(STORE_SUBMENU_CSS + " > li[section='freegames'] > a[ng-href*='store/free-games']");
    protected static final By DEALS_EXCLUSIVES_LOCATOR = By.cssSelector(STORE_SUBMENU_CSS + " > li[section='dealcenter'] > a[ng-href*='store/deals']");
    protected static final By GAME_LIBRARY_LOCATOR = By.cssSelector(ORIGIN_NAV_CSS + " > li[section='gamelibrary'] a");
    protected static final By NOTIFICATIONS_LOCATOR = By.cssSelector("#origin-content .origin-navigation-tertiary .origin-inbox-mini-header");

    protected static final String FREE_GAMES_SUBMENU_ITEM_CSS_TMPL = STORE_SUBMENU_CSS + " > li[section='freegames'] > div > ul > li[title-str='%s'] > a";
    protected static final By FREE_TRIAL_LOCATOR = By.cssSelector(String.format(FREE_GAMES_SUBMENU_ITEM_CSS_TMPL, "Trials"));
    protected static final By DEMOAS_BETAS_LOCATOR = By.cssSelector(String.format(FREE_GAMES_SUBMENU_ITEM_CSS_TMPL, "Demos and Betas"));

    protected static final String ORIGIN_ACCESS_SUBMENU_ITEM_CSS_TMPL = ORIGIN_NAV_CSS + " > li[section='originaccess'] > div > ul > li[title-str='%s'] > a";
    protected static final By VAULT_GAMES_LOCATOR = By.cssSelector(String.format(ORIGIN_ACCESS_SUBMENU_ITEM_CSS_TMPL, "Vault Games"));
    protected static final By MANAGE_MY_MEMBERSHIP_LOCATOR = By.cssSelector(String.format(ORIGIN_ACCESS_SUBMENU_ITEM_CSS_TMPL, "Manage My Membership"));
    protected static final By ORIGIN_ACCESS_TRIAL_LOCATOR = By.cssSelector(String.format(ORIGIN_ACCESS_SUBMENU_ITEM_CSS_TMPL, "Play First Trials"));
    protected static final By ORIGIN_ACCESS_FAQ_LOCATOR = By.cssSelector(String.format(ORIGIN_ACCESS_SUBMENU_ITEM_CSS_TMPL, "Origin Access FAQ"));
    protected static final By LEARN_ABOUT_PREMIER_LOCATOR = By.cssSelector(String.format(ORIGIN_ACCESS_SUBMENU_ITEM_CSS_TMPL, "Learn About Premier"));
    
    protected static final By NOTIFICATIONS_GIFT_LOCATOR = By.cssSelector("#origin-content .origin-navigation-tertiary .origin-inbox-flyout .origin-inbox-list-item [title-str='YOU\\'VE GOT A GIFT']");

    protected static final By ABOUT_LOCATOR = By.cssSelector(".origin-navigation-tertiary .origin-nav > li[title-str='About'] > a");
    protected static final By DOWNLOAD_ORIGIN_LOCATOR = By.cssSelector(".origin-navigation-tertiary .origin-nav > li[title-str='Download'] > a");

    protected static final By BLOG_LOCATOR = By.cssSelector(ORIGIN_NAV_CSS + " > li[section='blog'] a");
    protected static final By HELP_LOCATOR = By.cssSelector(ORIGIN_NAV_CSS + " > li[section='help'] a");

    protected static final String ORIGIN_LOGGEDOUT_BOTTOM_CSS = ".origin-navigation-bottom .origin-footernav-loggedout";
    protected static final String ORIGIN_LOGGEDOUT_LANGAUGE_PREFERENCES_ICON_CSS = ORIGIN_LOGGEDOUT_BOTTOM_CSS + " origin-cta-selectlanguage origin-icon[icon='language']";

    protected static final By ORIGIN_LOGGEDOUT_LANGAUGE_PREFERENCES_ICON_LOCATOR = By.cssSelector(ORIGIN_LOGGEDOUT_LANGAUGE_PREFERENCES_ICON_CSS);

    // Mobile
    protected static final By HAMBURGER_MENU = By.cssSelector("div.origin-navigation-top > div.l-origin-hamburger[ng-click='toggleMainMenu($event)']");

    /**
     * Constructor
     *
     * @param driver
     */
    public NavigationSidebar(WebDriver driver) {
        super(driver);
    }

    ///////////////////////////////////////////////////////////////////////
    // Basic functions for nav bar interaction
    ///////////////////////////////////////////////////////////////////////
    /**
     * Scrolls to then clicks on an element in the sidebar
     *
     * @param element The element to scroll to and click
     */
    private void clickSidebarElement(WebElement element) {
        openSidebar();
        scrollSidebarToElement(element);
        element.click();
    }

    /**
     * Clicks on the Forward button in the navigation sidebar.
     */
    public void clickForward() {
        _log.debug("clicking forward");
        clickSidebarElement(waitForElementClickable(GLOBAL_FORWARD_BUTTON_LOCATOR));
    }

    /**
     * Clicks on the Back button in the navigation sidebar.
     */
    public void clickBack() {
        _log.debug("clicking back");
        clickSidebarElement(waitForElementClickable(GLOBAL_BACK_BUTTON_LOCATOR));
    }

    /**
     * Clicks on the open 'Discover' item in the navigation sidebar.
     */
    public void clickDiscoverLink() {
        _log.debug("clicking myhome link");
        clickSidebarElement(waitForElementClickable(DISCOVER_BUTTON_LOCATOR));
    }

    /**
     * Clicks on the open 'Store' item in the navigation sidebar.
     */
    public void clickStoreLink() {
        _log.debug("clicking store link");
        clickSidebarElement(waitForElementClickable(STORE_BUTTON_LOCATOR));
    }

    /**
     * Click 'Browse Games' at the navigation sidebar
     */
    public void clickBrowseGamesLink() {
        _log.debug("clicking browse games link");
        clickSidebarElement(waitForElementClickable(BROWSE_GAMES_LOCATOR));
    }

    /**
     * Click 'Deals & Exclusives' at the navigation sidebar
     */
    public void clickDealsExclusivesLink() {
        _log.debug("clicking deals & exclusives link");
        clickSidebarElement(waitForElementClickable(DEALS_EXCLUSIVES_LOCATOR));
        hoverOnStoreButton(); // close the submenu popup
    }

    /**
     * Click 'Origin Access' at the navigation sidebar
     */
    public void clickOriginAccessLink() {
        _log.debug("clicking origin access link");
        clickSidebarElement(waitForElementClickable(ORIGIN_ACCESS_NAV_LOCATOR));
    }

    /**
     * Click 'Game Library' at the navigation sidebar
     */
    public void clickGameLibraryLink() {
        _log.debug("clicking game library link");
        clickSidebarElement(waitForElementClickable(GAME_LIBRARY_LOCATOR));
    }

    /**
     * Click 'About' at the navigation sidebar
     */
    public void clickAboutLink() {
        _log.debug("clicking about link");
        clickSidebarElement(waitForElementClickable(ABOUT_LOCATOR));
    }

    /**
     * Click 'Download' at the navigation side bar
     */
    public void clickDownloadOriginLink() {
        _log.debug("clicking download link");
        clickSidebarElement(waitForElementClickable(DOWNLOAD_ORIGIN_LOCATOR));
    }

    /**
     * Click 'Blog' link at the navigation sidebar
     */
    public void clickBlogLink() {
        _log.debug("clicking blog link");
        clickSidebarElement(waitForElementClickable(BLOG_LOCATOR));
    }

    /**
     * Click 'Help' link at the navigation sidebar
     */
    public void clickHelpLink() {
        _log.debug("clicking help link");
        clickSidebarElement(waitForElementClickable(HELP_LOCATOR));

    }

    /**
     * Click on the given item in the Free Games submenu. We do not open the
     * submenu first because of an issue with hovering in browsers.
     *
     * @param subMenuItemLocator The locator of the item to click on
     */
    private void clickFreeGamesSubMenuLink(By subMenuItemLocator) {
        hoverOnFreeGames();
        forceClickElement(waitForElementPresent(subMenuItemLocator));
    }

    /**
     * Click the 'Free Games'->'Trials' Link
     */
    public void clickFreeGamesTrialLink() {
        clickFreeGamesSubMenuLink(FREE_TRIAL_LOCATOR);
    }

    /**
     * Click 'Demos and Betas'
     */
    public void clickDemosBetasLink() {
        clickFreeGamesSubMenuLink(DEMOAS_BETAS_LOCATOR);
    }

    /**
     * Click 'You Got a Gift' from Notification sub-menu
     */
    public void clickYouGotGiftNotification() {
        hoverOnNotifications();
        waitForElementVisible(NOTIFICATIONS_GIFT_LOCATOR).click();
    }

    ///////////////////////////////////////////////////////////////////////
    // 'Navigation' pane Pages and Actions
    ///////////////////////////////////////////////////////////////////////
    /**
     * Scrolls to then hovers over an element in the sidebar
     *
     * @param element The element to scroll to and hover over
     */
    public void hoverOnSidebarElement(WebElement element) {
        scrollSidebarToElement(element);
        hoverOnElement(element);
    }

    /**
     * Hover on the Store button in the side nav<br>
     * NOTE: Use this to close the sub-menu popup after clicking/hovering on
     * links with sub-menus. This includes: <br>
     * - Browse Games<br>
     * - Free Games<br>
     * - Deals & Exclusives<br>
     * - Origin Access<br>
     */
    public void hoverOnStoreButton() {
        _log.debug("hovering on store");
        hoverOnSidebarElement(waitForElementClickable(STORE_BUTTON_LOCATOR));
    }

    /**
     * Hover on the 'Browse Games' link in the side nav
     */
    public void hoverOnBrowseGames() {
        _log.debug("hovering on browse games");
        hoverOnSidebarElement(waitForElementClickable(BROWSE_GAMES_LOCATOR));
    }

    /**
     * Hover on the 'Origin Access' link in the side nav
     */
    public void hoverOnOriginAccess() {
        _log.debug("hovering on origin access");
        hoverOnSidebarElement(waitForElementClickable(ORIGIN_ACCESS_NAV_LOCATOR)); 
    }

    /**
     * Hover on the 'Free Games' link in the side nav
     */
    public void hoverOnFreeGames() {
        _log.debug("hovering on free games");
        hoverOnSidebarElement(waitForElementClickable(FREE_GAMES_LOCATOR));
    }

    /**
     * Hover on the 'Notifications' link in the side nav
     */
    public void hoverOnNotifications() {
        _log.debug("hovering on free games");
        hoverOnSidebarElement(waitForElementClickable(NOTIFICATIONS_LOCATOR));
    }

    /**
     * Go to Download Origin page
     *
     * @return {@link DownloadOriginPage} A page object that represents the
     * Download Origin page
     */
    public DownloadOriginPage gotoDownloadOriginPage() {
        _log.debug("goto DownloadOriginPage");
        clickDownloadOriginLink();
        waitForPageThatMatches(DOWNLOADS_ROUTE_URL_REGEX, 10);
        DownloadOriginPage downloadPage = new DownloadOriginPage(driver);
        downloadPage.waitForPageToLoad();
        downloadPage.waitForAngularHttpComplete();
        _log.debug("DownloadOriginPage loaded");
        return downloadPage;

    }

    /**
     * Go to 'About' Store page
     *
     * @return {@link AboutStorePage} A page object that represents the About
     * Store Page
     */
    public AboutStorePage gotoAboutStorePage() {
        _log.debug("goto About storepage");
        clickAboutLink();
        waitForPageThatMatches(ABOUT_ROUTE_URL_REGEX, 10);
        AboutStorePage aboutStorePage = new AboutStorePage(driver);
        aboutStorePage.waitForPageToLoad();
        aboutStorePage.waitForAngularHttpComplete();
        _log.debug("About storepage loaded");
        return aboutStorePage;
    }

    /**
     * Go to the 'My Home' page
     *
     * @return {@link DiscoverPage} A page object that represents the myhome
     * (home) page
     */
    public DiscoverPage gotoDiscoverPage() {
        _log.debug("goto myhome page");
        clickDiscoverLink();
        waitForPageThatMatches(HOME_ROUTE_URL_REGEX);
        final DiscoverPage discoverPage = nextPage(DiscoverPage.class);
        discoverPage.waitForPageToLoad();
        discoverPage.waitForAngularHttpComplete();
        _log.debug("myhome page loaded");
        return discoverPage;
    }

    /**
     * Go to the 'Browse Games' page
     *
     * @return {@link BrowseGamesPage} A page object that represents the stores
     * browse games page
     */
    public BrowseGamesPage gotoBrowseGamesPage() {
        _log.debug("goto browse games page");
        clickBrowseGamesLink();
        waitForPageThatMatches(BROWSE_GAMES_ROUTE_URL_REGEX);
        final BrowseGamesPage browsePage = nextPage(BrowseGamesPage.class);
        browsePage.waitForPageToLoad();
        browsePage.waitForAngularHttpComplete();
        _log.debug("browse page loaded");
        hoverOnStoreButton(); // close the submenu popup
        browsePage.waitForGamesToLoad();
        return browsePage;
    }

    /**
     * Go to the 'Game Library' page
     *
     * @return {@link GameLibrary} A page object that represents the game
     * library page
     */
    public GameLibrary gotoGameLibrary() {
        _log.debug("goto game library");
        clickGameLibraryLink();
        waitForPageThatMatches(GAME_LIBRARY_ROUTE_URL_REGEX);
        final GameLibrary gameLibrary = nextPage(GameLibrary.class);
        gameLibrary.waitForPageToLoad();
        gameLibrary.waitForAngularHttpComplete();
        _log.debug("game library loaded");
        return gameLibrary;
    }

    /**
     * Go to the 'Game Library' page
     *
     * @return {@link GameLibrary} A page object that represents the game
     * library page
     */
    public StorePage gotoStorePage() {
        _log.debug("goto store page");
        clickStoreLink();
        waitForPageThatMatches(STORE_MAIN_ROUTE_URL_REGEX, 12);
        final StorePage storePage = nextPage(StorePage.class);
        storePage.waitForPageToLoad();
        storePage.waitForAngularHttpComplete();
        _log.debug("store page loaded");
        return storePage;
    }

    /**
     * Go to 'Deals' Page
     *
     * @return A page object that represents the deals page
     */
    public DealsPage gotoDealsPage() {
        _log.debug("goto deals page");
        clickDealsExclusivesLink();
        waitForPageThatMatches(DEALS_ROUTE_URL_REGEX, 12);
        final DealsPage dealsPage = nextPage(DealsPage.class);
        dealsPage.waitForPageToLoad();
        dealsPage.waitForAngularHttpComplete();
        _log.debug("deals page loaded");
        return dealsPage;

    }

    /**
     * Go to 'Origin Access' page
     *
     * @return A page object that represents the origin access page
     */
    public OriginAccessPage gotoOriginAccessPage() {
        _log.debug("goto origin access page");
        clickOriginAccessLink();
        waitForPageThatMatches(ORIGIN_ACCESS_ROUTE_URL_REGEX, 12);
        final OriginAccessPage originAccessPage = nextPage(OriginAccessPage.class);
        originAccessPage.waitForPageToLoad();
        originAccessPage.waitForAngularHttpComplete();
        _log.debug("origin access page loaded");
        hoverOnStoreButton(); // close the submenu popup
        return originAccessPage;

    }

    /**
     * Go to 'Demos and Betas' page
     *
     * @return A page object that represents the Demos and Betas page
     */
    public DemosAndBetasPage gotoDemosBetasPage() {
        _log.debug("goto Demos and Betas page");
        clickDemosBetasLink();
        waitForPageThatMatches(DEMOS_BETAS_ROUTE_URL_REGEX, 12);
        DemosAndBetasPage demoPage = new DemosAndBetasPage(driver);
        demoPage.waitForPageToLoad();
        demoPage.waitForAngularHttpComplete();
        _log.debug("Demos and Betas page loaded");
        return demoPage;
    }

    /**
     * Go to 'Origin Access' Faq page
     *
     * @return A page object that represents the Origin Access Faq page
     */
    public OriginAccessFaqPage gotoOriginAccessFaqPage() {
        _log.debug("goto origin access Faq page");
        clickOriginAccessFaqLink();
        waitForPageThatMatches(ORIGIN_ACCESS_FAQ_ROUTE_URL_REGEX, 12);
        OriginAccessFaqPage originAccessFaqPage = new OriginAccessFaqPage(driver);
        originAccessFaqPage.waitForPageToLoad();
        originAccessFaqPage.waitForAngularHttpComplete();
        _log.debug("origin access Faq page loaded");
        return originAccessFaqPage;
    }

    /**
     * Click on the 'Origin Access Faq' Link
     */
    public void clickOriginAccessFaqLink() {
        clickOriginAccessSubMenuLinks(ORIGIN_ACCESS_FAQ_LOCATOR);
    }
    
    /**
     * Click on the 'Learn About Premier' Link
     */
    public void clickLearnAboutPremierLink() {
        clickOriginAccessSubMenuLinks(LEARN_ABOUT_PREMIER_LOCATOR);
    }

    /**
     * Go to 'Origin Access trial' page
     *
     * @return A page object that represents the Origin Access trial page
     */
    public OriginAccessTrialPage gotoOriginAccessTrialPage() {
        _log.debug("goto origin access trials page");
        clickOriginAccessTrialLink();
        waitForPageThatMatches(ORIGIN_ACCESS_ROUTE_URL_REGEX, 12);
        sleep(4000);
        OriginAccessTrialPage originAccessTrialsPage = new OriginAccessTrialPage(driver);
        originAccessTrialsPage.waitForPageToLoad();
        originAccessTrialsPage.waitForAngularHttpComplete();
        _log.debug("origin access trials page loaded");
        return originAccessTrialsPage;
    }

    /**
     * Click the 'Origin Access Trial' Link
     */
    public void clickOriginAccessTrialLink() {
        clickOriginAccessSubMenuLinks(ORIGIN_ACCESS_TRIAL_LOCATOR);
    }

    /**
     * Go to 'Vault Games' page
     *
     * @return A page object that represents the Vault Games page
     */
    public VaultPage gotoVaultGamesPage() {
        _log.debug("goto vault games page");
        clickVaultGamesLink();
        waitForPageThatMatches(VAULT_GAMES_ROUTE_URL_REGEX, 12);
        VaultPage vaultPage = new VaultPage(driver);
        vaultPage.waitForPageToLoad();
        vaultPage.waitForAngularHttpComplete();
        _log.debug("vault page loaded");
        return vaultPage;
    }

    /**
     * Click the 'Vault Games' link
     */
    public void clickVaultGamesLink() {
        clickOriginAccessSubMenuLinks(VAULT_GAMES_LOCATOR);
    }

    /**
     * Click the 'Manage My Membership' link
     */
    public void clickManageMyMembershipLink() {
        clickOriginAccessSubMenuLinks(MANAGE_MY_MEMBERSHIP_LOCATOR);
    }
    
    /**
     * Click on the given item in the Origin Access submenu. We do not open the
     * submenu first because of an issue with hovering in browsers.
     *
     * @param subMenuItemLocator The locator of the item to click on
     */
    private void clickOriginAccessSubMenuLinks(By subMenuItemLocator) {
        hoverOnOriginAccess();
        forceClickElement(waitForElementPresent(subMenuItemLocator));
    }

    /**
     * Verify the 'Deals & Exclusives' link is currently selected in the sidebar
     *
     * @return True if 'Deals & Exclusives' is selected, false otherwise
     */
    public boolean verifyDealsExclusivesActive() {
        return driver.findElement(DEALS_EXCLUSIVES_LOCATOR).getAttribute("class").contains("active");
    }

    /**
     * Verify the 'Origin Access' link is currently selected in the sidebar
     *
     * @return True if 'Origin Access' is selected, false otherwise
     */
    public boolean verifyOriginAccessActive() {
        return driver.findElement(ORIGIN_ACCESS_NAV_LOCATOR).getAttribute("class").contains("active");
    }

    /**
     * Verify that the 'Store' link is visible
     *
     * @return True if the 'Store' link is visible, false otherwise
     */
    public boolean verifyStoreLinkVisible() {
        return waitIsElementVisible(STORE_BUTTON_LOCATOR);
    }

    /**
     * Verify that the 'My Home' link is visible
     *
     * @return True if the 'My Home' link is visible, false otherwise
     */
    public boolean verifyDiscoverLinkVisible() {
        return waitIsElementVisible(DISCOVER_BUTTON_LOCATOR);
    }

    /**
     * Verify that 'Notifications' have a pending gift
     *
     * @return true if there is a pending gift notification, false otherwise
     */
    public boolean verifyGiftReceived() {
        hoverOnNotifications();
        return waitIsElementPresent(NOTIFICATIONS_GIFT_LOCATOR);
    }

    /**
     * Scroll the sidebar until the target element is at the top of the sidebar
     * or until the sidebar is scrolled as far as it can scroll. Will close the
     * site stripe if it is visible, because the sitestripe can cause elements
     * to be scrolled into unclickable positions
     *
     * @param target The element within the sidebar, which should be scrolled to
     * the top of the sidebar
     */
    private void scrollSidebarToElement(WebElement target) {
        //The nav container selector matches more than one element,
        //so this will find the first one that is visible
        WebElement container = driver.findElements(ORIGIN_NAV_SIDEBAR_CONTAINER_LOCATOR).
                stream().
                filter(element -> element.isDisplayed()).
                findFirst().
                get();

        //Temporary workaround until we get an EACore override that will
        //prevent site stripes from showing up.
        //If the sitestripe is visible, we might end up scrolling
        //an element behind the sitestripe into an unclickable position
        SystemMessage siteStripe = new SystemMessage(driver);
        if (siteStripe.isSiteStripeVisible()) {
            siteStripe.closeSiteStripe();
        }

        //Scroll the navigation sidebar container to the position of
        //the target element
        String script = "arguments[1].scrollTop = arguments[0].offsetTop;";
        jsExec.executeScript(script, target, container);
    }

    /**
     * Check if the navigation sidebar is open.
     *
     * @return true if the sidebar is open. This is always true for non mobile.
     */
    public boolean isSidebarOpen() {
        return !isMobile()
                || waitForElementPresent(ORIGIN_NAV_SIDEBAR_HEADER)
                        .getAttribute("class")
                        .contains("origin-navigation-isvisible");
    }

    /**
     * Opens the sidebar if it is closed
     */
    public void openSidebar() {
        if (isMobile() && !isSidebarOpen()) {
            waitForElementClickable(HAMBURGER_MENU).click();
        }
    }

    /**
     * Closes the sidebar if it is open
     */
    public void closeSidebar() {
        if (isMobile() && isSidebarOpen()) {
            waitForElementClickable(HAMBURGER_MENU).click();
        }
    }

    /**
     * @return true if the language preferences icon (appears only when logged
     * out) is visible, false otherwise
     */
    public boolean verifyLoggedOutLanguagePreferencesIconVisible() {
        return waitIsElementVisible(ORIGIN_LOGGEDOUT_LANGAUGE_PREFERENCES_ICON_LOCATOR, 2);
    }

    /**
     * Click the language preferences icon (appears only when logged out), or
     * throw exception if the icon is not visible
     */
    public void clickLoggedOutLanguagePreferencesIcon() {
        if (verifyLoggedOutLanguagePreferencesIconVisible()) {
            waitForElementClickable(ORIGIN_LOGGEDOUT_LANGAUGE_PREFERENCES_ICON_LOCATOR).click();
        } else {
            throw new RuntimeException("Error: Logged-out 'Language Preferences' icon is not visible");
        }
    }

    /**
     * Checks that the visible title of the 'Game Library' menu option matches
     * the input string
     *
     * @return true if the title of the 'Game Library' menu option matches the
     * input string
     */
    public boolean verifyGameLibraryString() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(GAME_LIBRARY_LOCATOR).getText(), I18NUtil.getMessage("library"));
    }
}
