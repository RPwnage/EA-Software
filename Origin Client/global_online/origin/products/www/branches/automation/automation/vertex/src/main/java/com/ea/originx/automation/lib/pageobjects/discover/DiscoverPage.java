package com.ea.originx.automation.lib.pageobjects.discover;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.pageobjects.common.MyHomeCarousel;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.*;
import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.util.List;
import java.util.stream.Collectors;
import org.testng.util.Strings;

/**
 * Page object representing the 'My Home' page.
 *
 * @author tharan
 * @author palui
 */
public class DiscoverPage extends EAXVxSiteTemplate {

    protected static final String DISCOVER_PAGE_CSS = "#origin-content #content origin-home-page-loggedin";
    protected static final By WELCOME_PAGE_LOCATOR = By.cssSelector(".origin-welcome .otktitle-page");
    protected static final By WELCOME_PAGE_NEW_USER_LOCATOR = By.cssSelector("#origin-content #content origin-home-page-loggedin-empty-state");
    protected static final By WELCOME_PAGE_EMPTY_STATE_LOCATOR = By.cssSelector(".origin-home-empty-state-grid-layout");
    protected static final By DISCOVER_ALL_IMAGES_LOCATOR = By.cssSelector(DISCOVER_PAGE_CSS + " div[style*='http']");
    protected static final By DISCOVER_PAGE_LOCATOR = By.cssSelector(DISCOVER_PAGE_CSS);
    protected static final By HOME_PAGE_OFFLINE_MESSAGE_LOCATOR = By.cssSelector("#content origin-home-page-offline origin-offline-cta .otktitle-2");
    protected static final By GO_BACK_ONLINE_MESSAGE_LOCATOR = By.cssSelector("#content origin-home-page-offline origin-offline-cta .origin-message-content");
    protected static final By HOME_PAGE_RECOMMENDATION_TILE_SECTION_LOCATOR = By.cssSelector(".origin-home-recommended-tiles");

    protected static final String EMPTY_STATE_TILE_CSS = ".origin-homepage-empty-state-tile[text='%s']";
    protected static final By BROWSE_GAMES_LOCATOR = By.cssSelector(String.format(EMPTY_STATE_TILE_CSS, "BROWSE GAMES"));
    protected static final By PLAY_A_FREE_GAME_LOCATOR = By.cssSelector(String.format(EMPTY_STATE_TILE_CSS, "PLAY A FREE GAME"));
    protected static final By BUY_SOME_GAMES_LOCATOR = By.cssSelector(String.format(EMPTY_STATE_TILE_CSS, "BUY SOME GAMES"));
    protected static final By CHECK_OUT_ORIGIN_ACCESS_LOCATOR = By.cssSelector(String.format(EMPTY_STATE_TILE_CSS, "CHECK OUT ORIGIN ACCESS"));

    protected static final String WHAT_FRIENDS_ARE_DOING_CSS = DISCOVER_PAGE_CSS + " origin-home-discovery-area .discovery-area origin-home-discovery-bucket[title='What Your Friends Are Doing']";
    protected static final String WHAT_FRIENDS_ARE_DOING_TILES_CSS = WHAT_FRIENDS_ARE_DOING_CSS + " .origin-home-section .origin-home-discovery-tiles .l-origin-home-discovery-tile";
    protected static final String WHAT_FRIENDS_ARE_DOING_GAME_TILE_CSS_TMPL = WHAT_FRIENDS_ARE_DOING_TILES_CSS + " .origin-tile[offerid='%s']";
    protected static final String WHAT_FRIENDS_ARE_DOING_GAME_TILE_PLAY_BUTTON_CSS_TMPL = WHAT_FRIENDS_ARE_DOING_GAME_TILE_CSS_TMPL
            + " .origin-tile-container origin-home-discovery-tile-game-overlay .origin-tile-overlay-content origin-cta-downloadinstallplay .origin-cta-primary .otkbtn.otkbtn-primary";

    protected static final String SECTIONS_CSS = DISCOVER_PAGE_CSS + " origin-home-discovery-bucket";
    protected static final String RECOMMEND_FRIENDS_SECTION_CSS = SECTIONS_CSS + "[title-str='People you may know']";
    protected static final By RECOMMEND_FRIENDS_SECTION_LOCATOR = By.cssSelector(RECOMMEND_FRIENDS_SECTION_CSS);
    // Currently the section "buckets" can only be uniquely identified with title-str. Below is the older string used and
    //  may be removed once the newer string is deployed on all environments
    protected static final String RECOMMEND_FRIENDS_SECTION_ALTERNATIVE_CSS = SECTIONS_CSS + "[title-str='Find more friends and have fun']";
    protected static final By RECOMMEND_FRIENDS_SECTION_ALTERNATIVE_LOCATOR = By.cssSelector(RECOMMEND_FRIENDS_SECTION_ALTERNATIVE_CSS);

