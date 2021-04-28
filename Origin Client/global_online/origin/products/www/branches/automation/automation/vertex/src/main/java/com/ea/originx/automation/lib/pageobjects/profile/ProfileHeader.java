package com.ea.originx.automation.lib.pageobjects.profile;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.client.OriginClient;
import java.util.Arrays;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.openqa.selenium.*;
import org.openqa.selenium.support.Color;

/**
 * The header of the 'Profile' page.
 *
 * @author lscholte
 * @author mdobre
 */
public class ProfileHeader extends EAXVxSiteTemplate {

    private static final org.apache.logging.log4j.Logger _log = LogManager.getLogger(ProfileHeader.class);

    protected static final By PROFILE_HEADER_LOCATOR = By.cssSelector("header.origin-profile-header");
    protected static final By PROFILE_HEADER_USERNAME_LOCATOR = By.cssSelector("div.otktitle-page.origin-profile-header-username");
    protected static final By PROFILE_USER_REAL_NAME_LOCATOR = By.cssSelector("span.otktitle-3");
    protected static final By AVATAR_LOCATOR = By.cssSelector(".origin-profile-header .origin-profile-header-info-section img");

    protected static final By PROFILE_ANONYMOUS_LOCATOR = By.cssSelector(".origin-profile-anonymous");
    protected static final By LOGIN_CTA_BTN_LOCATOR = By.cssSelector("origin-profile-anonymous origin-cta-login");

    protected static final By PROFILE_HEADER_DROPDOWN_BUTTON_LOCATOR = By.cssSelector("span.origin-profile-header-controls-options-button");
    protected static final By PROFILE_HEADER_DROPDOWN_CONTAINER_LOCATOR = By.cssSelector("div#origin-profile-header-controls-options-menu");

    private static final By STATUS_TEXT_LOCATOR = By.cssSelector(".origin-profile-header-presence-area .otktitle-5");
    private static final By STATUS_ONLINE_LOCATOR = By.cssSelector("header.origin-profile-header .origin-profile-header-info-section .origin-profile-header-avatar.otkavatar-isonline");
    private static final By STATUS_AWAY_LOCATOR = By.cssSelector("header.origin-profile-header .origin-profile-header-info-section .origin-profile-header-avatar.otkavatar-isaway");
    private static final By STATUS_OFFLINE_LOCATOR = By.cssSelector("header.origin-profile-header .origin-profile-header-info-section .origin-profile-header-avatar.otkavatar-isoffline");
    private static final By STATUS_IN_GAME_LOCATOR = By.cssSelector("header.origin-profile-header .origin-profile-header-info-section .origin-profile-header-avatar.otkavatar-isingame");
    private static final By STATUS_BROADCASTING_LOCATOR = By.cssSelector("header.origin-profile-header .origin-profile-header-info-section .origin-profile-header-avatar.otkavatar-isbroadcasting");
    private static final By PROFILE_HEADER_INFO_SECTION_LOCATOR = By.cssSelector(".origin-profile-header-info-section");

    protected static final String DROPDOWN_ITEM_XPATH_TMPL = "//div[contains(@id, 'origin-profile-header-controls-options-menu')]//a[text()='%s']";
    protected static final By SEND_FRIEND_REQUEST_BUTTON_LOCATOR = By.xpath(
            "//A[contains(@class, 'origin-profile-header-controls-primary-button') and contains(text(), 'Send Friend Request')]");
    protected static final By FRIEND_REQUEST_SENT_BUTTON_LOCATOR = By.xpath(
            "//A[contains(@class, 'origin-profile-header-controls-primary-button') and contains(text(), 'Friend Request Sent')]");
    protected static final By ACCEPT_FRIEND_REQUEST_BUTTON_LOCATOR = By.xpath(
            "//A[contains(@class, 'origin-profile-header-controls-primary-button') and contains(text(), 'Accept Friend Request')]");

