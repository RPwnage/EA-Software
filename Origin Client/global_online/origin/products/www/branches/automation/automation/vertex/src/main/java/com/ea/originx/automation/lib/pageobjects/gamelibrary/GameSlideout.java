package com.ea.originx.automation.lib.pageobjects.gamelibrary;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.utils.Waits;
import com.google.common.base.Function;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.NoSuchElementException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.support.ui.FluentWait;
import java.lang.invoke.MethodHandles;
import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * A page object that represents a game slideout.
 *
 * @author glivingstone
 * @author palui
 */
public class GameSlideout extends EAXVxSiteTemplate {

    private static final int DOWNLOADINSTALLPLAY_BUTTON_VERIFICATION_INTERVAL_MILLISEC = 500;

    protected static final String SLIDEOUT_CSS = "origin-gamelibrary-ogd";
    protected static final String GAME_BACKGROUND_IMAGE_CSS = SLIDEOUT_CSS + " .origin-ogd-backgroundimage.packart";
    protected static final String GAME_INFO_CSS = SLIDEOUT_CSS + " .origin-ogd-gameinfo";
    protected static final String GAME_DETAILS_CSS = SLIDEOUT_CSS + " .origin-ogd-gamedetails";
    protected static final String ACTION_ITEMS_CSS = GAME_DETAILS_CSS + " .origin-ogd-actionitems .origin-ogd-actionitem";

    // Close Button
    protected static final By SLIDEOUT_CLOSE_BUTTON_LOCATOR = By.cssSelector(SLIDEOUT_CSS + " .origin-ogd-close .origin-ogd-icon-closecircle");

    // Game Background Image
    protected static final By GAME_BACKGROUND_IMAGE_LOCATOR = By.cssSelector(GAME_BACKGROUND_IMAGE_CSS + " .origin-store-blurimage img.origin-store-blurimage-image");

    // Game Info
    protected static final By GAME_TITLE_LOCATOR = By.cssSelector(GAME_INFO_CSS + " .otktitle-2.origin-ogd-gametitle");
    protected static final By NON_ORIGIN_GAME_LABEL_LOCATOR = By.cssSelector(GAME_INFO_CSS + " .otktitle-4.origin-ogd-nog");
    protected static final By GAME_ICON_IMAGE_LOCATOR = By.cssSelector(GAME_INFO_CSS + " img.origin-ogd-gamepackart");
    protected static final By ORIGIN_ACCESS_BADGE_LOCATOR = By.cssSelector(SLIDEOUT_CSS + " .origin-ogd-accesspremiumgamelogo i");

    protected static final String EXPECTED_NON_ORIGIN_GAME_LABEL = "Non-Origin Game";
    
    // Game Stats
    protected static final String GAME_STATS_CSS = GAME_DETAILS_CSS + " .origin-ogd-gamestats";
    protected static final By HOURS_PLAYED_LOCATOR = By.cssSelector(GAME_STATS_CSS + " section:first-child");
    protected static final By ACTUAL_HOURS_PLAYED_TEXT = By.cssSelector(GAME_STATS_CSS + " section:first-child .otktitle-4");

    // Game Details
    protected static final By DOWNLOADINSTALLPLAY_BUTTON_LOCATOR = By.cssSelector(GAME_DETAILS_CSS + " origin-cta-downloadinstallplay .origin-cta-primary .otkbtn.otkbtn-primary");
    protected static final By DOWNLOADINSTALLPLAY_BUTTON_TEXT_LOCATOR = By.cssSelector(GAME_DETAILS_CSS + " origin-cta-downloadinstallplay .origin-cta-primary .otkbtn.otkbtn-primary .otkbtn-primary-text");
    protected static final By MESSAGE_TITLE_LOCATOR = By.cssSelector(GAME_DETAILS_CSS + " .origin-ogd-messages .origin-ogd-message-title-content");
    protected static final By UPGRADE_NOW_CTA_LOCATOR = By.cssSelector(GAME_DETAILS_CSS + " .origin-ogd-messages .origin-ogd-message-title-content .otka");