    protected static final String EXPECTED_OFFLINE_MESSAGE_TEXT = "You're Offline";
    protected static final String[] EXPECTED_GO_ONLINE_MESSAGE_KEYWORDS = {"Go", "online"};

    // Entitlement Tile
    private static final String ANY_TILE_WITH_OFFERID_TMPL = "[offerid='%s']";
    private static final String PLAYING_USER_LOCATOR = ".origin-avatarlist .otkavatar-isingame img[alt='%s']";

    // Gifting
    private static final By NOTIFICATION_LINK_LOCATOR = By.cssSelector(".origin-inbox-mini-header");
    private static final By OPEN_GIFT_CTA_LOCATOR = By.cssSelector(".origin-inbox-item");

    // Carousels
    private static final String HOME_ALL_CAROUSELS_LOCATOR = "origin-home-discovery-bucket section ";
    private static final String HOME_CAROUSEL_TITLE_LOCATOR = " h2";
    private static final String ALL_CAROUSELS_INDICATORS_LOCATOR = "div .origin-store-carousel-product-indicators-container";

    // Game updates and events
    private static final String GAME_UPDATES_AND_EVENTS_SECTION_CSS = SECTIONS_CSS + "[title-str='Game news and articles']";
    private static final By GAME_UPDATES_AND_EVENTS_SECTION_LOCATOR = By.cssSelector(GAME_UPDATES_AND_EVENTS_SECTION_CSS);

    protected static final String DISCOVER_PAGE_URL = ".*origin.com.*home.*";

    protected static final By REFRESH_PAGE_BUTTON_LOCATOR = By.cssSelector("origin-home-newcontentbanner a.origin-newcontentbanner");

    // get all hyper links in home page
    protected static final By ALL_HYPERLINK_LIST = By.cssSelector("#content a[href]");

    protected static final By VIEW_MORE_BUTTONS_LOCATOR = By.cssSelector("a[ng-click='showMoreTiles()'][ng-bind-html='viewmoreStr']");

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public DiscoverPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Refresh the 'My Home' page by clicking the small button at the top.
     */
    public void refreshPage() {
        waitForElementClickable(REFRESH_PAGE_BUTTON_LOCATOR).click();
    }

    /**
     * Verify the 'Refresh' button is visible.
     *
     * @return true if the 'Refresh' button is visible
     */
    public boolean verifyRefreshButtonVisible() {
        return waitIsElementVisible(REFRESH_PAGE_BUTTON_LOCATOR);
    }

    /**
     * Automatically open the gift and add it to the 'Game Library'.
     */
    public void openGiftAddToLibrary() {
        this.clickUnopenedGiftTile();
        OpenGiftPage openGiftPage = new OpenGiftPage(driver);
        openGiftPage.waitForVisible();
        openGiftPage.clickDownloadLater();
    }

    /**
     * Verify if the 'My Home' page has been reached.
     *
     * @return true if page reached, false otherwise
     */
    public boolean verifyDiscoverPageReached() {
        return waitIsElementPresent(DISCOVER_PAGE_LOCATOR);
    }

    /**
     * Verify if the 'My Home' page has been reached after a new account is
     * created.
     *
     * @return true if page reached after a new account is created, false
     * otherwise
     */
    public boolean verifyDiscoverPageReachedWithNewAccount() {
        return waitIsElementPresent(WELCOME_PAGE_NEW_USER_LOCATOR);
    }

    /**
     * Verify if the 'Recommended Friends' section of this 'My Home' page is
     * visible.
     *
     * @return true if the 'Recommended Friends' section is visible, false
     * otherwise
     */
    public boolean verifyRecFriendsSectionVisible() {
        return waitIsElementVisible(RECOMMEND_FRIENDS_SECTION_LOCATOR, 5) || waitIsElementVisible(RECOMMEND_FRIENDS_SECTION_ALTERNATIVE_LOCATOR, 2);
    }