    protected static final String NAVIGATION_SECTION_XPATH = "//div[contains(@class,'origin-profile-navsection')]";
    protected static final String NAVIGATION_PILLS_XPATH = NAVIGATION_SECTION_XPATH + "//ul[contains(@class,'otknav-pills')]";
    protected static final By ACHIEVEMENTS_TAB_PILL_LOCATOR = By.xpath(NAVIGATION_PILLS_XPATH + "//li[contains(@class, 'otkpill')]//a[contains(text(), 'Achievements')]");
    protected static final By ACHIEVEMENTS_TAB_PILL_ACTIVE_LOCATOR = By.xpath(NAVIGATION_PILLS_XPATH + "//li[@class='otkpill otkpill-active']//a[contains(text(), 'Achievements')]");
    protected static final By GAMES_TAB_PILL_LOCATOR = By.xpath(NAVIGATION_PILLS_XPATH + "//li[contains(@class, 'otkpill')]//a[contains(text(), 'Games')]");
    protected static final String GAMES_TAB_PILL_LOCATOR_2 = NAVIGATION_PILLS_XPATH + "//li[contains(@class, 'otkpill')]//a[contains(text(), '%s')]";
    protected static final By GAMES_TAB_PILL_ACTIVE_LOCATOR = By.xpath(NAVIGATION_PILLS_XPATH + "//li[@class='otkpill otkpill-active']//a[contains(text(), 'Games')]");
    protected static final By FRIENDS_TAB_PILL_LOCATOR = By.xpath(NAVIGATION_PILLS_XPATH + "//li[contains(@class, 'otkpill')]//a[contains(text(), 'Friends')]");
    protected static final By FRIENDS_TAB_PILL_ACTIVE_LOCATOR = By.xpath(NAVIGATION_PILLS_XPATH + "//li[@class='otkpill otkpill-active']//a[contains(text(), 'Friends')]");
    protected static final By WISHLIST_TAB_PILL_LOCATOR = By.xpath(NAVIGATION_PILLS_XPATH + "//li[contains(@class, 'otkpill')]//a[contains(text(), 'Wishlist')]");
    protected static final By WISHLIST_TAB_PILL_ACTIVE_LOCATOR = By.xpath(NAVIGATION_PILLS_XPATH + "//li[@class='otkpill otkpill-active']//a[contains(text(), 'Wishlist')]");
    protected static final By ORIGIN_PROFILE_WISHLIST_SHARE_AREA = By.cssSelector(".origin-profile-wishlist-share-area");
    protected static final By ORIGIN_PROFILE_WISHLIST_EMPTY_HEADING = By.cssSelector(".origin-profile-wishlist-empty-heading");
    protected static final By ORIGIN_PROFILE_WISHLIST_GAMETILE = By.cssSelector(".origin-profile-wishlist-gametile");
    protected static final By ORIGIN_PROFILE_WISHLIST_EMPTY = By.cssSelector(".origin-profile-wishlist-empty");
    protected static final By ORIGIN_PROFILE_PRIVATE = By.cssSelector(".origin-profile-private");
    protected static final By FRIENDS_TAB_FIND_FRIENDS_LOCATOR = By.cssSelector(".header-text .find-friends-link");
    protected static final By FRIENDS_TAB_HEADER_LOCATOR = By.cssSelector(".friendtile-section .row-header");
    protected static final String GAMETILE_OFFERID_CSS_TMPL = "origin-profile-page-games-gametile[offerid='%s'] > .origin-tile";
    protected static final String BORDER_COLOR = "border-color";

    //Option text
    protected static final String IGNORE_FRIEND_REQUEST_AND_BLOCK_TEXT = "Ignore friend request and block";
    protected static final String UNFRIEND_AND_BLOCK_TEXT = "Unfriend and Block";
    protected static final String UNFRIEND_TEXT = "Unfriend";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ProfileHeader(WebDriver driver) {
        super(driver);
        boolean isClient = OriginClient.getInstance(driver).isQtClient(driver);
    }
    
    /**
    * Waits for Profile Page to load.
    */
    public void waitForProfilePage() {
        waitForPageToLoad();
        waitForAngularHttpComplete(30);

        // Wait for profile headeer to appear
        waitForAnyElementVisible(PROFILE_HEADER_LOCATOR);
    }

    /**
     * Verifies that Origin is displaying a 'Profile' page.
     *
     * @return true if a 'Profile' page is displayed, false otherwise
     */
    public boolean verifyOnProfilePage() {
        return waitIsElementVisible(PROFILE_HEADER_LOCATOR);
    }