    protected static final By OGD_MSG_TITLE_CONTENT_HYPERLINK_LOCATOR = By.cssSelector(GAME_DETAILS_CSS + " .origin-ogd-messages .origin-ogd-message-title-content .otka");
    protected static final By OGD_MSG_TITLE_CONTENT_LOCATOR = By.cssSelector(GAME_DETAILS_CSS + " .origin-ogd-messages .origin-ogd-message-title-content");
    protected static final By DOWNLOAD_ICON_LOCATOR = By.cssSelector(GAME_DETAILS_CSS + " .origin-ogd-message-icon");
    protected static final String[] MESSAGE_KEYWORDS = {"Upgrade", "Game"};
    
    protected static final By BUY_THE_GAME_LINK_LOCATOR = By.cssSelector(GAME_DETAILS_CSS + " .origin-ogd-messages .origin-ogd-message-title-content .otka:nth-child(2)");
    protected static final String[] ORIGIN_ACCESS_EXPIRED_KEYWORDS = {"Origin", "Access", "Expired"};
    protected static final String EXPECTED_RENEW_MEMBERSHIP_TEXT = "Renew membership";
    protected static final String EXPECTED_BUY_GAME_TEXT = "Buy the game";
    protected static final String EXPECTED_UPGRADE_NOW_TEXT = "Upgrade now";

    // Navigation links
    protected static final String NAVIGATION_LIST_XPATH = "//UL[contains(@class,'otknav-pills')]//li[contains(@class,'otkpill otkpill-light')]";
    protected static final String NAVIGATION_LIST_ACTIVE_XPATH = "//UL[contains(@class,'otknav-pills')]//li[contains(@class,'otkpill otkpill-active otkpill-light')]";
    protected static final By FRIENDS_WHO_PLAY_NAV_LINK_LOCATOR = By.xpath(NAVIGATION_LIST_XPATH + "//a[contains(text(),'Friend')]");
    protected static final By FRIENDS_WHO_PLAY_NAV_LINK_ACTIVE_LOCATOR = By.xpath(NAVIGATION_LIST_ACTIVE_XPATH + "//a[contains(text(),'Friend')]");
    protected static final By ACHIEVEMENTS_NAV_LINK_LOCATOR = By.xpath(NAVIGATION_LIST_XPATH + "//a[text()='Achievements']");
    protected static final By ACHIEVEMENTS_NAV_LINK_ACTIVE_LOCATOR = By.xpath(NAVIGATION_LIST_ACTIVE_XPATH + "//a[text()='Achievements']");
    protected static final By EXTRA_CONTENT_NAV_LINK_LOCATOR = By.xpath(NAVIGATION_LIST_XPATH + "//a[text()='Extra Content']");
    protected static final By STUFF_PACKS_NAV_LINK_LOCATOR = By.xpath(NAVIGATION_LIST_XPATH + "//a[text()='Stuff Packs']");
    protected static final By EXTRA_CONTENT_NAV_LINK_ACTIVE_LOCATOR = By.xpath(NAVIGATION_LIST_ACTIVE_XPATH + "//a[text()='Extra Content']");
    protected static final By EXPANSION_PACKS_NAV_LINK_ACTIVE_LOCATOR = By.xpath(NAVIGATION_LIST_XPATH + "//a[text()='Expansion Packs']");
    protected static final By FRIEND_TILES_LOCATOR = By.cssSelector("div.origin-usertile.origin-tile");
    protected static final By ACHIEVEMENT_TILES_LOCATOR = By.cssSelector("origin-achievement-badge");
    protected static final By EXTRA_CONTENT_TILES_LOCATOR = By.cssSelector("div.origin-gamecard");
    protected static final By NAV_LINKS_LOCATOR = By.cssSelector("li.otkpill.otkpill-overflow.otkdropdown.otkpill-light > div > ul > li > a");
    protected static final By MORE_DROPDOWN_NAV_LINK_LOCATOR = By.cssSelector(".origin-pills .otkpill-overflow");

    // Settings (Cogwheel)
    protected static final String SETTINGS_CSS = ACTION_ITEMS_CSS + " .origin-ogd-actionitem-settings";

    // Favorites button
    protected static final String FAVORITES_CSS = ACTION_ITEMS_CSS + " .origin-ogd-actionitem-favorite";
    protected static final By IS_FAVORITED_LOCATOR = By.cssSelector(FAVORITES_CSS + ".favorited");
    protected static final By FAVORITES_BUTTON_LOCATOR = By.cssSelector(FAVORITES_CSS + " > i");

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public GameSlideout(WebDriver driver) {
        super(driver);
    }

