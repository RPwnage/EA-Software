package com.ea.originx.automation.lib.pageobjects.social;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.common.ContextMenu;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.utils.Waits;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import java.util.ArrayList;
import java.util.List;
import java.util.NoSuchElementException;

import static com.ea.originx.automation.lib.resources.OriginClientData.USER_DOT_PRESENCE_OFFLINE_COLOUR;

/**
 * The 'Social Hub' component object.
 *
 * @author sng
 * @author glivingstone
 */
public class SocialHub extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(SocialHub.class);

    protected static final By SOCIAL_HUB_LOCATOR = By.xpath("//div[contains(@class,'l-origin-social-hub-area')]");
    protected static final String SOCIAL_HUB_AREA_HEADER_CSS = ".l-origin-social-hub-area .origin-social-hub-area-header";
    protected static final By SOCIAL_HUB_AREA_HEADER_LOCATOR = By.cssSelector(SOCIAL_HUB_AREA_HEADER_CSS);
    protected static final By SOCIAL_HUB_MENU_DROPDOWN_LOCATOR = By.cssSelector(SOCIAL_HUB_AREA_HEADER_CSS + " .origin-social-hub-area-options-menu-icon");
    protected static final String SOCIAL_HUB_POPOUT_WINDOW_URL = ".*social-hub((?!oigContext).)*";

    //Social Hub drop down menu options
    protected static final String SOCIAL_HUB_MENUOPTION_MINIMIZE_WINDOW = "Minimize Window";
    protected static final String SOCIAL_HUB_MENUOPTION_OPEN_IN_NEW_WINDOW = "Open In A New Window";
    protected static final String SOCIAL_HUB_MENUOPTION_CHAT_SETTINGS = "Chat Settings";
    protected static final String SOCIAL_HUB_MENUOPTION_CLOSE_ALL_CHAT_TABS = "Close All Chat Tabs";

    // Social Hub offline message title and content
    protected static final String SOCIAL_HUB_OFFLINE_CTA_MESSAGE_CSS = "#social div.origin-offline-cta div.origin-message";
    protected static final By OFFLINE_MESSAGE_TITLE_LOCATOR = By.cssSelector(SOCIAL_HUB_OFFLINE_CTA_MESSAGE_CSS + " .otktitle-2.origin-message-title");
    protected static final By OFFLINE_MESSAGE_CONTENT_LOCATOR = By.cssSelector(SOCIAL_HUB_OFFLINE_CTA_MESSAGE_CSS + " .otktitle-3.origin-message-content");

    protected static final String OFFLINE_MESSAGE_TITLE = "You're offline";

    // Check the prefix of the entire message content message as the entire message is long and are broken into parts with hyperlink reference(s)
    protected static final String[] OFFLINE_MESSAGE_CONTENT_KEYWORDS = {"go", "online"};

    // Friends Sections
    protected static final By FRIENDS_ROSTER_LOCATOR = By.xpath("//div[@class=origin-social-hub-roster-area-list-roster]");
    protected static final By INCOMING_FRIEND_REQUESTS_LOCATOR = By.cssSelector("origin-social-hub-roster-section[filter='INCOMING'] .origin-social-hub-roster-item.origin-social-hub-roster-friend.origin-social-hub-roster-pendingitem");
    protected static final By FRIEND_REQUEST_ACCEPT_BUTTON_LOCATOR = By.cssSelector(".origin-social-hub-roster-pendingitem-accept-icon .otkicon.otkicon-checkcircle");
    protected static final By FRIEND_REQUEST_DECLINE_BUTTON_LOCATOR = By.cssSelector(".origin-social-hub-roster-pendingitem-decline-icon .otkicon.otkicon-closecircle");
    protected static final By ALL_FRIEND_REQUEST_AVATARS = By.cssSelector(".otkavatar.origin-social-hub-roster-friend-link > img");

    protected static final By FRIENDS_SEARCHBAR_CLEAR_BUTTON_LOCATOR = By.cssSelector("span.origin-social-hub-roster-area-friends-clear-filter i.otkicon.otkicon-closecircle");
    protected static final By FRIENDS_SEARCHBAR_INPUT_LOCATOR = By.cssSelector("div.origin-social-hub-roster-area-friends-filter-group-field input");
    protected static final By FRIENDS_ADD_CONTACTS_BUTTON_LOCATOR = By.cssSelector(".origin-social-hub-roster-area-friends-add-friends .otkicon-addcontact");

    protected static final String NO_FRIENDS_MESSAGE_CSS = ".origin-social-hub-roster-area-list.origin-social-hub-roster-area-list-nofriends";
    protected static final By NO_FRIENDS_MESSAGE_HEADER_LOCATOR = By.cssSelector(NO_FRIENDS_MESSAGE_CSS + " .otktitle-2");
    protected static final By NO_FRIENDS_MESSAGE_BODY_LOCATOR = By.cssSelector(NO_FRIENDS_MESSAGE_CSS + " .otkc");
    protected static final By FIND_FRIENDS_BUTTON_LOCATOR = By.cssSelector(NO_FRIENDS_MESSAGE_CSS + " .origin-social-hub-cta .origin-social-hub-cta-inner-wrapper .otkbtngroup .otkbtngroup-item .otkbtn");

    protected static final String USER_PRESENCE_CSS = SOCIAL_HUB_AREA_HEADER_CSS + " .otkdropdown #origin-social-hub-area-presence-trigger span";
    protected static final By USER_PRESENCE_LOCATOR = By.cssSelector(USER_PRESENCE_CSS);
    protected static final String USER_PRESENCE_CONTEXT_MENU_CSS = "#origin-social-hub-area-presence-menu";
    protected static final By USER_PRESENCE_CONTEXT_MENU_LOCATOR = By.cssSelector(USER_PRESENCE_CONTEXT_MENU_CSS);
    protected static final By USER_PRESENCE_CONTEXT_MENU_OPTIONS_LOCATOR = By.cssSelector(USER_PRESENCE_CONTEXT_MENU_CSS + " .otkmenu.otkmenu-context > li > a");
    protected static final By USER_PRESENCE_DOT_LOCATOR = By.cssSelector(".origin-social-hub-area-header .origin-social-hub-area-presence .otkpresence");
    protected static final By USER_PRESENCE_CONTEXT_MENU_VISIBLE_LOCATOR = By.cssSelector(USER_PRESENCE_CONTEXT_MENU_CSS + ".otkcontextmenu-isvisible");

    protected static final String AREA_VIEWPORT_CSS = ".origin-social-hub-roster-area-viewport";
    protected static final String INCOMING_SECTION_CSS = AREA_VIEWPORT_CSS + " origin-social-hub-roster-section[filter='INCOMING'] .origin-social-hub-section-viewport";
    protected static final String OUTGOING_SECTION_CSS = AREA_VIEWPORT_CSS + " origin-social-hub-roster-section[filter='OUTGOING'] .origin-social-hub-section-viewport";
    protected static final String FRIEND_SECTION_CSS = AREA_VIEWPORT_CSS + " origin-social-hub-roster-section[filter='FRIENDS'] .origin-social-hub-section-viewport";
    protected static final By INCOMING_SECTION_LOCATOR = By.cssSelector(INCOMING_SECTION_CSS);
    protected static final By OUTGOING_SECTION_LOCATOR = By.cssSelector(OUTGOING_SECTION_CSS);
    protected static final By FRIEND_SECTION_LOCATOR = By.cssSelector(FRIEND_SECTION_CSS);
    protected static final By SOCIAL_HUB_MINIMIZE_ICON = By.cssSelector(SOCIAL_HUB_AREA_HEADER_CSS + " i.origin-social-hub-minimize_button");
    
    protected static final By COLLAPSED_INCOMING_SECTION_LOCATOR = By.cssSelector(AREA_VIEWPORT_CSS + " origin-social-hub-roster-section[filter='INCOMING'] .origin-social-hub-section-downarrow-rotate");
    protected static final By COLLAPSED_OUTGOING_SECTION_LOCATOR = By.cssSelector(AREA_VIEWPORT_CSS + " origin-social-hub-roster-section[filter='OUTGOING'] .origin-social-hub-section-downarrow-rotate");
    protected static final By COLLAPSED_FRIENDS_SECTION_LOCATOR = By.cssSelector(AREA_VIEWPORT_CSS + " origin-social-hub-roster-section[filter='FRIENDS'] .origin-social-hub-section-downarrow-rotate");
    protected static final By INCOMING_EXPANSION_HEADER_LOCATOR = By.cssSelector(AREA_VIEWPORT_CSS + " origin-social-hub-roster-section[filter='INCOMING'] .origin-social-hub-section-downarrow.otkicon-downarrow");
    protected static final By OUTGOING_EXPANSION_HEADER_LOCATOR = By.cssSelector(AREA_VIEWPORT_CSS + " origin-social-hub-roster-section[filter='OUTGOING'] .origin-social-hub-section-downarrow.otkicon-downarrow");
    protected static final By FRIENDS_EXPANSION_HEADER_LOCATOR = By.cssSelector(AREA_VIEWPORT_CSS + " origin-social-hub-roster-section[filter='FRIENDS'] .origin-social-hub-section-downarrow.otkicon-downarrow");

    protected static final By ALL_INCOMING_USERNAMES_LOCATOR = By.cssSelector(INCOMING_SECTION_CSS + " h3.otktitle-4");
    protected static final By ALL_OUTGOING_USERNAMES_LOCATOR = By.cssSelector(OUTGOING_SECTION_CSS + " h3.otktitle-4");
    protected static final By ALL_FRIEND_USERNAMES_LOCATOR = By.cssSelector(FRIEND_SECTION_CSS + " h3.origin-social-hub-roster-friend-originId");
    protected static final By ALL_FRIEND_REQUESTS_LOCATOR = By.cssSelector(".origin-social-hub-roster-item.origin-social-hub-roster-friend.origin-social-hub-roster-pendingitem");
    protected static final By ALL_FRIEND_TILES_LOCATOR = By.cssSelector(FRIEND_SECTION_CSS + ".origin-social-hub-roster-item.origin-social-hub-roster-friend");
    protected static final String FRIEND_TILE_BY_USERNAME_XPATH_TMPL = "//div[contains(@class, 'origin-social-hub-roster-item origin-social-hub-roster-friend') and .//strong[contains(text(), '%s')]]";

    protected static final By FRIENDS_STATUS_TOASTY_LOCATOR = By.xpath(".//h1[contains(@ng-show, 'toast.title')]");
    protected static final By ALL_FRIEND_REQUEST_USERNAMES = By.cssSelector(".origin-social-hub-roster-friend-inner-wrapper strong");
    private static final String MINIMIZE = "Minimize";

    // Close button for Social hub Inside Oig
    protected static final By CLOSE_BUTTON_LOCATOR = By.xpath("//*[@id='buttonContainer']/*[@id='btnClose']");

    /**
     * Enum for 'Social Hub' presence, with expected text String.
     */
    private static final String[] PRESENCE_TEXT = {"In Game", "Online", "Away", "Invisible", "Offline"};

    /**
     * The enum for presence type.
     */
    public enum PresenceType {
        //The order for each index is significant because it corresponds to the
        //order in which friends should appear when displayed in the social hub.
        //i.e In Game friends are shown first, followed by online friends, etc
        INGAME(0), ONLINE(1), AWAY(2), INVISIBLE(3), OFFLINE(4);

        private final int index;

        private PresenceType(int index) {
            this.index = index;
        }

        private String getPresenceText() {
            return PRESENCE_TEXT[this.index];
        }
    }

    /**
     * Basic constructor for the 'Social Hub' object.
     *
     * @param driver Selenium WebDriver
     */
    public SocialHub(WebDriver driver) {
        super(driver);
    }

    /**
     * Minimizes the 'Social Hub'.
     */
    public void minimizeSocialHub() {
        boolean leftClick = true;
        new ContextMenu(driver, SOCIAL_HUB_MENU_DROPDOWN_LOCATOR, leftClick).selectItemContainingIgnoreCase(MINIMIZE);
    }

    /**
     * Minimize 'Social Hub' if minimizable option is present in context menu.
     */
    public void minimizeSocialHubIfMinimizable() {
        boolean leftClick = true;
        ContextMenu contextMenu = new ContextMenu(driver, SOCIAL_HUB_MENU_DROPDOWN_LOCATOR, leftClick);
        contextMenu.openDropdown();
        if (contextMenu.getItem(MINIMIZE) != null) {
            contextMenu.selectItem(MINIMIZE);
        }
    }

    /**
     * Check if the incoming section is collapsed.
     *
     * @return true if the incoming section in the 'Social Hub' is collapsed,
     * false otherwise
     */
    public boolean isIncomingCollapsed() {
        return isElementVisible(COLLAPSED_INCOMING_SECTION_LOCATOR);
    }

    /**
     * Check if the outgoing section is collapsed.
     *
     * @return true if the outgoing section in the 'Social Hub' is collapsed,
     * false otherwise
     */
    public boolean isOutgoingCollapsed() {
        return isElementVisible(COLLAPSED_OUTGOING_SECTION_LOCATOR);
    }

    /**
     * Check if the 'Friends' section is collapsed.
     *
     * @return true if the 'Friends' section in the 'Social Hub' is collapsed,
     * false otherwise
     */
    public boolean isFriendsCollapsed() {
        return isElementVisible(COLLAPSED_FRIENDS_SECTION_LOCATOR);
    }

    /**
     * Click on the incoming header to expand or collapse the 'Friends' section.
     */
    public void clickIncomingHeader() {
        waitForElementVisible(INCOMING_EXPANSION_HEADER_LOCATOR).click();
    }

    /**
     * Click on the outgoing header to expand or collapse the 'Friends' section.
     */
    public void clickOutgoingHeader() {
        waitForElementVisible(OUTGOING_EXPANSION_HEADER_LOCATOR).click();
    }

    /**
     * Click on the 'Friends' header to expand or collapse the 'Friends' section.
     */
    public void clickFriendsHeader() {
        waitForElementVisible(FRIENDS_EXPANSION_HEADER_LOCATOR).click();
    }
    
    /**
     * Clicks the icon which minimizes the social hub
     */
    public void clickMinimizeSocialHub() {
        waitForElementClickable(SOCIAL_HUB_MINIMIZE_ICON).click();
    }

    /**
     * Returns all friends visible.
     *
     * @return All friends with type origin-social-hub-roster-friend
     */
    public List<Friend> getAllFriends() {
        List<String> friendsNameList = getAllFriendUsernames();
        List<Friend> friendsList = new ArrayList<>();
        for (String friendName : friendsNameList) {
            friendsList.add(getFriend(friendName));
        }
        return friendsList;
    }

    /**
     * Gets the usernames of all the friends in the 'Social Hub'.
     *
     * @return A list of each friend's username
     */
    public List<String> getAllFriendUsernames() {
        List<WebElement> usernameElements = waitForAllElementsVisible(ALL_FRIEND_USERNAMES_LOCATOR);
        List<String> allUsernames = new ArrayList<>(usernameElements.size());

        for (WebElement username : usernameElements) {
            allUsernames.add(username.getText());
        }
        return allUsernames;
    }

    /**
     * Return a particular 'Friend' object.
     *
     * @param locator Friend section to search ('Friend Request Received',
     * 'Friend Requests Sent', or 'Friends')
     * @param userName Username of friend
     * @return 'Friend' object
     */
    private Friend getFriend(By locator, String userName) {
        _log.debug("getting friend: " + userName);
        this.waitForPageToLoad();
        this.waitForAngularHttpComplete();
        WebElement friendSection = waitForElementPresent(locator);
        try {
            final By friendLocator = By.xpath(String.format(FRIEND_TILE_BY_USERNAME_XPATH_TMPL, userName));
            friendSection.findElement(friendLocator);
            return new Friend(driver, friendLocator);
        } catch (NoSuchElementException e) {
            return null;
        }
    }

    /**
     * Verify friend's presence is 'Away'.
     *
     * @param username Username of a friend
     * @return true if friend's presence is 'Away', false otherwise
     */
    public boolean verifyFriendAway(String username) {
        return getFriend(username).verifyPresenceAway();
    }

    /**
     * Verify friend's presence is 'Online'.
     *
     * @param username Username of a friend
     * @return true if friend's presence is 'Online', false otherwise
     */
    public boolean verifyFriendOnline(String username) {
        return getFriend(username).verifyPresenceOnline();
    }

    /**
     * Verify friend's presence is 'Offline'.
     *
     * @param username Username of a friend
     * @return true if friend's presence is 'Offline', false otherwise
     */
    public boolean verifyFriendOffline(String username) {
        return getFriend(username).verifyPresenceOffline();
    }

    /**
     * Verify friend's presence is 'In Game'.
     *
     * @param username Username of friend
     * @return true if friend's presence is 'In Game', false otherwise
     */
    public boolean verifyFriendInGame(String username) {
        return getFriend(username).verifyPresenceInGame();
    }

    /**
     * Return a 'Friend' object from the 'Friend Requests Received' section.
     *
     * @param userName Username of friend
     * @return 'Friend' object
     */
    public Friend getFriendRequestReceived(String userName) {
        return getFriend(INCOMING_SECTION_LOCATOR, userName);
    }

    /**
     * Return a 'Friend' object from the 'Friend Requests Sent' section.
     *
     * @param userName Username of friend
     * @return 'Friend' object
     */
    public Friend getFriendRequestSent(String userName) {
        return getFriend(OUTGOING_SECTION_LOCATOR, userName);
    }

    /**
     * Return a 'Friend' object from the 'Friends' section.
     *
     * @param userName Username of friend
     * @return Friend Object
     */
    public Friend getFriend(String userName) {
        return getFriend(FRIEND_SECTION_LOCATOR, userName);
    }

    /**
     * Return any friend object from any of the 3 sections ('Friend Request
     * Received', 'Friend Requests Sent', or 'Friends').
     *
     * @param userName Username of friend
     * @return 'Friend' object
     */
    public Friend getAnyFriend(String userName) {
        Friend friend = getFriend(userName);
        if (friend == null) {
            friend = getFriendRequestSent(userName);
            if (friend == null) {
                friend = getFriendRequestReceived(userName);
            }
        }
        return friend;
    }

    /**
     * Accepts an incoming friend request from the specified friend.
     *
     * @param username The username whose friend request should be accepted
     */
    public void acceptIncomingFriendRequest(String username) {
        List<WebElement> allPendingFriends = waitForAllElementsVisible(INCOMING_FRIEND_REQUESTS_LOCATOR);
        WebElement friend = getFriendRequestElementWithText(allPendingFriends, username);
        waitForChildElementPresent(friend, FRIEND_REQUEST_ACCEPT_BUTTON_LOCATOR).click();
    }

    /**
     * Declines an incoming friend request from the specified friend.
     *
     * @param username The username whose friend request should be declined
     */
    public void declineIncomingFriendRequest(String username) {
        List<WebElement> allPendingFriends = waitForAllElementsVisible(INCOMING_FRIEND_REQUESTS_LOCATOR);
        WebElement friend = getFriendRequestElementWithText(allPendingFriends, username);
        waitForChildElementPresent(friend, FRIEND_REQUEST_DECLINE_BUTTON_LOCATOR).click();
    }

    /**
     * Enters text into the friends search bar.
     *
     * @param searchText The search text to enter
     */
    public void searchFriendsByText(String searchText) {
        _log.debug("searching by: " + searchText);
        WebElement searchBar = waitForElementClickable(FRIENDS_SEARCHBAR_INPUT_LOCATOR);
        searchBar.clear();
        searchBar.sendKeys(searchText);
        waitForAngularHttpComplete();
    }

    /**
     * Clears the text from the friends search bar.
     *
     * @param useButton true if the search text should be cleared using the X
     * button. False if it should be cleared manually.
     */
    public void clearSearchBarText(boolean useButton) {
        if (useButton) {
            _log.debug("click clear button");
            waitForElementClickable(FRIENDS_SEARCHBAR_CLEAR_BUTTON_LOCATOR).click();
        } else {
            _log.debug("clear input");
            waitForElementClickable(FRIENDS_SEARCHBAR_INPUT_LOCATOR).clear();
        }
        waitForAngularHttpComplete();
    }

    /**
     * Clicks on 'Find Friends' button, which opens a 'Search for friends'
     * dialog.
     */
    public void clickFindFriendsButton() {
        waitForElementClickable(FIND_FRIENDS_BUTTON_LOCATOR).click();
    }

    /**
     * Verifies that the friends search bar text is correct.
     *
     * @param expectedText The expected text in the search bar
     * @return true if the search bar text matches the expected text, false otherwise
     */
    public boolean verifySearchBarText(String expectedText) {
        final WebElement searchBar = waitForElementPresent(FRIENDS_SEARCHBAR_INPUT_LOCATOR);
        return searchBar.getAttribute("value").equals(expectedText);
    }

    /**
     * Verifies that a friend's real name is correct.
     *
     * @param username The username of the friend whose real name we want to
     * verify
     * @param expectedRealName The expected real name of the friend ("" to check
     * the non existence of the name)
     * @return true if the friend's real name is correctly displayed, false otherwise
     */
    public boolean verifyRealName(String username, String expectedRealName) {
        return getFriend(username)
                .getRealName()
                .map(x -> x.equals(expectedRealName))
                .orElse(expectedRealName.equals(""));
    }

    /**
     * Verifies that a friend is found in the 'Friends' section of the 'Social Hub'.
     *
     * @param username The username to look for in the 'Friend' section of the
     * 'Social Hub'
     * @return true if the friend was found, false otherwise
     */
    public boolean verifyFriendVisible(String username) {
        _log.debug("verifying friend visible: " + username);
        //If the mouse is hovered over the friend in the social hub,
        //then the friend's real name will be visible instead, which will
        //interfere with checking if that friend is visible in the social hub.
        //Hovering the mouse over something else (like the friend search bar)
        //will work around this issue.
        hoverOnElement(waitForElementPresent(FRIENDS_SEARCHBAR_INPUT_LOCATOR));

        try {
            final List<WebElement> allFriends = waitForAllElementsVisible(ALL_FRIEND_USERNAMES_LOCATOR);
            for (final WebElement friend : allFriends) {
                _log.debug("'" + friend.getText() + "' visible");
            }
            final WebElement correctFriend = getElementWithText(allFriends, username);
            return correctFriend != null && correctFriend.isDisplayed();
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verifies that a friend request has been sent to a user
     *
     * @param username The username that should be verified as a pending friend
     * @return true if the specified user shows up as a pending friend, meaning
     * that a friend request has been sent but not yet been accepted
     */
    public boolean verifyFriendRequestSent(String username) {
        try {
            final List<WebElement> allPendingFriends = waitForAllElementsVisible(ALL_OUTGOING_USERNAMES_LOCATOR);
            final WebElement correctFriend = getElementWithText(allPendingFriends, username);
            return correctFriend != null && correctFriend.isDisplayed();
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify that a friend request has been received from a requestor user.
     *
     * @param username Requestor's username
     */
    public boolean verifyFriendRequestReceived(String username) {
        try {
            final List<WebElement> allIncomingRequestElement = waitForAllElementsVisible(ALL_INCOMING_USERNAMES_LOCATOR);
            final WebElement requestElement = getElementWithText(allIncomingRequestElement, username);
            return requestElement != null && requestElement.isDisplayed();
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verifies that the 'X' button on the friend search bar exists.
     *
     * @return true if the 'X' button is shown on the friend search bar,
     * false otherwise
     */
    public boolean verifySearchBarClearButtonExists() {
        return waitForElementPresent(FRIENDS_SEARCHBAR_CLEAR_BUTTON_LOCATOR).isDisplayed();
    }

    /**
     * Get 'Presence Status' String from the 'Social Hub'.
     *
     * @return 'Presence Status' String
     */
    public String getSocialHubPresenceStatus() {
        _log.debug("getting social presence status");
        this.waitForPageToLoad();
        this.waitForAngularHttpComplete(30);
        final WebElement presence = waitForElementVisible(USER_PRESENCE_LOCATOR, 15);

        return presence.getText();
    }
    
    /**
     * Verifies the presence dot color matches that of the parameter
     *
     * @param matchColor color to match against
     * @return true if the color is match, false otherwise
     */
    private boolean verifyDotStatusColor(String matchColor){
        try {
           return getColorOfElement(waitForElementVisible(USER_PRESENCE_DOT_LOCATOR), "background-color").equals(matchColor);
        } catch (TimeoutException e) {
            return false;
        }
    }
    
    /**
     * Verifies the color of the presence dot is gray when offline
     * 
     * @return true if the color is a match, false otherwise
     */
    public boolean verifyOfflineDotStatusColor(){
        return verifyDotStatusColor(USER_DOT_PRESENCE_OFFLINE_COLOUR);
    }
    

    /**
     * Verify presence on the 'Social Hub' text matches that of the parameter.
     *
     * @param presence Type of presence to verify
     * @return true if matches, false otherwise
     */
    private boolean verifyUserPresence(PresenceType presence) {
        return (presence.getPresenceText().equals(getSocialHubPresenceStatus()));
    }

    /**
     * Verify presence on the 'Social Hub' is 'Online'.
     *
     * @return true if presence is 'Online', false otherwise
     */
    public boolean verifyUserOnline() {
        return verifyUserPresence(PresenceType.ONLINE);
    }

    /**
     * Verify presence on the 'Social Hub' is 'Away'.
     *
     * @return true if presence is 'Away', false otherwise
     */
    public boolean verifyUserAway() {
        return verifyUserPresence(PresenceType.AWAY);
    }

    /**
     * Verify presence on the 'Social Hub' is 'Invisible'.
     *
     * @return true if presence is 'Invisible', false otherwise
     */
    public boolean verifyUserInvisible() {
        return verifyUserPresence(PresenceType.INVISIBLE);
    }

    /**
     * Verify presence on the 'Social Hub' is 'Offline'.
     *
     * @return true if presence is 'Offline', false otherwise
     */
    public boolean verifyUserOffline() {
        return verifyUserPresence(PresenceType.OFFLINE);
    }

    /**
     * Verify presence on the 'Social Hub' is 'In Game'.
     *
     * @return true if presence is 'In Game', false otherwise
     */
    public boolean verifyUserInGame() {
        return verifyUserPresence(PresenceType.INGAME);
    }

    /**
     * Verify that the 'No Friends' message is displayed and correct.
     *
     * @return true if the 'No Friends' message header and body is both correct
     * and displayed, false otherwise
     */
    public boolean verifyNoFriendsMessage() {
        WebElement noFriendsMessageTitle, noFriendsMessageBody;
        try {
            noFriendsMessageTitle = waitForElementVisible(NO_FRIENDS_MESSAGE_HEADER_LOCATOR);
            noFriendsMessageBody = waitForElementVisible(NO_FRIENDS_MESSAGE_BODY_LOCATOR);
        } catch (TimeoutException e) {
            return false;
        }
        boolean titleCorrect = StringHelper.containsAnyIgnoreCase(noFriendsMessageTitle.getText(), OriginClientData.NO_FRIENDS_TITLE);
        final String[] expectedBody = {"friends", "search"};
        boolean bodyCorrect = StringHelper.containsIgnoreCase(noFriendsMessageBody.getText(), expectedBody);
        return titleCorrect && bodyCorrect;
    }

    /**
     * Verify that the 'Find Friends' button is displayed.
     *
     * @return true if the 'Find Friends' button is displayed
     */
    public boolean verifyFindFriendsButtonDisplayed() {
        return waitIsElementVisible(FIND_FRIENDS_BUTTON_LOCATOR);
    }

    /**
     * Set the user 'Presence Status' to that matching the presence parameter in 'Social Hub'.
     *
     * @param presence Presence type that has to be set
     */
    public void setUserStatus(PresenceType presence) {
        _log.debug("setting user status: " + presence.getPresenceText());
        clickUserPresenceDropDown();
        List<WebElement> userPresenceDropDownOptions = waitForAllElementsVisible(USER_PRESENCE_CONTEXT_MENU_OPTIONS_LOCATOR);
        for (WebElement option : userPresenceDropDownOptions) {
            if (presence.getPresenceText().contains(option.getText())) {
                option.click();
                break;
            }
        }
    }

    /**
     * Set the user 'Presence Status' to 'Online'.
     */
    public void setUserStatusOnline() {
        setUserStatus(PresenceType.ONLINE);
        Waits.pollingWait(() -> verifyPresenceDotOnline());
    }

    /**
     * Set the user 'Presence Status' to 'Away'.
     */
    public void setUserStatusAway() {
        setUserStatus(PresenceType.AWAY);
        Waits.pollingWait(() -> verifyPresenceDotAway());
    }

    /**
     * Set the user 'Presence Status' to 'Invisible'.
     */
    public void setUserStatusInvisible() {
        setUserStatus(PresenceType.INVISIBLE);
        Waits.pollingWait(() -> verifyPresenceDotInvisible());
    }

    /**
     * Set the user 'Presence Status' to 'In Game'.
     */
    public void setUserStatusInGame() {
        setUserStatus(PresenceType.INGAME);
    }

    /**
     * Click on the user 'Presence' dropdown.
     */
    public void clickUserPresenceDropDown() {
        final WebElement userPresenceContextMenu = waitForElementPresent(USER_PRESENCE_CONTEXT_MENU_LOCATOR);
        hoverOnElement(userPresenceContextMenu);
        forceClickElement(userPresenceContextMenu);
    }

    /**
     * Verify the user 'Presence' dropdown is visible.
     *
     * @return true if the class has invisible part of it, false otherwise
     */
    public boolean verifyUserPresenceDropDownVisible() {
        return waitIsElementVisible(USER_PRESENCE_CONTEXT_MENU_VISIBLE_LOCATOR);
    }

    /**
     * Click on the 'Social Hub' header.
     */
    public void clickOnSocialHubHeader() {
        waitForElementClickable(SOCIAL_HUB_AREA_HEADER_LOCATOR).click();
    }

    /**
     * Verify 'Presence' dot has innerclass containing presence parameter passed.
     *
     * @param presence Presence that needs to be checked
     * @return true if inner class has presence type, false otherwise
     */
    public boolean verifyPresenceDotStatus(PresenceType presence) {
        _log.debug("verifying presence dot status: " + presence.getPresenceText());
        return waitForElementVisible(USER_PRESENCE_DOT_LOCATOR).getAttribute("class").contains(presence.getPresenceText().toLowerCase());
    }

    /**
     * Verify 'Presence' dot is set to 'Online' status.
     *
     * @return true if status is 'Online', false otherwise
     */
    public boolean verifyPresenceDotOnline() {
        return verifyPresenceDotStatus(PresenceType.ONLINE);
    }

    /**
     * Verify 'Presence' dot is set to 'Away' status.
     *
     * @return true if status is 'Away', false otherwise
     */
    public boolean verifyPresenceDotAway() {
        return verifyPresenceDotStatus(PresenceType.AWAY);
    }

    /**
     * Verify 'Presence' dot is set to 'Invisible' status.
     *
     * @return true if status is 'Invisible', false otherwise
     */
    public boolean verifyPresenceDotInvisible() {
        return verifyPresenceDotStatus(PresenceType.INVISIBLE);
    }

    /**
     * Verify 'Presence' dot is set to 'Offline'.
     *
     * @return true if status is 'Offline', false otherwise
     */
    public boolean verifyPresenceDotOffline() {
        return verifyPresenceDotStatus(PresenceType.OFFLINE);
    }

    /**
     * Verifies that an avatar exists for every friend request.
     *
     * @return true if able find 'https://' from image source, false otherwise
     */
    public boolean verifyAvatarsForFriendRequestExist() {
        List<WebElement> avatarsForFriendRequest = waitForAllElementsVisible(ALL_FRIEND_REQUEST_AVATARS);
        for (WebElement element : avatarsForFriendRequest) {
            if (!element.getAttribute("ng-src").contains("https://")) {
                return false;
            }
        }
        return true;
    }

    /**
     * Verify that the there is a list of usernames for friend requests.
     *
     * @return true if successfully found usernames from 'Friend Requests',
     * false otherwise
     */
    public boolean verifyListOfUserNameExistsForFriendRequest() {
        List<WebElement> friendRequests = getUserNamesForFriendRequest();
        if (friendRequests.isEmpty()) {
            return false;
        } else {
            for (WebElement element : friendRequests) {
                if (element.getText().isEmpty()) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * Get usernames for friend requests.
     *
     * @return List of user names for friend requests
     */
    public List<WebElement> getUserNamesForFriendRequest() {
        return waitForAllElementsVisible(ALL_FRIEND_REQUEST_USERNAMES);
    }

    /**
     * Verifies that usernames for friend requests are sorted alphabetically.
     *
     * @return true if friend requests are sorted alphabetically by username,
     * false otherwise
     */
    public boolean verifyUsernameSortedAlphabeticallybyFriendRequest() {
        List<WebElement> allFriendsRequests = waitForAllElementsVisible(ALL_FRIEND_REQUESTS_LOCATOR);
        for (int i = 0; i < allFriendsRequests.size() - 1; i++) {
            String name1 = allFriendsRequests.get(i).getText();
            String name2 = allFriendsRequests.get(i + 1).getText();
            int result = name1.compareToIgnoreCase(name2);

            // if result is bigger than 0, it means first name is bigger than second name
            if (result > 0) {
                return false;
            }
        }
        return true;
    }

    /**
     * Verifies that there is friend request 'Accept' icon for every friend
     * request.
     *
     * @return true if number of friend request 'Accept' icons is
     * the same as the number of requests, false otherwise
     */
    public boolean verifyFriendsRequestAcceptIcons() {
        List<WebElement> allFriendsRequests = waitForAllElementsVisible(ALL_FRIEND_REQUESTS_LOCATOR);
        List<WebElement> allAvatarsFriendRequest = waitForAllElementsVisible(FRIEND_REQUEST_ACCEPT_BUTTON_LOCATOR);
        return allFriendsRequests.size() == allAvatarsFriendRequest.size();
    }

    /**
     * Verifies that there is friend request decline icon for every friend
     * request.
     *
     * @return true if the number of friend request 'Decline' icons is the same
     * as the number of requests, false otherwise
     */
    public boolean verifyFriendsRequestDeclineIcons() {
        List<WebElement> allFriendsRequests = waitForAllElementsVisible(ALL_FRIEND_REQUESTS_LOCATOR);
        List<WebElement> allAvatarsFriendDeclines = waitForAllElementsVisible(FRIEND_REQUEST_DECLINE_BUTTON_LOCATOR);
        return allFriendsRequests.size() == allAvatarsFriendDeclines.size();
    }

    /**
     * Verify a friend exists.
     *
     * @param userName Username of the friend
     * @return true if requester is a friend, false otherwise
     */
    public boolean verifyFriendExists(String userName) {
        _log.debug("verifying friend's existence: " + userName);
        return getFriend(userName) != null;
    }

    /**
     * Verify if username for requester does exist in friends request.
     *
     * @param userName Name of requester
     * @return true if name of requester is found in 'Friend Requests',
     * false otherwise
     */
    public boolean verifyUserNameExistInFriendRequests(String userName) {
        return getFriendRequests().stream()
                .anyMatch(req -> req.getText().equals(userName));
    }

    /**
     * Returns all friend requests.
     *
     * @return List of friend requests WebElements
     */
    private List<WebElement> getFriendRequests() {
        return driver.findElements(INCOMING_FRIEND_REQUESTS_LOCATOR);
    }

    /**
     * In case there are two lines for username for friend request, this
     * removes second line before comparing Strings.
     *
     * @param friendRequests List of 'FriendRequest' element
     * @param userName Username of friend
     * @return User request
     */
    private WebElement getFriendRequestElementWithText(List<WebElement> friendRequests, String userName) {
        WebElement result = null;
        if (friendRequests != null && userName != null) {
            for (WebElement friendRequest : friendRequests) {
                String friendRequestUserName = friendRequest.getText().split("\\n")[0];
                if (friendRequestUserName.equals(userName)) {
                    result = friendRequest;
                    break;
                }
            }
        }
        return result;
    }

    /**
     * Verify the order of friends in the friends list. 'In Game' friends should
     * be displayed first, followed by 'Online' friends, followed by 'Away' friends,
     * followed by 'Offline' friends.
     *
     * @return true if the friends are ordered correctly based on their
     * presence, false otherwise
     */
    public boolean verifyFriendsPresenceOrder() {
        List<Friend> allFriends = getAllFriends();
        PresenceType previousPresence = PresenceType.INGAME;
        for (Friend friend : allFriends) {
            PresenceType currentPresence = friend.getPresenceFromAvatar();
            if (currentPresence.index < previousPresence.index) {
                return false;
            }
            previousPresence = currentPresence;
        }
        return true;
    }

    /**
     * Verify that the friend tiles that have the specified presence are in
     * alphabetical order (ignoring case).
     *
     * @param presence The presence to check. All other friend tiles that do not
     * have this presence are ignored
     * @return true if the set of displayed friend tiles with the specified
     * presence are in alphabetical order. False if at least one friend tiles is
     * not in alphabetical order.
     */
    public boolean verifyFriendsAlphabeticalOrder(PresenceType presence) {

        //Gets the friend usernames that match the given presence
        List<String> usernamesMatchingPresence = new ArrayList<>();
        for (String username : getAllFriendUsernames()) {
            Friend friend = getFriend(username);
            if (friend.getPresenceFromAvatar() == presence) {
                usernamesMatchingPresence.add(username);
            }
        }

        for (int i = 0; i < usernamesMatchingPresence.size() - 1; ++i) {
            String currentFriend = usernamesMatchingPresence.get(i).toLowerCase();
            String nextFriend = usernamesMatchingPresence.get(i + 1).toLowerCase();
            if (currentFriend.compareTo(nextFriend) > 0) {
                return false;
            }
        }

        return true;
    }

    /**
     * Verify that the 'Social Hub' is visible.
     *
     * @return true if the 'Social Hub' is visible, false otherwise
     */
    public boolean verifySocialHubVisible() {
        return waitIsElementVisible(SOCIAL_HUB_AREA_HEADER_LOCATOR);
    }

    /**
     * Verify the 'Social Hub displays the expected offline message title and content
     * message subset.
     *
     * @return true if both offline message title and content message subset are
     * correct, false otherwise
     */
    public boolean verifyOfflineMessage() {
        boolean titleOK = waitForElementVisible(OFFLINE_MESSAGE_TITLE_LOCATOR).getText().equalsIgnoreCase(OFFLINE_MESSAGE_TITLE);
        boolean contentOK = StringHelper.containsIgnoreCase(waitForElementVisible(OFFLINE_MESSAGE_CONTENT_LOCATOR).getText(), OFFLINE_MESSAGE_CONTENT_KEYWORDS);
        return titleOK && contentOK;
    }

    /**
     * Verifies that all the menu items in the 'Social Hub' dropdown context menu are visible
     *
     * @return true if all the menu items are visible
     */
    public boolean verifySocialHubMenuItemsVisible(){
        ContextMenu contextMenu = new ContextMenu(driver, SOCIAL_HUB_MENU_DROPDOWN_LOCATOR, true);
        boolean menuItemsVisible = contextMenu.verifyAllItemsVisible(SOCIAL_HUB_MENUOPTION_MINIMIZE_WINDOW,
                SOCIAL_HUB_MENUOPTION_OPEN_IN_NEW_WINDOW, SOCIAL_HUB_MENUOPTION_CHAT_SETTINGS, SOCIAL_HUB_MENUOPTION_CLOSE_ALL_CHAT_TABS);
        waitForElementClickable(SOCIAL_HUB_MENU_DROPDOWN_LOCATOR).click(); // closes the context menu so that the next test can click again to open
        return menuItemsVisible;
    }

    /**
     * Verify that all the menu items in the dropdown menu within the poped out 'Social Hub' is visible
     *
     * @return true if all the menu items for the popped out 'Social Hub' is visible
     */
    public boolean verifyPopOutSocialHubMenuItemsVisible(){
        ContextMenu contextMenu = new ContextMenu(driver, SOCIAL_HUB_MENU_DROPDOWN_LOCATOR, true);
        WebElement menuItem = contextMenu.getItem(SOCIAL_HUB_MENUOPTION_OPEN_IN_NEW_WINDOW);
        if (menuItem != null){
            menuItem.click();
            sleep(5000); // Wait for Chat to load
            Waits.switchToPageThatMatches(driver, SOCIAL_HUB_POPOUT_WINDOW_URL);
            contextMenu = new ContextMenu(driver, SOCIAL_HUB_MENU_DROPDOWN_LOCATOR, true);
            return contextMenu.verifyAllItemsVisible(SOCIAL_HUB_MENUOPTION_CHAT_SETTINGS, SOCIAL_HUB_MENUOPTION_CLOSE_ALL_CHAT_TABS)
                    && !contextMenu.verifyAllItemsVisible(SOCIAL_HUB_MENUOPTION_MINIMIZE_WINDOW, SOCIAL_HUB_MENUOPTION_OPEN_IN_NEW_WINDOW);
        }
        return false;
    }

    /**
     * Clicks the specified menu item within the 'Social Hub' Drop down menu
     *
     * @param menuItemTitle the title of menu item to be clicked
     */
    public void clickSocialHubDropdownMenuOption(String menuItemTitle){
        ContextMenu contextMenu = new ContextMenu(driver, SOCIAL_HUB_MENU_DROPDOWN_LOCATOR, true);
        WebElement menuItem = contextMenu.getItem(menuItemTitle);
        if(menuItem != null){
            menuItem.click();
        }
    }

    /**
     * Clicks the 'Chat Setings' menu option in the 'Social Hub'
     */
    public void clickChatSettingsMenuOption(){
        clickSocialHubDropdownMenuOption(SOCIAL_HUB_MENUOPTION_CHAT_SETTINGS);
    }

    /**
     * Clicks the 'Close All Chat Tabs' menu option in the 'Social Hub'
     */
    public void clickCloseAllChatTabsMenuOption(){
        clickSocialHubDropdownMenuOption(SOCIAL_HUB_MENUOPTION_CLOSE_ALL_CHAT_TABS);
    }

    /**
     * Opens the 'Social Hub' drop down menu and verifies that clicking anywhere else on the 'Social Hub' will close the context menu
     *
     * @return true if the context menu was closed when clicking within the 'Social Hub'
     */
    public boolean verifySocialHubDropdownCloseOnSocialHubClick(){
        ContextMenu contextMenu = new ContextMenu(driver, SOCIAL_HUB_MENU_DROPDOWN_LOCATOR, true);
        contextMenu.openDropdown();
        clickFriendsHeader();
        return !contextMenu.verifyDropdownOpen();
    }

    /**
     * Opens the 'Social Hub' drop down menu and verifies that clicking anywhere eon the client will close the context menu
     *
     * @return true if the context menu was closed when clicking within the client window
     */
    public boolean verifySocialHubDropdownCloseOnClientClick(){
        boolean leftClick = true;
        ContextMenu contextMenu = new ContextMenu(driver, SOCIAL_HUB_MENU_DROPDOWN_LOCATOR, leftClick);
        contextMenu.openDropdown();
        new GlobalSearch(driver).clickOnGlobalSearch();
        return !contextMenu.verifyDropdownOpen();
    }
    
    /**
     * Verify that the 'Add Contacts' button is displayed.
     *
     * @return true if the 'Add Contacts' button is displayed
     */
    public boolean verifyAddContactsButtonVisible() {
        return waitIsElementVisible(FRIENDS_ADD_CONTACTS_BUTTON_LOCATOR);
    }
    
    /**
     * Clicks on 'Add Contacts' button, which opens a 'Search for friends'
     * dialog.
     */
    public void clickAddContactsButton() {
        waitForElementClickable(FRIENDS_ADD_CONTACTS_BUTTON_LOCATOR).click();
    }
}