    /**
     * Verifies that Origin is displaying a 'Sign In' page. Occurs when a
     * non-account user attempts to view a profile.
     *
     * @return true if a 'Sign In' page is displayed, false otherwise
     */
    public boolean verifyOnSignInPage() {
        return waitIsElementVisible(PROFILE_ANONYMOUS_LOCATOR);
    }

    /**
     * Click the 'Login' button. Should only be used in the browser where a user
     * can view the SPA anonymously
     */
    public void clickLoginCtaBtn() {
        waitForElementClickable(LOGIN_CTA_BTN_LOCATOR).click();
    }

    /**
     * Gets the real name of a user from their 'Profile' page.
     *
     * @return The real name of the user, or an empty String if their real name
     * is not set
     */
    public String getUserRealName() {
        List<WebElement> realNameElements;
        try {
            realNameElements = waitForAllElementsVisible(PROFILE_USER_REAL_NAME_LOCATOR, 30);
        } catch (TimeoutException e) {
            _log.debug("User does not have their real name shown on their profile page");
            return "";
        }

        String realName = "";
        for (WebElement element : realNameElements) {
            realName += element.getText() + " ";
        }
        return realName.trim();
    }

    /**
     * Open the dropdown menu at the 'Profile' header.
     */
    public void openDropdownMenu() {
        if (!isDropdownMenuOpened()) {
            waitForElementClickable(PROFILE_HEADER_DROPDOWN_BUTTON_LOCATOR).click();
        }
    }

    /**
     * Click an option at the dropdown menu of the 'Profile' header
     *
     * @param optionText The option to click on
     */
    public void selectDropdownOption(String optionText) {
        By locator = By.xpath(String.format(DROPDOWN_ITEM_XPATH_TMPL, optionText));
        openDropdownMenu();
        waitForElementClickable(locator).click();
    }

    /**
     * Verifies that there is a navigation section on the page.
     *
     * @return true if the navigation section exists, false otherwise
     */
    public boolean verifyNavigationSection() {
        return waitIsElementVisible(By.xpath(NAVIGATION_PILLS_XPATH));
    }

    /**
     * Opens the 'Achievements' tab of the 'Profile' page.
     */
    public void openAchievementsTab() {
        waitForElementVisible(ACHIEVEMENTS_TAB_PILL_LOCATOR).click();
    }

    /**
     * Verifies that the 'Achievements' tab is the active tab in the 'Profile'
     * page.
     *
     * @return true if the 'Achievements' tab is active, false otherwise
     */
    public boolean verifyAchievementsTabActive() {
        return waitIsElementVisible(ACHIEVEMENTS_TAB_PILL_ACTIVE_LOCATOR);
    }

    /**
     * Opens the 'Games' tab of the 'Profile' page.
     */
    public void openGamesTab() {
        waitForElementVisible(GAMES_TAB_PILL_LOCATOR).click();
        waitForGamesTabToLoad();
    }

    /**
     * Opens the 'Games' tab of the 'Profile' page given the text of 'Games'
     */
    public void openGamesTab(String myGamesText) {
        waitForElementVisible(By.xpath(String.format(GAMES_TAB_PILL_LOCATOR_2, myGamesText))).click();
        waitForGamesTabToLoad(myGamesText);
    }

    /**
     * Clicks the 'Send Friend Request' button.
     */
    public void clickSendFriendRequest() {
        waitForElementClickable(SEND_FRIEND_REQUEST_BUTTON_LOCATOR).click();
    }

    /**
     * Clicks the 'Accept Friend Request' button.
     */
    public void clickAcceptFriendRequest() {
        waitForElementClickable(ACCEPT_FRIEND_REQUEST_BUTTON_LOCATOR).click();
    }

    /**
     * Verifies that the 'Games' tab is the active tab in the 'Profile' page.
     *
     * @return True if the 'Games' tab is active. False otherwise
     */
    public boolean verifyGamesTabActive() {
        return waitIsElementVisible(GAMES_TAB_PILL_ACTIVE_LOCATOR);
    }