    /**
     * Get the cogwheel object which has a context menu for actions on the game.
     *
     * @return Cogwheel object
     *
     */
    public GameSlideoutCogwheel getCogwheel() {
        return new GameSlideoutCogwheel(driver, SETTINGS_CSS);
    }

    /**
     * Waits for the slide out to render onto the DOM.
     *
     * @return true when the slide out is present, false otherwise
     */
    public boolean waitForSlideout() {
        return Waits.pollingWait(() -> isElementPresent(GAME_TITLE_LOCATOR));
    }

    /**
     * Function to start download.
     *
     * @param timeout Time to wait until downloadable
     */
    public void startDownload(int timeout) {
        if (verifyDownloadableState(timeout)) {
            clickDownloadInstallPlayButton();
        }
    }

    /**
     * Launch the game by clicking the 'Play' button.
     *
     * @param timeout Time to wait until playable
     */
    public void startPlay(int timeout) {
        if (verifyPlayableState(timeout)) {
            clickDownloadInstallPlayButton();
        }
    }

    /**
     * Install the game.
     *
     * @param timeout Time to wait until it can be clicked to be installed
     */
    public void startInstall(int timeout) {
        if (verifyInstallState(timeout)) {
            clickDownloadInstallPlayButton();
        }
    }

    /**
     * Pause game download.
     *
     * @param timeout Time to wait until it can be clicked to be paused
     */
    public void pauseGameDownload(int timeout) {
        if (verifyDownloadPausableState(timeout)) {
            clickDownloadInstallPlayButton();
        }
    }

    /**
     * Clicks the 'Upgrade Now' link. Only available on standard entitlements
     * when there is an upgraded vault entitlement, and the user has an active
     * subscription.
     */
    public void upgradeEntitlement() {
        sleep(1000); // wait for animation to be finished
        waitForElementClickable(OGD_MSG_TITLE_CONTENT_HYPERLINK_LOCATOR).click();
    }
    
    /**
     * Verify the 'Upgrade now' link is visible
     *
     * @return true if the text contains the string 'Upgrade now' , and false
     * otherwise
     */
    public boolean verifyUpgradeNowLinkVisible(){
        return StringHelper.containsIgnoreCase(waitForElementVisible(UPGRADE_NOW_CTA_LOCATOR).getText(), EXPECTED_UPGRADE_NOW_TEXT);
    }

    /**
     * Close the slideout with the 'X' button.
     */
    public void clickSlideoutCloseButton() {
        waitForElementVisible(SLIDEOUT_CLOSE_BUTTON_LOCATOR);
        waitForElementClickable(SLIDEOUT_CLOSE_BUTTON_LOCATOR).click();
        sleep(1000); // Wait for animation to complete.
    }

    /**
     * Verify if the game is downloadable.
     *
     * @param timeout Time to wait until downloadable
     * @return true if verification passed, false otherwise
     */
    public boolean verifyDownloadableState(int timeout) {
        return verifyButtonText("Download", timeout);
    }

    /**
     * Verify if the game is downloading.
     *
     * @param timeout Time to wait until downloading
     * @return true if verification passed, false otherwise
     */
    public boolean verifyDownloadingState(int timeout) {
        return verifyButtonText("Installing", timeout);
    }

    /**
     * Verify if game is ready to install.
     *
     * @param timeout Time to wait until install
     * @return true if verification passed, false otherwise
     */
    public boolean verifyInstallState(int timeout) {
        return verifyButtonText("Install", timeout);
    }

    /**
     * Verify if game is installing.
     *
     * @param timeout Time to wait until installing
     * @return true if verification passed, false otherwise
     */
    public boolean verifyFinalizingState(int timeout) {
        return verifyButtonText("Finalizing", timeout);
    }

    /**
     * Verify if game is playable.
     *
     * @param timeout Time to wait until playable
     * @return true if verification passed, false otherwise
     */
    public boolean verifyPlayableState(int timeout) {
        return verifyButtonText("Play", timeout);
    }