    /**
     * Return the 'Recommended Friends' section of this 'My Home' page.
     *
     * @return Section object if visible, or throw TimeoutException otherwise
     */
    public RecFriendsSection getRecFriendsSection() {
        WebElement sectionElement;
        if (waitIsElementVisible(RECOMMEND_FRIENDS_SECTION_LOCATOR, 5)) {
            sectionElement = waitForElementVisible(RECOMMEND_FRIENDS_SECTION_LOCATOR);
        } else {
            sectionElement = waitForElementVisible(RECOMMEND_FRIENDS_SECTION_ALTERNATIVE_LOCATOR);
        }
        return new RecFriendsSection(driver, sectionElement);
    }

    /**
     * Verify Offline message 'You're Offline' exists.
     *
     * @return true if offline message exists, false otherwise
     */
    public boolean verifyOfflineMessageExists() {
        return waitForElementVisible(HOME_PAGE_OFFLINE_MESSAGE_LOCATOR, 10).getText().equalsIgnoreCase(EXPECTED_OFFLINE_MESSAGE_TEXT);
    }

    /**
     * Verify the message stating how to go back online is visible.
     *
     * @return true if the message stating how to go back online is visible,
     * false otherwise
     */
    public boolean verifyGoBackOnlineMessageVisible() {
        String actualMessage = waitForElementVisible(GO_BACK_ONLINE_MESSAGE_LOCATOR).getText();
        return StringHelper.containsIgnoreCase(actualMessage, EXPECTED_GO_ONLINE_MESSAGE_KEYWORDS);
    }

    /**
     * Verify the 'Recommendation' section is visible.
     *
     * @return true if recommendation section is visible, false otherwise
     */
    public boolean isRecommendationVisible() {
        return waitIsElementVisible(HOME_PAGE_RECOMMENDATION_TILE_SECTION_LOCATOR, 10);
    }

    /**
     * Verify a game tile with matching offerId can be found under the 'What
     * Your Friends Are Doing' section.
     *
     * @param offerId Offer ID to match
     * @return true if match found, false otherwise
     */
    public boolean verifyWhatFriendsAreDoingGameTile(String offerId) {
        return waitIsElementPresent(By.cssSelector(String.format(WHAT_FRIENDS_ARE_DOING_GAME_TILE_CSS_TMPL, offerId)));
    }

    /**
     * Hover on game tile with matching offerId under the 'What Your Friends Are
     * Doing' section (to open the context menu).
     *
     * @param offerId Offer ID to locate the game tile
     */
    public void hoverOnWhatFriendsAreDoingGameTile(String offerId) {
        WebElement gameTile = waitForElementPresent(By.cssSelector(String.format(WHAT_FRIENDS_ARE_DOING_GAME_TILE_CSS_TMPL, offerId)));
        hoverOnElement(gameTile);
    }

    /**
     * Hover on game tile with matching offer ID under the 'What Your Friends
     * Are Doing' section and click the 'Play' button.
     *
     * @param offerId Offer ID to locate the game tile
     */
    public void clickPlayButtonAtWhatFriendsAreDoingGameTile(String offerId) {
        hoverOnWhatFriendsAreDoingGameTile(offerId);
        waitForElementClickable(By.cssSelector(String.format(WHAT_FRIENDS_ARE_DOING_GAME_TILE_PLAY_BUTTON_CSS_TMPL, offerId))).click();
    }

    /**
     * Wait for the home page to load.
     */
    public void waitForPage() {
        waitForPageThatMatches(DISCOVER_PAGE_URL, 30);
        waitForPageToLoad();
    }

    /**
     * Hover on the notification link and then click the 'Open' button.
     */
    public void clickUnopenedGiftTile() {
        WebElement notifications = waitForElementPresent(NOTIFICATION_LINK_LOCATOR);
        hoverOnElement(notifications);
        waitForElementPresent(OPEN_GIFT_CTA_LOCATOR).click();
    }

    /**
     * Verify the gift tile is the first tile on the home page.
     *
     * @return true if the gift tile is the first tile on the home page, false
     * otherwise
     */
    public boolean verifyGiftTilePosition() {
        By tilesLocator = By.cssSelector(".origin-tile");
        // Get the first origin tile and check that it is the gifting tile
        WebElement firstTile = waitForElementVisible(tilesLocator);
        WebElement parentElement = firstTile.findElement(By.xpath(".."));
        return parentElement.getTagName().equals("origin-home-recommended-action-gift");
    }