    /**
     * Opens the 'Friends' tab of the 'Profile' page.
     */
    public void openFriendsTab() {
        waitForElementVisible(FRIENDS_TAB_PILL_LOCATOR).click();
    }

    /**
     * Verifies that the 'Friends' tab is the active tab in the 'Profile' page.
     *
     * @return true if the 'Friends' tab is active, false otherwise
     */
    public boolean verifyFriendsTabActive() {
        return waitIsElementVisible(FRIENDS_TAB_PILL_ACTIVE_LOCATOR);
    }

    /**
     * Opens the 'Wishlist' tab of the 'Profile' page
     */
    public void openWishlistTab() {
        //Refreshes added in case the wishlist page loads unsuccessfully (can happen multiple times)
        if (!waitIsElementVisible(WISHLIST_TAB_PILL_LOCATOR)) {
            driver.navigate().refresh();
            sleep(5000); //wait for slow refreshes
        }
        if (!waitIsElementVisible(WISHLIST_TAB_PILL_LOCATOR)) {
            driver.navigate().refresh();
            sleep(5000);
        }

        waitForElementClickable(WISHLIST_TAB_PILL_LOCATOR).click();
        waitForWishlistTabToLoad();
    }

    /**
     * Opens the 'Wishlist' tab of the 'Profile' page when the profile is
     * private.
     */
    public void openWishlistTabPrivate() {
        //Refreshes added in case the wishlist page loads incorrectly (can happen multiple times)
        if (!waitIsElementVisible(WISHLIST_TAB_PILL_LOCATOR)) {
            driver.navigate().refresh();
            sleep(5000); //wait for slow refreshes
        }
        if (!waitIsElementVisible(WISHLIST_TAB_PILL_LOCATOR)) {
            driver.navigate().refresh();
            sleep(5000);
        }
        waitForElementClickable(WISHLIST_TAB_PILL_LOCATOR).click();
        int retries = 4;
        int i;
        for (i = 0; i < retries; i++) {
            if (waitIsElementVisible(ORIGIN_PROFILE_PRIVATE)) {
                return;
            } else {
                driver.navigate().refresh();
            }
        }
        if (i == retries) {
            throw new RuntimeException("failed to display that a user's wishlist is private");
        }
        waitForElementVisible(ORIGIN_PROFILE_PRIVATE);
    }

    /**
     * Verifies that the 'Wishlist' tab is the active tab in the 'Profile' page.
     *
     * @return true if the 'Wishlist' tab is active, false otherwise
     */
    public boolean verifyWishlistActive() {
        return waitIsElementVisible(WISHLIST_TAB_PILL_ACTIVE_LOCATOR);
    }

    /**
     * Waits for the 'Wishlist' tab to load.
     *
     */
    public void waitForWishlistTabToLoad() {
        //Refreshes added in case the wishlist page loads unsuccessfully (can happen multiple times)
        for (int i = 0; i < 3; i++) {
            if (!waitIsElementVisible(WISHLIST_TAB_PILL_ACTIVE_LOCATOR)) {
                refreshAndSleep(driver);
            } else {
                break;
            }
        }
        waitForAnyElementVisible(By.cssSelector(".origin-profile-wishlist-share-area"), By.cssSelector(".origin-profile-wishlist-empty-heading"),
                By.cssSelector(".origin-profile-wishlist-gametile"), By.cssSelector(".origin-profile-wishlist-empty"));
    }

    /**
     * Select 'Unfriend' option from the dropdown menu.
     */
    public void selectUnfriendFromDropDownMenu() {
        openDropdownMenu();
        selectDropdownOption(UNFRIEND_TEXT);
    }

    /**
     * Waits for the 'Achievements' tab to load.
     *
     */
    public void waitForAchievementsTabToLoad() {
        waitForElementVisible(ACHIEVEMENTS_TAB_PILL_ACTIVE_LOCATOR); // Waits till the element is visible
        waitForAnyElementVisible(By.cssSelector(".origin-achievements-nogames-title"), By.cssSelector(".origin-achievements-overall-title"), By.cssSelector(".origin-achievements-private"));
    }