    /**
     * Verify if game download is pause-able.
     *
     * @param timeout Wait until download is pause-able
     * @return true if verification passed, false otherwise
     */
    public boolean verifyDownloadPausableState(int timeout) {
        return verifyButtonText("Pause Download", timeout);
    }

    /**
     * Verify if game is playing.
     *
     * @param timeout Wait until game is playing
     * @return true if verification passed, false otherwise
     */
    public boolean verifyPlayingState(int timeout) {
        return verifyButtonAttr("disabled", "true", timeout);
    }

    /**
     * Verify that the game title in the game slideout matches the expected
     * game title.
     *
     * @param expectedTitle The expected game title displayed on the game
     * slideout
     * @return true if the game title on the game slideout matches the expected
     * title, false otherwise
     */
    public boolean verifyGameTitle(String expectedTitle) {
        WebElement gameTitle = waitForElementPresent(GAME_TITLE_LOCATOR);
        return gameTitle.getAttribute("textContent").equals(expectedTitle);
    }

    /**
     * Verify the game slideout has a label containing (case ignored)
     * 'Non-Origin Game'.
     *
     * @return true if expected label found, false otherwise
     */
    public boolean verifyNonOriginGameLabel() {
        String label = waitForElementVisible(NON_ORIGIN_GAME_LABEL_LOCATOR).getText();
        return StringHelper.containsIgnoreCase(label, EXPECTED_NON_ORIGIN_GAME_LABEL);
    }

    /**
     * Verify game slideout background image is visible.
     *
     * @return true if background image is visible, false otherwise
     */
    public boolean verifyBackgroundImageVisible() {
        return waitIsElementVisible(GAME_BACKGROUND_IMAGE_LOCATOR);
    }

    /**
     * Get game slideout background image src.
     *
     * @return String of the background image src
     */
    public String getBackgroundImageSrc() {
        return waitForElementVisible(GAME_BACKGROUND_IMAGE_LOCATOR).getAttribute("src");
    }

    /**
     * Verifies the navigation section is visible.
     *
     * @return true if the navigation section is visible, false otherwise
     */
    public boolean verifyNavLinksVisible() {
        return (waitIsElementVisible(By.xpath(NAVIGATION_LIST_XPATH)));
    }

    /**
     * Verify the 'Friends Who Play' pill is on the navigation section.
     *
     * @return true if the 'Friends Who Play' nav button exists, false otherwise
     */
    public boolean verifyFriendsWhoPlayNavLinkVisible() {
        return waitIsElementVisible(FRIENDS_WHO_PLAY_NAV_LINK_LOCATOR, 2);
    }

    /**
     * Verify the 'Achievements' pill is on the navigation section.
     *
     * @return true if the 'Achievements' nav button exists, false otherwise
     */
    public boolean verifyAchievementsNavLinkVisible() {
        return waitIsElementVisible(ACHIEVEMENTS_NAV_LINK_LOCATOR, 2);
    }

    /**
     * Verify the 'Expansions' pill is on the navigation section.
     *
     * @return true if the 'Expansions' nav button exists
     */
    public boolean verifyExtraContentNavLinkVisible() {
        return waitIsElementVisible(EXTRA_CONTENT_NAV_LINK_LOCATOR, 2);
    }

    /**
     * Verify the  'Friends Who Play' pill is  active on the navigation section.
     *
     * @return true if the active 'Friends Who Play' nav button is active, false otherwise
     */
    public boolean verifyFriendsWhoPlayNavLinkActive() {
        return waitIsElementVisible(FRIENDS_WHO_PLAY_NAV_LINK_ACTIVE_LOCATOR);
    }

    /**
     * Verify the 'Achievements' pill is active on the navigation section.
     *
     * @return true if the 'Achievements' nav button is active, false otherwise
     */
    public boolean verifyAchievementsNavLinkActive() {
        return waitIsElementVisible(ACHIEVEMENTS_NAV_LINK_ACTIVE_LOCATOR);
    }

    /**
     * Verify the 'Expansions' pill is active on the navigation section.
     *
     * @return true if the 'Expansions' nav button is active, false otherwise
     */
    public boolean verifyExtraContentNavLinkActive() {
        return waitIsElementVisible(EXTRA_CONTENT_NAV_LINK_ACTIVE_LOCATOR);
    }