    /**
     * Locate all carousels, then find the ones which are bigger than the
     * standard and get the title
     */
    public MyHomeCarousel getFirstMultiPageCarouselTitle() {
        List<WebElement> allCarousels = driver.findElements(By.cssSelector(HOME_ALL_CAROUSELS_LOCATOR));
        String title = null;
        for (int i = 0; i < allCarousels.size(); i++) {
            if (!(allCarousels.get(i).findElement(By.cssSelector(ALL_CAROUSELS_INDICATORS_LOCATOR)) == null)) {
                title = allCarousels.get(i).findElement(By.cssSelector(HOME_CAROUSEL_TITLE_LOCATOR)).getText();
                break;
            }
        }
        return new MyHomeCarousel(driver, title);
    }

    /**
     * Checks if there is a game tile on the page with the given user in the
     * playing section of that tile..
     *
     * @param offerId The offer ID of the tile to look for
     * @param userName The username to check for in the given offer tile
     * @return true if a tile for the given entitlement exists on the page and
     * there is an avatar for the given user on that game tile, false otherwise
     */
    public boolean verifyTileHasUserAvatar(String offerId, String userName) {
        try {
            WebElement gameTile = waitForElementPresent(By.cssSelector(String.format(ANY_TILE_WITH_OFFERID_TMPL, offerId, 15)));
            waitForChildElementPresent(gameTile, By.cssSelector(String.format(PLAYING_USER_LOCATOR, userName, 5)));
            return true; // If the element exists we are good
        } catch (TimeoutException e) {
        }
        return false;
    }

    /**
     * Check if any of images do not exist.
     *
     * @return List with image URL if found, empty list otherwise
     */
    public List<String> verifyAllImagesExist() {
        List<String> imageNotFoundList = driver.findElements(DISCOVER_ALL_IMAGES_LOCATOR)
                .stream()
                .filter(element -> {
                    // style="background-image: url(&quot;https://qa.data2.origin.com/.../mirrors-edge-standard-edition_hometile.jpg&quot;);"
                    // to get URL only from style attribute as above
                    String sourceUrl = element.getAttribute("style").split("\"")[1];
                    try {
                        return TestScriptHelper.verifyBrokenLink(sourceUrl);
                    } catch (IOException e) {
                        _log.error(e);
                    }
                    return false;
                })
                .map(element -> element.getAttribute("style").split("\"")[1]).collect(Collectors.toList());
        return imageNotFoundList;
    }

    /**
     * Click the 'View More' button.
     */
    public void clickViewMoreButtons() {
        driver.findElements(VIEW_MORE_BUTTONS_LOCATOR)
                .forEach(button -> {
                    hoverOnElement(button);
                    button.click();
                });
    }

    /**
     * Verifies that the welcome message is visible at the top of the 'My Home'
     * page
     *
     * @return true if the welcome message is visible, false otherwise
     */
    public boolean verifyWelcomeMessageVisible() {
        String welcomeMessage = waitForElementVisible(WELCOME_PAGE_LOCATOR).getText();
        return !Strings.isNullOrEmpty(welcomeMessage);
    }

    /**
     * Verifies that the welcome message contains the user name
     *
     * @param userName The username to check for in the welcome message
     * @return true if the welcome message contains the username, false
     * otherwise
     */
    public boolean verifyWelcomeMessageContainsUserName(String userName) {
        return waitForElementVisible(WELCOME_PAGE_LOCATOR).getText().toLowerCase().contains(userName.toLowerCase());
    }

    /**
     * Verify 'Game Updates and Events' section is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyGameUpdatesAndEventsSectionVisible() {
        return waitIsElementPresent(GAME_UPDATES_AND_EVENTS_SECTION_LOCATOR);
    }

    /**
     * Verify 'My Home' is in an empty game state.
     *
     * @return true if it is, false otherwise
     */
    public boolean verifyEmptyGameState() {
        return waitIsElementVisible(WELCOME_PAGE_EMPTY_STATE_LOCATOR);
    }
    
    /**
     * Verify the 'Browse Games' tile is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyBrowseGamesTileVisible() {
        return waitIsElementVisible(BROWSE_GAMES_LOCATOR);
    }

    /**
     * Verify the 'Play a Free Game' tile is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyPlayAFreeGameTileVisible() {
        return waitIsElementVisible(PLAY_A_FREE_GAME_LOCATOR);
    }

    /**
     * Verify the 'Buy Some Games' tile is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyBuySomeGamesTileVisible() {
        return waitIsElementVisible(BUY_SOME_GAMES_LOCATOR);
    }

    /**
     * Verify the 'Check Out Origin Access' tile is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyCheckOutOATileVisible() {
        return waitIsElementVisible(CHECK_OUT_ORIGIN_ACCESS_LOCATOR);
    }
}