    /**
     * Waits for the 'Friends' tab to load.
     *
     * @return true if the 'Friends' tab is loaded, false otherwise
     */
    public void waitForFriendsTabToLoad() {
        waitForElementVisible(FRIENDS_TAB_PILL_ACTIVE_LOCATOR); // Waits till the element is visible
        waitForAnyElementVisible(FRIENDS_TAB_FIND_FRIENDS_LOCATOR, FRIENDS_TAB_HEADER_LOCATOR);
    }

    /**
     * Waits for the 'Games' tab to load.
     *
     * @return true if the 'Games' tab is loaded, false otherwise
     */
    public void waitForGamesTabToLoad() {
        waitForElementVisible(GAMES_TAB_PILL_ACTIVE_LOCATOR); // Waits till the element is visible
        waitForAnyElementVisible(By.cssSelector("h2.otktitle-3.row-header"), By.cssSelector("h3.otktitle-2"), By.cssSelector(".origin-profile-games-gametile"));
    }

    /**
     * Waits for the 'Games' tab to load.
     *
     * @return true if the 'Games' tab is loaded, false otherwise
     */
    public void waitForGamesTabToLoad(String myGamesText) {
        waitForElementVisible(By.xpath(String.format(GAMES_TAB_PILL_LOCATOR_2, myGamesText))); // Waits till the element is visible
        waitForAnyElementVisible(By.cssSelector("h2.otktitle-3.row-header"), By.cssSelector("h3.otktitle-2"), By.cssSelector(".origin-profile-games-gametile"));
    }

    /**
     * Verify profile header displays the expected username.
     *
     * @param expectedName The expected name to check against
     * @return true if username matches, false otherwise
     */
    public boolean verifyUsername(String expectedName) {
        try {
            return waitForElementVisible(PROFILE_HEADER_USERNAME_LOCATOR)
                    .getText()
                    .trim()
                    .equals(expectedName);
        } catch (StaleElementReferenceException e) {
            return waitForElementVisible(PROFILE_HEADER_USERNAME_LOCATOR)
                    .getText()
                    .trim()
                    .equals(expectedName);
        }
    }

    /**
     * Verifies that the given Strings exist in the profile dropdown.
     *
     * @param strings The Strings to match in the dropdown
     * @return true if the dropdown contains all the given Strings
     */
    public boolean verifyDropdownOptions(String... strings) {
        openDropdownMenu();
        return Arrays.stream(strings)
                .map(s -> waitIsElementVisible(
                By.xpath(String.format(DROPDOWN_ITEM_XPATH_TMPL, s))))
                .allMatch(b -> b);
    }

    /**
     * Verify the status presence is visible
     *
     * @return true if the status presence is visible, false otherwise
     */
    public boolean verifyPresenceStatusVisible() {
        return waitIsElementVisible(STATUS_TEXT_LOCATOR);
    }

    /**
     * Verify dropdown menu at the 'Profile' header has been opened.
     *
     * @return true if the dropdown menu on the 'Profile' header is open, false
     * otherwise
     */
    public boolean isDropdownMenuOpened() {
        return isElementVisible(PROFILE_HEADER_DROPDOWN_CONTAINER_LOCATOR);
    }

    /**
     * Verify 'Send Friend Request' button is now visible.
     *
     * @return true if the 'Send Friend Request' button is visible, false
     * otherwise
     */
    public boolean verifySendFriendRequestButtonVisible() {
        return waitIsElementVisible(SEND_FRIEND_REQUEST_BUTTON_LOCATOR);
    }

    /**
     * Verify that the 'Friend Request Sent' indicator is visible.
     *
     * @return true if a friend request has been sent, false otherwise
     */
    public boolean verifyFriendRequestSentVisible() {
        return waitIsElementVisible(FRIEND_REQUEST_SENT_BUTTON_LOCATOR);
    }

    /**
     * Verify the 'Profile Status' text matches the expected String (ignoring
     * case).
     *
     * @param expected The expected String
     * @return true if the text matches the expected String, false otherwise
     */
    public boolean verifyProfileStatusTextMatches(String expected) {
        WebElement presenceText = waitForElementVisible(STATUS_TEXT_LOCATOR);
        return presenceText.getText().equalsIgnoreCase(expected);
    }