    /**
     * Verify downloadinstallplay button's text equals to given text
     * and wait for given timeout.
     *
     * @param expected String that contains expected text
     * @param timeout Timeout in seconds
     * @return true if the button text is as expected, false otherwise
     */
    private boolean verifyButtonText(String expected, int timeout) {
        final WebElement btn = getDownloadInstallPlayButton();
        if (!btn.getText().equals(expected)) {
            new FluentWait<WebElement>(btn).
                    withTimeout(timeout, TimeUnit.SECONDS).
                    pollingEvery(DOWNLOADINSTALLPLAY_BUTTON_VERIFICATION_INTERVAL_MILLISEC, TimeUnit.MILLISECONDS).
                    until(new Function<WebElement, Boolean>() {
                        @Override
                        public Boolean apply(WebElement element) {
                            try {
                                return expected.equals(element.getText());
                            } catch (NoSuchElementException ignore) {
                                return false;
                            }
                        }
                    });
        }
        return true;
    }

    /**
     * Verify downloadinstallplay button's attribute equals to given
     * value and wait for given timeout.
     *
     * @param name String that contains attribute name
     * @param expected String that contains expected value
     * @param timeout Timeout in seconds
     * @return true if the button's attribute is as expected, false otherwise
     */
    private boolean verifyButtonAttr(String name, String expected, int timeout) {
        final WebElement btn = getDownloadInstallPlayButton();
        if (null == btn.getAttribute(name) || !btn.getAttribute(name).equals(expected)) {
            new FluentWait<WebElement>(btn).
                    withTimeout(timeout, TimeUnit.SECONDS).
                    pollingEvery(DOWNLOADINSTALLPLAY_BUTTON_VERIFICATION_INTERVAL_MILLISEC, TimeUnit.MILLISECONDS).
                    until(new Function<WebElement, Boolean>() {
                        @Override
                        public Boolean apply(WebElement element) {
                            try {
                                return (null != element.getAttribute(name) && expected.equals(element.getAttribute(name)));
                            } catch (NoSuchElementException ignore) {
                                return false;
                            }
                        }
                    });
        }
        return true;
    }

    /**
     * Verify downloadinstallplay button's state equals to given
     * state and wait for given timeout.
     *
     * @param expected The expected state
     * @param timeout Timeout in seconds
     * @return true if the button's state is as expected
     */
    public boolean verifyButtonEnabled(final boolean expected, int timeout) {
        final WebElement btn = getDownloadInstallPlayButton();
        if (expected != btn.isEnabled()) {
            new FluentWait<WebElement>(btn).
                    withTimeout(timeout, TimeUnit.SECONDS).
                    pollingEvery(DOWNLOADINSTALLPLAY_BUTTON_VERIFICATION_INTERVAL_MILLISEC, TimeUnit.MILLISECONDS).
                    until(new Function<WebElement, Boolean>() {
                        @Override
                        public Boolean apply(WebElement element) {
                            try {
                                return element.isEnabled() == expected;
                            } catch (NoSuchElementException ignore) {
                                return false;
                            }
                        }
                    });
        }
        return true;
    }

    /**
     * Verify the 'Download/Install/Play' button is visible
     *
     * @return true if the button is visible, false otherwise
     */
    public boolean verifyButtonVisible() {
        return waitIsElementVisible(DOWNLOADINSTALLPLAY_BUTTON_LOCATOR);
    }

    /**
     * Getter for the 'Download/Install/Play' button.
     *
     * @return WebElement for the 'Download/Install/Play' button
     */
    private WebElement getDownloadInstallPlayButton() {
        return waitForElementVisible(DOWNLOADINSTALLPLAY_BUTTON_LOCATOR);
    }

    /**
     * Getter for the 'Download/Install/Play' button text.
     *
     * @return WebElement for the 'Download/Install/Play' button text
     */
    private WebElement getDownloadInstallPlayButtonText() {
        return waitForElementVisible(DOWNLOADINSTALLPLAY_BUTTON_TEXT_LOCATOR);
    }

    /**
     * Click the 'Download/Install/Play' button.
     */
    private void clickDownloadInstallPlayButton() {
        waitForElementClickable(getDownloadInstallPlayButtonText()).click();
    }

    /**
     * Verify if the game slideout shows the game is favorited.
     *
     * @return true if favorited, false otherwise
     */
    public boolean isFavorited() {
        return isElementVisible(IS_FAVORITED_LOCATOR);
    }

    /**
     * Click the 'Favourites' button (to toggle between adding and removing the
     * game from the favorites list).
     */
    public void clickFavoritesButton() {
        waitForElementClickable(FAVORITES_BUTTON_LOCATOR).click();
    }

    /**
     * Click 'Extra Content'.
     */
    public void clickExtraContentTab() {
        waitForElementClickable(EXTRA_CONTENT_NAV_LINK_LOCATOR, 5).click();
        waitForAnimationComplete(GAME_TITLE_LOCATOR);
    }

    /**
     * Click 'Stuff Pack'.
     */
    public void clickStuffPackTab(){
        WebElement moreMenu;
        try {
            moreMenu = waitForElementClickable(MORE_DROPDOWN_NAV_LINK_LOCATOR);
        } catch (Exception e){
            moreMenu = null;
            _log.error(e.getMessage());
        }
        // Need to check if the "More" drop down menu is available and check if the tab is withing the overflow menu
        if (moreMenu != null){
            moreMenu.click();
            List<WebElement> getOverflowTabs = getAllTabs();
            for (WebElement menuItem : getOverflowTabs) {
                if(menuItem.getAttribute("data-telemetry-label").toLowerCase().contains("stuff packs")){
                    scrollToElement(menuItem);
                    sleep(500); // need to wait for scroll
                    menuItem.click();
                    break;
                }
            }
        } else {
            waitForElementClickable(STUFF_PACKS_NAV_LINK_LOCATOR, 5).click();
        }
    }

    /**
     * Gets all the Nav Link elements.
     *
     * @return List of all Nav Link elements
     */
    private List<WebElement> getAllTabs(){
        return driver.findElements(NAV_LINKS_LOCATOR);
    }

    /**
     * Click 'Expansion Packs'.
     */
    public void clickExpansionPacksTab() {
        waitForElementClickable(EXPANSION_PACKS_NAV_LINK_ACTIVE_LOCATOR, 5).click();
    }

    /**
     * Click 'Friends Who Play'.
     */
    public void clickFriendsWhoPlayTab() {
        waitForElementClickable(FRIENDS_WHO_PLAY_NAV_LINK_LOCATOR).click();
    }

    /**
     * Click 'Achievements'.
     */
    public void clickAchievementsTab() {
        waitForElementClickable(ACHIEVEMENTS_NAV_LINK_LOCATOR).click();
    }
    
    /**
     * Click 'Upgrade Now' link.
     */
    public void clickUpgradeNowLink() {
        waitForElementClickable(UPGRADE_NOW_CTA_LOCATOR).click();
    }

    /**
     * Verify the there are friends listed in the 'Friends Who Play' tab.
     *
     * @return true if there are friends listed in the 'Friends Who Play' tab,
     * false otherwise
     */
    public boolean verifyFriendsInFriendsWhoPlayTab() {
        return isElementPresent(FRIEND_TILES_LOCATOR);
    }

    /**
     * Verify there are achievements in the 'Achievement' tab.
     *
     * @return true if there are achievements in the 'Achievement' tab,
     * false otherwise
     */
    public boolean verifyAchievementsInAchievementsTab() {
        return isElementPresent(ACHIEVEMENT_TILES_LOCATOR);
    }

    /**
     * Verify there is extra content in the 'Extra Content' tab.
     *
     * @return true if there is extra content in the 'Extra Content' tab,
     * false otherwise
     */
    public boolean verifyExtraContentInExtraContentTab() {
        scrollToElement(waitForAnyElementVisible(EXTRA_CONTENT_NAV_LINK_LOCATOR));
        return waitIsElementVisible(EXTRA_CONTENT_TILES_LOCATOR, 10);
    }

    /**
     * Verify that the offer ID in the game slideout matches the expected
     * offer ID for the game.
     *
     * @param offerID The expected offer ID for the game slideout
     * @return true if the offer ID on the game slideout matches the expected
     * offer ID, false otherwise
     */
    public boolean verifyGameTiteByOfferID(String offerID) {
        WebElement gameTitle = waitForElementPresent(By.cssSelector(SLIDEOUT_CSS));
        return gameTitle.getAttribute("offerid").equals(offerID);
    }
    