    /**
     * Verify the 'Profile Status' text contains the expected String.
     *
     * @param expected The expected String
     * @return true if the text contains the expected String, false otherwise
     */
    public boolean verifyProfileStatusTextContains(String expected) {
        WebElement presenceText = waitForElementVisible(STATUS_TEXT_LOCATOR);
        return presenceText.getText().contains(expected);
    }

    /**
     * Verify that the profile avatar shows an 'Online' presence.
     *
     * @return true if the avatar shows an 'Online' presence, false otherwise
     */
    public boolean verifyPresenceOnline() {
        return isElementVisible(STATUS_ONLINE_LOCATOR) && verifyProfileStatusTextMatches("ONLINE");
    }

    /**
     * Verify that the profile avatar and text show an 'Away' presence.
     *
     * @return true if the avatar shows an 'Away' presence, false otherwise
     */
    public boolean verifyPresenceAway() {
        return isElementVisible(STATUS_AWAY_LOCATOR) && verifyProfileStatusTextMatches("AWAY");
    }

    /**
     * Verify that the profile avatar is set to 'Offline' and the text is
     * 'Invisible'. This can only occur when viewing the user's own profile.
     *
     * @return true if the avatar shows an 'Invisible' presence, false otherwise
     */
    public boolean verifyPresenceInvisible() {
        return isElementVisible(STATUS_OFFLINE_LOCATOR) && verifyProfileStatusTextMatches("INVISIBLE");
    }

    /**
     * Verify that the profile avatar shows an 'Offline' presence.
     *
     * @return true if the avatar shows an 'Offline' presence, false otherwise
     */
    public boolean verifyPresenceOffline() {
        return isElementVisible(STATUS_OFFLINE_LOCATOR) && verifyProfileStatusTextMatches("OFFLINE");
    }

    /**
     * Verify that the profile avatar shows an 'In-Game' presence.
     *
     * @return true if the avatar shows an 'In-Game' presence, false otherwise
     */
    public boolean verifyPresenceInGame() {
        return isElementVisible(STATUS_IN_GAME_LOCATOR) && verifyProfileStatusTextContains("PLAYING");
    }

    /**
     * Verify that the profile avatar shows a 'Broadcasting' presence.
     *
     * @return true if the avatar shows a 'Broadcasting' presence, false
     * otherwise
     */
    public boolean verifyPresenceBroadcasting() {
        return isElementVisible(STATUS_BROADCASTING_LOCATOR) && verifyProfileStatusTextMatches("BROADCASTING");
    }

    /**
     * Verify if the user's presence is showing in green color when he is online
     *
     * @return true if the presence is showing in green, false otherwise
     */
    public boolean verifyOnlineStatusColor() {
        String onlineColor = Color.fromString(waitForElementVisible(STATUS_ONLINE_LOCATOR).getCssValue(BORDER_COLOR)).asHex();
        return StringHelper.containsIgnoreCase(onlineColor, OriginClientData.ORIGIN_ONLINE_STATUS_COLOUR);
    }

    /**
     * Verify if the user's presence is showing in red color when he is away
     *
     * @return true if the presence is showing in red, false otherwise
     */
    public boolean verifyAwayStatusColor() {
        String awayColor = Color.fromString(waitForElementVisible(STATUS_AWAY_LOCATOR).getCssValue(BORDER_COLOR)).asHex();
        return StringHelper.containsIgnoreCase(awayColor, OriginClientData.ORIGIN_AWAY_STATUS_COLOUR);
    }

    /**
     * Verify if the user's presence is showing in grey color when he is
     * invisible
     *
     * @return true if the presence is showing in grey, false otherwise
     */
    public boolean verifyInvisibleStatusColor() {
        String invisibleColor = Color.fromString(waitForElementVisible(STATUS_OFFLINE_LOCATOR).getCssValue(BORDER_COLOR)).asHex();
        return StringHelper.containsIgnoreCase(invisibleColor, OriginClientData.ORIGIN_INVISIBLE_STATUS_COLOUR);
    }