    /**
     * Verify there is text displayed under the 'Hours Played' section of the OGD.
     * 
     * @return true if there is text that contains the string 'hour' in the
     * 'Hours Played' section, and false otherwise
     */
    public boolean verifyActualHoursPlayedText() {
        String actualHoursPlayedText = waitForElementPresent(ACTUAL_HOURS_PLAYED_TEXT).getText();
        return StringHelper.containsIgnoreCase(actualHoursPlayedText, "hour");
    }
    
    /**
     * Verify there is a violator stating upgrade the game is possible
     *
     * @return true if there is a visible violator that contains keywords, false
     * otherwise
     */
    public boolean verifyViolatorUpgradeGameVisible() {
        String violatorText = waitForElementVisible(MESSAGE_TITLE_LOCATOR).getText();
        return StringHelper.containsAnyIgnoreCase(violatorText, MESSAGE_KEYWORDS);
    }
    
    /**
     * Verify the download icon is visible
     * 
     * @return true if the download icon is visible, false otherwise 
     */
    public boolean verifyDownloadIconVisible(){
        return waitIsElementPresent(DOWNLOAD_ICON_LOCATOR);
    }
    
    /**
     * Verify the download icon is white
     * 
     * @return true if the download icon is white, false otherwise 
     */
    public boolean verifyDownloadIconColor() {
         return getColorOfElement(waitForElementVisible(DOWNLOAD_ICON_LOCATOR)).equals(OriginClientData.OGD_DOWNLOAD_ICON_COLOR);
    }

    /**
     * Verify if the Origin Access Memebership has expired text displayed
     * 
     * @return true if the entitlement has expired text displayed
     * and false otherwise
     */
    public boolean verifyOriginAccessMembershipHasExpired() {
        return waitIsElementVisible(OGD_MSG_TITLE_CONTENT_LOCATOR)? 
        StringHelper.containsIgnoreCase(waitForElementVisible(OGD_MSG_TITLE_CONTENT_LOCATOR).getText(), "expired") : false;
    }

    /**
     * Verify if the CTA button for the entitlement slideout is Enabled
     * 
     * @return true if the entitlement is Enabled,
     * and false otherwise
     */
    public boolean verifyGameEnabled() {
        return waitIsElementVisible(DOWNLOADINSTALLPLAY_BUTTON_LOCATOR) ?
                !waitForElementVisible(DOWNLOADINSTALLPLAY_BUTTON_LOCATOR).getAttribute("class").contains("isdisabled") : false;
    }

     /* Verify the 'Origin Access' badge is visible
     * 
     * @return true if the badge is visible, false otherwise
     */
    public boolean verifyOriginAccessBadgeVisible(){
        return waitIsElementVisible(ORIGIN_ACCESS_BADGE_LOCATOR);
    }
    
    /**
     * Verify the link to resubscribe 'Origin Access' is visible
     *
     * @return true if the text contains the string 'Renew
     * Membership', and false otherwise
     */
    public boolean verifyRenewMembershipLinkVisible(){
        return StringHelper.containsIgnoreCase(waitForElementVisible(UPGRADE_NOW_CTA_LOCATOR).getText(), EXPECTED_RENEW_MEMBERSHIP_TEXT);
    }
    
    /**
     * Verify the 'Buy the game' link is visible
     *
     * @return true if the text contains the string 'Buy the Game' , and false
     * otherwise
     */
    public boolean verifyBuyTheGameLinkVisible(){
        return StringHelper.containsIgnoreCase(waitForElementVisible(BUY_THE_GAME_LINK_LOCATOR).getText(), EXPECTED_BUY_GAME_TEXT);
    }
    
    /**
     * Verify there is a text stating 'Origin Access expired'
     *
     * @return true if there is a visible text that contains keywords, false
     * otherwise
     */
    public boolean verifyOriginAccessExpiredTextVisible() {
        return StringHelper.containsAnyIgnoreCase(waitForElementVisible(MESSAGE_TITLE_LOCATOR).getText(), ORIGIN_ACCESS_EXPIRED_KEYWORDS);
    }
}