    /**
     * Verify if the user's presence is showing in blue color when he is in game
     *
     * @return true if the presence is showing in blue, false otherwise
     */
    public boolean verifyInGameStatusColor() {
        String inGameColor = Color.fromString(waitForElementVisible(STATUS_IN_GAME_LOCATOR).getCssValue(BORDER_COLOR)).asHex();
        return StringHelper.containsIgnoreCase(inGameColor, OriginClientData.ORIGIN_IN_GAME_STATUS_COLOUR);
    }

    /**
     * Verify if the user's presence is showing in grey color when he is offline
     *
     * @return true if the presence is showing in grey, false otherwise
     */
    public boolean verifyOfflineStatusColor() {
        String offlineColor = Color.fromString(waitForElementVisible(STATUS_OFFLINE_LOCATOR).getCssValue(BORDER_COLOR)).asHex();
        return StringHelper.containsIgnoreCase(offlineColor, OriginClientData.USER_DOT_PRESENCE_OFFLINE_COLOUR);
    }

    /**
     * Gets the src for the 'Avatar' in the profile page. This should be the URL
     * to the avatar image.
     *
     * @return The avatar source, or null if there is none
     */
    public String getAvatarSrc() {
        WebElement avatarItem = waitForElementVisible(AVATAR_LOCATOR);
        return avatarItem.getAttribute("src");
    }

    /**
     * Verify that the current profile is private.
     *
     * @return true if the currently viewed profile is private, false otherwise
     */
    public boolean verifyProfileIsPrivate() {
        return isElementVisible(By.cssSelector(".origin-profile-private"));
    }

    /**
     * Verify the current profile's user is blocked.
     *
     * @return true if the currently viewed profile is a user that has been
     * blocked, false otherwise
     */
    public boolean verifyUserIsBlocked() {
        return waitIsElementVisible(By.cssSelector(".origin-profile-blocked"), 5);
    }

    /**
     * Select 'Ignore Friend Request and Block' option from the dropdown menu.
     */
    public void selectIgnoreFriendRequestAndBlockFromDropDownMenu() {
        if (isDropdownMenuOpened()) {
            openDropdownMenu();
        }
        selectDropdownOption(IGNORE_FRIEND_REQUEST_AND_BLOCK_TEXT);
    }

    /**
     * Checks if the given option from the dropdown menu is present.
     *
     * @return true if present, false otherwise
     */
    public boolean verifyDropdownOptionVisible(String optionText) {
        By locator = By.xpath(String.format(DROPDOWN_ITEM_XPATH_TMPL, optionText));
        openDropdownMenu();
        return waitIsElementVisible(locator, 5);
    }

    /**
     * Select 'Unfriend and Block' option from the dropdown menu.
     */
    public void selectUnfriendAndBlockFromDropDownMenu() {
        openDropdownMenu();
        selectDropdownOption(UNFRIEND_AND_BLOCK_TEXT);
    }
    
    /**
     * Verify that the 'Edit on ea.com' CTA button is the only visible in the profile header
     * info section
     */
    public boolean verifyEditOnEaCTAOnlyButtonVisible() {
        WebElement originProfileHeaderInfoSection = driver.findElement(PROFILE_HEADER_INFO_SECTION_LOCATOR);
        List<WebElement> buttonsList = originProfileHeaderInfoSection.findElements(By.cssSelector("a")); // get all the buttons in the profile header info section
        int numVisibleButtons = 0;
        // Checks how many buttons in the profile header info section are enabled, displayed, and don't have empty text
        // One of the buttons returned in buttonsList returns "" as its text but is displayed and enabled
        for (int i = 0; i < buttonsList.size(); i++) {
            WebElement button = buttonsList.get(i);
            String buttonText = button.getText();
            if (button.isDisplayed() && button.isEnabled() && !StringHelper.nullOrEmpty(buttonText)) {
                numVisibleButtons++;
            }
        }
        return numVisibleButtons == 1;
    }
    
    /**
     * Check if an entitlement is visible in the user 'Games' tab profile given
     * an offer ID.
     *
     * @param offerID The offer ID of the entitlement to check for visible
     * @return true if a visible entitlement is found matching the given offer
     * ID
     */
    public boolean isGameTileVisibleByOfferID(String offerID) {
        return waitIsElementVisible(By.cssSelector(String.format(GAMETILE_OFFERID_CSS_TMPL, offerID)));
    }
}