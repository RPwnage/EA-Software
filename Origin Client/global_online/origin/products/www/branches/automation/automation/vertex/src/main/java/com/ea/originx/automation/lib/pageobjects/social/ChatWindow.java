package com.ea.originx.automation.lib.pageobjects.social;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.pageobjects.template.OAQtSiteTemplate;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import static com.ea.originx.automation.lib.resources.OriginClientData.ORIGIN_URL_MESSAGE_COLOUR;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.utils.Waits;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.TimeUnit;
import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.*;
import org.openqa.selenium.support.Color;

/**
 * Page object that represents the 'Chat Window'.
 *
 * @author glivingstone
 */
public class ChatWindow extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(com.ea.originx.automation.lib.pageobjects.social.ChatWindow.class);

    protected static final By CHAT_WINDOW_LOCATOR = By.cssSelector("div.origin-social-chatwindow-area");

    protected static final By CHAT_WINDOW_TITLE_LOCATOR = By.cssSelector("h3.origin-titlebar-subwindow-title");
    protected static final By MINIMIZE_BUTTON_LOCATOR = By.cssSelector("div.origin-titlebar-subwindow-minimize"); // popped-in chat window
    protected static final By POPOUT_BUTTON_LOCATOR = By.cssSelector("div.origin-titlebar-subwindow-popout");

    protected static final By PHISHING_WARNING_LOCATOR = By.xpath("//h6[contains(@class,'otkc-small origin-social-chatwindow-conversation-historyitem-system') and contains(text(),'password')]");
    protected static final By OFFLINE_NOTIFICATION_LOCATOR = By.cssSelector("h6.otkc.otkc-small.origin-social-chatwindow-conversation-historyitem-system.system-bubble");
    protected static final By FRIEND_STATUS_LOCATOR = By.cssSelector("h6.otkc.otkc-small.origin-social-chatwindow-conversation-historyitem-system.system-bubble");
    protected static final By FRIEND_STATUS_BUBBLE_LOCATOR = By.cssSelector("origin-titlebar-subwindow header.origin-titlebar-subwindow div.origin-titlebar-subwindow-transclude i.otkpresence");

    protected static final By CONVERSATION_HISTORY_LOCATOR = By.cssSelector(".origin-social-chatwindow-conversation-history");

    protected static final By TEXT_INPUT_AREA_LOCATOR = By.cssSelector("textarea.origin-social-chatwindow-conversation-input");

    protected static final By CONVERSATION_BUBBLE_LOCATOR = By.cssSelector("div.origin-social-chatwindow-conversation-historyitem-monologue-bubble");

    protected static final By AVATAR_LOCATOR = By.cssSelector("img.otkavatar-img");

    protected static final String CHAT_WINDOW_OPEN_TABS_CSS = "div.origin-social-chatwindow-area.origin-social-chatwindow-area-showing-tabs";
    protected static final String CHAT_WINDOW_OPEN_TABS_XPATH = "//div[@class='origin-social-chatwindow-area origin-social-chatwindow-area-showing-tabs']";
    protected static final By CHAT_WINDOW_OPEN_TABS_LOCATOR = By.cssSelector(CHAT_WINDOW_OPEN_TABS_CSS);

    protected static final By CHAT_TAB_ACTIVE_LOCATOR = By.cssSelector(CHAT_WINDOW_OPEN_TABS_CSS + " li.otktab-active");
    protected static final By CHAT_TAB_ACTIVE_TITLE_LOCATOR = By.cssSelector(CHAT_WINDOW_OPEN_TABS_CSS + " li.otktab-active div.otktab-content span.otktab-title");
    protected static final By CHAT_TAB_VISIBLE_LOCATOR = By.cssSelector(CHAT_WINDOW_OPEN_TABS_CSS + " li.origin-social-chatwindow-tab");

    protected static final String CHAT_TAB_USER_XPATH_TMPL = CHAT_WINDOW_OPEN_TABS_XPATH + "//li[contains(@class, 'origin-social-chatwindow-tab ')]//strong[text()[contains(., '%s')]]/../..";
    protected static final String CHAT_TAB_USER_CLOSE_BUTTON_XPATH_TMPL = CHAT_TAB_USER_XPATH_TMPL + "/../..//SPAN[contains(@class, 'origin-social-chatwindow-tab-close')]//i[contains(@class, 'otkicon otkicon-close')]";

    protected static final String CHAT_TAB_OVERFLOW_CSS = CHAT_WINDOW_OPEN_TABS_CSS + " li.origin-social-chatwindow-tab-overflow-tab";
    protected static final String CHAT_TAB_OVERFLOW_XPATH = CHAT_WINDOW_OPEN_TABS_XPATH + "//li[contains(@class, 'origin-social-chatwindow-tab-overflow-tab')]";

    protected static final By CHAT_TAB_OVERFLOW_LOCATOR = By.cssSelector(CHAT_TAB_OVERFLOW_CSS);
    protected static final By CHAT_TAB_OVERFLOW_DROPDOWN_LOCATOR = By.cssSelector(CHAT_TAB_OVERFLOW_CSS + " div.otkdropdown-wrap ul.otkmenu-dropdown");
    protected static final By CHAT_TAB_OVERFLOW_DROPDOWN_TABS_LOCATOR = By.cssSelector(CHAT_TAB_OVERFLOW_CSS + " div.otkdropdown-wrap ul.otkmenu-dropdown li.origin-social-chatwindow-tab-overflow-menu-item");
    protected static final By CHAT_TAB_OVERFLOW_LINK_LOCATOR = By.cssSelector(CHAT_TAB_OVERFLOW_CSS + " a.otkdropdown-trigger-caret");
    protected static final By CHAT_TAB_OVERFLOW_MENU_LOCATOR = By.cssSelector(CHAT_TAB_OVERFLOW_CSS + " #origin-social-chatwindow-tab-overflow-menu");

    protected static final By TIMESTAMP_LOCATOR = By.cssSelector("h5.otkc.otkc-small.origin-social-chatwindow-conversation-historyitem-timestamp-time");

    protected static final String CHAT_OVERFLOW_MENU_USER_XPATH_TMPL = CHAT_TAB_OVERFLOW_XPATH + "//li[contains(@class, 'origin-social-chatwindow-tab-overflow-menu-item')]//strong[text()[contains(., '%s')]]/../..";
    protected static final String CHAT_OVERFLOW_MENU_USER_CLOSE_BUTTON_XPATH_TMPL = CHAT_OVERFLOW_MENU_USER_XPATH_TMPL + "//span[contains(@class, 'origin-social-chatwindow-tab-overflow-menu-item-close')]//i[contains(@class, 'otkicon otkicon-close')]";
    protected static final String CHAT_OVERFLOW_MENU_USER_PRESENCE_INDICATOR_XPATH_TMPL = CHAT_OVERFLOW_MENU_USER_XPATH_TMPL + "//origin-social-chatwindow-conversation-presenceindicator";

    protected static final String CONVERSATION_BANNER_CSS = ".origin-social-conversation-banner-section";
    protected static final String CONVERSATION_BANNER_GAME_TITLE_CSS = CONVERSATION_BANNER_CSS + " section:not(.ng-hide) span:not(.ng-hide) .origin-social-chatwindow-conversation-banner-productlink:not(.ng-hide)";
    protected static final By CONVERSATION_BANNER_LOCATOR = By.cssSelector(CONVERSATION_BANNER_CSS);
    protected static final By CONVERSATION_BANNER_GAME_TITLE_LOCATOR = By.cssSelector(CONVERSATION_BANNER_GAME_TITLE_CSS);
    protected static final By CONVERSATION_MESSAGES_LOCATOR = By.cssSelector("div.origin-social-chatwindow-conversation-historyitem-monologue.self .origin-social-chatwindow-conversation-historyitem-monologue-bubble");
    protected static final By ONE_MESSAGE_LOCATOR = By.cssSelector("div.origin-social-chatwindow-conversation-historyitem-monologue.self .origin-social-chatwindow-conversation-historyitem-monologue-bubble a");
    protected static final By LEFT_MESSAGE_CHECK = By.xpath("//a[@class='otka user-provided']");
    
    protected static final String CONVERSATION_TOOLBAR_CSS = ".origin-social-conversation-toolbar";
    protected static final By VOICE_CHAT_SETTINGS_LOCATOR = By.cssSelector(CONVERSATION_TOOLBAR_CSS + " span[origin-tooltip='Voice chat settings']");

    //OIG
    private static final By OIG_CHAT_WINDOW_LOCATOR = By.xpath("//Origin--UIToolkit--OriginWindowTemplateMsgBox");
    private static final String OIG_WINDOW_URL = "qtwidget://Origin::Engine::IGOQWidget";
    private com.ea.originx.automation.lib.pageobjects.social.ChatWindow.QtChatWindow qtChatWindow = null;

    //Popped in/out Window
    protected static final String CHAT_WINDOW_POPPED_OUT_REGEX = ".*social-chatwindow((?!oigContext).)*";
    protected static final By CHAT_WINDOW_POPPED_IN_LOCATOR = By.cssSelector(".origin-titlebar-subwindow-popin i.otkicon-expandarrow");
    protected static final By CHAT_WINDOW_POPPED_IN_CLOSE_LOCATOR = By.cssSelector(".origin-titlebar-subwindow-close i.otkicon-close");
    protected static final By CHAT_WINDOW_POPPED_OUT_LOCATOR = By.cssSelector(".otk .popped-out");
    protected static final String QT_CHAT_WINDOW_POPPED_OUT_URL = ".*qtwidget://Origin::UIToolkit::OriginWindow.*";
    protected static final By TYPING_DOTS_LOCATOR = By.cssSelector("img.origin-social-chatwindow-conversation-history-typing-indicator");
    protected static final By QT_POPPED_OUT_WINDOW_CLOSE_LOCATOR = By.xpath("//*[@id='btnClose']");
    protected static final By QT_POPPED_OUT_WINDOW_MINIMIZE_LOCATOR = By.xpath("//*[@id='btnMinimize']");
    protected static final By CLOSE_BUTTON_LOCATOR = QT_POPPED_OUT_WINDOW_MINIMIZE_LOCATOR;
    protected static final By QT_POPPED_OUT_WINDOW_LOCATOR = By.xpath("//*[@id='OriginWindow_WebView']");

    private class QtChatWindow extends OAQtSiteTemplate {

        public QtChatWindow(WebDriver driver) {
            super(driver);
        }

        /**
         * Click 'X' button to close the dialog
         */
        public void clickXButton() {
            Waits.waitForPageThatMatches(driver, OriginClientData.OIG_PAGE_URL_REGEX, 8);
            waitForElementClickable(CLOSE_BUTTON_LOCATOR).click();
        }

    }

    /**
     * Enum for chat presence.
     */
    public enum Presence {
        ONLINE, INGAME, BROADCASTING, JOINABLE, AWAY, OFFLINE
    }

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ChatWindow(WebDriver driver) {
        super(driver);
        qtChatWindow = new com.ea.originx.automation.lib.pageobjects.social.ChatWindow.QtChatWindow(driver);
    }

    /**
     * Sends a message with the given text in the current chat window.
     *
     * @param text The text we want to send in the message
     */
    public void sendMessage(String text) {
        _log.debug("send message: " + text);
        WebElement thingyWhat = waitForElementClickable(TEXT_INPUT_AREA_LOCATOR);
        thingyWhat.sendKeys(text);
        thingyWhat.sendKeys(Keys.RETURN);
    }

    /**
     * Gets the size of the message box
     *
     * @return the width of the message box
     */
    public int getMessageBoxSize() {
        return driver.findElement(CONVERSATION_HISTORY_LOCATOR).getSize().getWidth();
    }

    /**
     * Gets the height of the message box
     *
     * @return the height of the message box
     */
    public int getMessageBoxHeight() {
        return driver.findElement(CONVERSATION_HISTORY_LOCATOR).getSize().getHeight();
    }

    /**
     * Gets the size of the text input area
     *
     * @return the height of the text input area
     */
    public int getInputTextAreaSize() {
        return driver.findElement(TEXT_INPUT_AREA_LOCATOR).getSize().getHeight();
    }

    /**
     * Minimize the chat window by clicking on the 'Minimize' button.
     */
    public void minimizeChatWindow() {
        _log.debug("minimizing chat window");
        waitForElementClickable(MINIMIZE_BUTTON_LOCATOR).click();
    }

    /**
     * Send a message a given number of times
     *
     * @param numberOfMessages how many times we should send the message
     * @param message what needs to be sent
     */
    public void sendMultipleMessages(int numberOfMessages, String message) {
        for (int i = 0; i < numberOfMessages; i++) {
            sendMessage(message);
        }
    }

    /**
     * Verify no more than 2048 characters can be sent in a message
     *
     * @return true if no more than 2048 characters can be sent, false otherwise
     */
    public boolean verifyMaximumMessageSizeReached() {
        WebElement textInput = waitForElementClickable(TEXT_INPUT_AREA_LOCATOR);
        textInput.sendKeys(StringUtils.repeat("a", 2048));
        textInput.sendKeys("z");
        textInput.sendKeys(Keys.RETURN);
        boolean result = !getLastMessage().contains("z");
        return result;
    }

    /**
     * gets the last message sent by the user
     *
     * @return the last message as a String
     */
    public String getLastMessage() {
        List<WebElement> messages = driver.findElements(CONVERSATION_MESSAGES_LOCATOR);        
        return messages.get(messages.size() - 1).getText();
    }

    /**
     * Get the initial size of the text input area, send a long message, verify
     * the size increases, delete the message and verify the text input area
     * resets its size
     *
     * @return true if the size of the text input area changes with the size of
     * the text
     */
    public boolean verifyTextInputAreaResizes() {
        WebElement textInput = waitForElementClickable(TEXT_INPUT_AREA_LOCATOR);
        int initialSize = getInputTextAreaSize();
        textInput.sendKeys(StringUtils.repeat("a", 100));
        int secondSize = getInputTextAreaSize();
        sleep(2000);
        return initialSize < secondSize;
    }

    /**
     * Pops out the chat window by clicking on the 'Pop out' button.
     */
    public void popoutChatWindow() {
        waitForElementClickable(POPOUT_BUTTON_LOCATOR).click();
    }

    /**
     * Closes a user's chat tab.
     *
     * @param username The name of the user whose chat tab we want to select
     * @param inOverflow Whether the chat tab is in the overflow menu or not
     */
    public void closeChatTab(String username, boolean inOverflow) {
        _log.debug("closing '" + username + "' chat tab");
        waitForAngularHttpComplete();
        final By closeButtonLocator;
        if (inOverflow) {
            closeButtonLocator = By.xpath(String.format(CHAT_OVERFLOW_MENU_USER_CLOSE_BUTTON_XPATH_TMPL, username));
        } else {
            closeButtonLocator = By.xpath(String.format(CHAT_TAB_USER_CLOSE_BUTTON_XPATH_TMPL, username));
        }
        WebElement element = waitForElementPresent(closeButtonLocator);
        // As far as I can tell, the element never becomes "displayed" or "clickable", so waiting for
        // those times out every time. But if you don't wait, Selenium complains that the element must be
        // displayed to click it. Manually waiting and then clicking it seems to work though.
        sleep(1000);
        element.click();
    }

    /**
     * Selects a user's chat tab.
     *
     * @param username The name of the user whose chat tab we want to select
     * @param inOverflow Whether the chat tab is in the overflow menu or not
     */
    public void selectChatTab(String username, boolean inOverflow) {
        _log.debug("selecting '" + username + "' chat tab");
        waitForAngularHttpComplete();
        WebElement chatTab;
        if (inOverflow) {
            chatTab = waitForElementClickable(By.xpath(String.format(CHAT_OVERFLOW_MENU_USER_XPATH_TMPL, username)));
        } else {
            chatTab = waitForElementClickable(By.xpath(String.format(CHAT_TAB_USER_XPATH_TMPL, username)));
        }
        chatTab.click();
    }

    /**
     * Clicks the overflow tab to open or close it.
     */
    public void clickOverflowTab() {
        _log.debug("clicking overflow tab");
        waitForAngularHttpComplete();
        waitForElementClickable(CHAT_TAB_OVERFLOW_LINK_LOCATOR).click();
    }

    /**
     * Verify the overflow dropdown is open.
     *
     * @return true if the overflow dropdown is open, false otherwise
     */
    public boolean verifyOverFlowOpen() {
        return waitIsElementVisible(CHAT_TAB_OVERFLOW_DROPDOWN_LOCATOR);
    }

    /**
     * Verify the overflow dropdown is scrollable. This occurs when there are
     * many chat windows open in the overflow tab. When using this function,
     * open more tabs such that when we scroll to the bottom of the list, the
     * first item is not on the screen.
     *
     * @return true if the overflow dropdown is scrollable, false otherwise
     */
    public boolean verifyOverFlowScrollable() {
        clickOverflowTab();
        if (waitIsElementVisible(CHAT_TAB_OVERFLOW_DROPDOWN_LOCATOR)) {
            List<WebElement> tabsInOverflow = driver.findElements(CHAT_TAB_OVERFLOW_DROPDOWN_TABS_LOCATOR);
            scrollToElement(tabsInOverflow.get(tabsInOverflow.size() - 1));
            return waitIsElementVisible(tabsInOverflow.get(0));
        } else {
            return false;
        }
    }

    /**
     * Click the user avatar in the 'Chat Window' that matches the parameter
     * avatarSrc except for the image size.
     *
     * @param avatarSrc The string that represents the users avatar URL
     */
    public void clickUserAvatar(String avatarSrc) {
        _log.debug("clicking user avatar");
        WebElement chatHead = waitForElementPresent(CHAT_WINDOW_LOCATOR);
        List<WebElement> allAvatars = chatHead.findElements(AVATAR_LOCATOR);

        // Remove the last part (size of the avatar image) of the url. (We likely extracted
        // the Avatar URL from MiniProfile and will try to find the matching avatar at the
        // ChatWindow, which will have a different size)
        String[] splitURL = avatarSrc.split("/");
        String avatarSrcUnsized = String.join("/", Arrays.copyOfRange(splitURL, 0, splitURL.length - 1));

        // Search for avatar with matching URL to click
        for (WebElement avatar : allAvatars) {
            if (avatar.getAttribute("src").contains(avatarSrcUnsized)) {
                waitForElementClickable(avatar).click();
                break;
            }
        }
    }

    /**
     * Verify if the avatar of the user and of the friend being chatted with are
     * displayed in the chat window
     *
     * @return true if both the avatars are displayed, false otherwise
     */
    public boolean verifyAvatarsAreDisplayed() {
        WebElement chatHead = waitForElementPresent(CHAT_WINDOW_LOCATOR);
        List<WebElement> allAvatars = chatHead.findElements(AVATAR_LOCATOR);
        boolean allAvatarsAreVisible = false; //suppose the avatars are not displayed
        List<String> alreadyParsed = new ArrayList<String>();
        for (WebElement avatar : allAvatars) {
            String attribute = avatar.getAttribute("alt");//there is always a duplicate of the first avatar, so we have to ignore it
            if (!alreadyParsed.contains(attribute)) {
                allAvatarsAreVisible = waitIsElementVisible(avatar);
                alreadyParsed.add(attribute);
            }
        }
        return allAvatarsAreVisible;
    }

    /**
     * Clicks the game title of the game banner that is in the chat window.
     */
    public void clickGameBannerTitleLink() {
        waitForElementClickable(CONVERSATION_BANNER_GAME_TITLE_LOCATOR).click();
    }

    /**
     * Verify the chat window is open.
     *
     * @return true if the chat window is open, false otherwise.
     */
    public boolean verifyChatOpen() {
        return waitIsElementVisible(CHAT_WINDOW_TITLE_LOCATOR);
    }

    /**
     * Verify if the current user's avatar and their messages are oriented on
     * the right side of the chat box
     *
     * @return true if the user's avatar and messages are oriented on the right
     * side of the chat box, false otherwise
     */
    public boolean verifyRightMessageAndAvatarPosition(String username) {
        WebElement chatHead = waitForElementPresent(CHAT_WINDOW_LOCATOR);
        List<WebElement> allAvatars = chatHead.findElements(AVATAR_LOCATOR);
        boolean avatarPosition = false;
        for (WebElement avatar : allAvatars) {
            String attribute = avatar.getAttribute("alt");
            if (attribute.equals(username)) {
                avatarPosition = (chatHead.getLocation().getX() + chatHead.getSize().getWidth()) / 2 < avatar.getLocation().getX();
                break;
            }
        }
        boolean conversationPostion = (chatHead.getLocation().getX() + chatHead.getSize().getWidth()) / 2 < driver.findElement(CONVERSATION_MESSAGES_LOCATOR).getLocation().getX();
        return conversationPostion && avatarPosition;
    }

    /**
     * Verify the avatar and their messages of the user being chatted with are
     * oriented on the left side of the chat box
     *
     * @return true if they are oriented on the left side of the chat box, false
     * otherwise
     */
    public boolean verifyLeftMessageAndAvatarPosition(String username) {
        WebElement chatHead = waitForElementPresent(CHAT_WINDOW_LOCATOR);
        List<WebElement> allAvatars = chatHead.findElements(AVATAR_LOCATOR);
        boolean avatarPosition = false;
        for (WebElement avatar : allAvatars) {
            String attribute = avatar.getAttribute("alt");
            if (attribute.equals(username)) {
                avatarPosition = (chatHead.getLocation().getX() + chatHead.getSize().getWidth()) / 2 > avatar.getLocation().getX();
                break;
            }
        }
        sleep(2000);
        boolean conversationPostion = (chatHead.getLocation().getX() + chatHead.getSize().getWidth()) / 2 > driver.findElement(LEFT_MESSAGE_CHECK).getLocation().getX();
        sleep(2000);
        return conversationPostion && avatarPosition;
    }

    /**
     * Gets the last message of the conversation, which contains an URL and
     * verifies if it is hyperlinked.
     *
     * @param URL_Message the message which has been sent
     * @return true if the color of the message is light blue, false otherwise.
     */
    public boolean verifyColourOfURL(String URL_Message) {
        String textColor = Color.fromString(driver.findElement(ONE_MESSAGE_LOCATOR).getCssValue("color")).asHex();
        boolean isBlue = StringHelper.containsIgnoreCase(textColor, ORIGIN_URL_MESSAGE_COLOUR);
        return isBlue;
    }

    /**
     * click on the URL from the last message received and verify if a browser
     * opens
     *
     * @param client
     * @return true if a browser opens
     * @throws IOException
     * @throws InterruptedException
     */
    public boolean verifyHyperlinkCanBeClicked(OriginClient client) throws IOException, InterruptedException {
        driver.findElement(ONE_MESSAGE_LOCATOR).click();
        sleep(10000);
        boolean verifyHyperlinkCanBeClicked = TestScriptHelper.verifyBrowserOpen(client);
        TestScriptHelper.killBrowsers(client);
        return verifyHyperlinkCanBeClicked;
    }

    /**
     * Verify a chat window is open with a user (in either the main or overflow
     * areas).
     *
     * @param username The username of the user to search for
     * @return true if there is a chat open when the user, false otherwise
     */
    public boolean verifyChatWithUserOpen(String username) {
        /* If there is only one chat window open, we check if the chat
         title is equal to the username
         */
        boolean onlyOneChat = !isElementPresent(CHAT_WINDOW_OPEN_TABS_LOCATOR)
                && !verifyChatTabsExist();

        if (onlyOneChat) {
            return verifyChatWindowTitle(username);
        } else {
            final boolean isInOverflow = verifyNameInOverflow(username);
            return isInOverflow;
        }

    }

    /**
     * Verify that multiple chat tabs are present.
     *
     * @return true if multiple chat tabs are present, false otherwise.
     */
    public boolean verifyChatTabsExist() {
        WebElement chatTabs = waitForElementPresent(CHAT_WINDOW_OPEN_TABS_LOCATOR);
        return chatTabs.isDisplayed();
    }

    /**
     * Verify that the currently displayed chat tabs are in the correct order.
     *
     * @param expectedChatTabs The expected chat tab names in the expected order
     * @return true if the chat tab order matches the expected order, false
     * otherwise
     */
    public boolean verifyChatTabOrder(String[] expectedChatTabs) {
        List<WebElement> chatTabs = driver.findElements(CHAT_TAB_VISIBLE_LOCATOR);
        if (expectedChatTabs.length != chatTabs.size()) {
            return false;
        }

        for (int i = 0; i < expectedChatTabs.length; ++i) {
            if (!chatTabs.get(i).getText().contains(expectedChatTabs[i])) {
                return false;
            }
        }

        return true;
    }

    /**
     * Verify that the name of the active chat tab is correct.
     *
     * @param expectedUsername The expected name for the active chat tab
     * @return true if the name of the active chat tab is correct, false
     * otherwise
     */
    public boolean verifyChatTabActive(String expectedUsername) {
        WebElement chatTabs = waitForElementPresent(CHAT_TAB_ACTIVE_LOCATOR);
        return chatTabs.getText().contains(expectedUsername);
    }

    /**
     * Verify that the active chat tab is highlighted.
     *
     * @return true if the active chat tab is highlighted, false otherwise
     */
    public boolean verifyChatTabHighlighted() {
        WebElement chatTab = waitForElementPresent(CHAT_TAB_ACTIVE_TITLE_LOCATOR);
        return Color.fromString(chatTab.getCssValue("color")).asHex().equals("#1e262c");
    }

    /**
     * Verify that the overflow chat tab exists.
     *
     * @return true if the overflow chat tab exists, false otherwise
     */
    public boolean verifyOverflowChatTabExists() {
        WebElement overflowTab = waitForElementPresent(CHAT_TAB_OVERFLOW_LOCATOR);
        return overflowTab.isDisplayed();
    }

    /**
     * Verify that the displayed number of overflow chats is correct.
     *
     * @param expectedNumber The expected number of overflow chat tabs
     * @return true if the displayed number of overflow chat tabs matches the
     * expected number, false otherwise
     */
    public boolean verifyOverflowNumber(int expectedNumber) {
        WebElement overflowTab = waitForElementPresent(CHAT_TAB_OVERFLOW_MENU_LOCATOR);
        int overflowTabNumber;
        try {
            overflowTabNumber = Integer.parseInt(overflowTab.getText());
        } catch (NumberFormatException nfe) {
            // If there are no tabs, getText will return the empty
            // string which will cause a NumberFormatException.
            // Catch it and treat it as 0
            overflowTabNumber = 0;
        }
        return expectedNumber == overflowTabNumber;
    }

    /**
     * Verify that a chat with a given user exists in the overflow menu.
     *
     * @param expectedName The username that is expected to be in the overflow
     * menu
     * @return true if the username is found in the overflow menu, false
     * otherwise
     */
    public boolean verifyNameInOverflow(String expectedName) {
        try {
            driver.findElement(By.xpath(String.format(CHAT_OVERFLOW_MENU_USER_XPATH_TMPL, expectedName)));
        } catch (NoSuchElementException e) {
            return false;
        }
        return true;
    }

    /**
     * Verify that the given user has a presence indicator in the overflow menu.
     *
     * @param username The user whose presence indicator we are expecting to
     * find in the overflow menu
     * @return true if the user's presence indicator is found, false otherwise
     */
    public boolean verifyUserHasPresenceInOverflow(String username) {
        try {
            driver.findElement(By.xpath(String.format(CHAT_OVERFLOW_MENU_USER_PRESENCE_INDICATOR_XPATH_TMPL, username)));
        } catch (NoSuchElementException e) {
            return false;
        }
        return true;
    }

    /**
     * Verify the 'Chat Window' title has the expected text in it; the username
     * of the user that the chat window is set to.
     *
     * @param expectedText The text we expect in the title.
     * @return true if the title text contains the expected text, false
     * otherwise
     */
    public boolean verifyChatWindowTitle(String expectedText) {
        WebElement chatTitle = waitForElementPresent(CHAT_WINDOW_TITLE_LOCATOR);
        return chatTitle.getText().contains(expectedText);
    }

    /**
     * Verify that the 'Anti-Phishing' warning appears.
     *
     * @return true if the phishing warning exists, false otherwise
     */
    public boolean verifyPhishingWarning() {
        return driver.findElements(PHISHING_WARNING_LOCATOR).size() > 0;
    }

    /**
     * Verify the phishing warning is not visible when the conversation extends
     *
     * @return true if the warning is not visible, false otherwise
     */
    public boolean verifyPhishingWarningNotVisible() {
        int chatYCoordinate = driver.findElement(CHAT_WINDOW_LOCATOR).getLocation().getY();
        int phishingYCoordinate = driver.findElement(PHISHING_WARNING_LOCATOR).getLocation().getY();
        return chatYCoordinate > phishingYCoordinate;
    }

    /**
     * Scrolls to the beginning of the conversation
     */
    public void scrollToBeginningOfChat() {
        WebElement phishingWarning = driver.findElement(PHISHING_WARNING_LOCATOR);
        scrollToElement(phishingWarning);
    }

    /**
     * Scrolls to the end of the conversation
     */
    public void scrollToEndOfChat() {
        List<WebElement> messages = driver.findElements(CONVERSATION_MESSAGES_LOCATOR);
        scrollToElement(messages.get(messages.size() - 2));
    }

    /**
     * Verify the 'User is Offline' message exists in the chat window.
     *
     * @param username The username we expect to appear in the offline message
     * @return true if the offline message appears, false otherwise
     */
    public boolean verifyOfflineMessage(String username) {
        return verifyPresenceMessage(username, "offline");
    }

    /**
     * Verify the 'User is now online' message exists in the chat window.
     *
     * @param username The username we expect
     * @return true if the online message appears, false otherwise
     */
    public boolean verifyOnlineMessage(String username) {
        return verifyPresenceMessage(username, "online");
    }

    /**
     * Verifies a presence message appears depending on the user's status.
     *
     * @param username The username we want to check for in the message
     * @param status The status we expect to appear of [online, offline, away].
     * @return true if the status message appears in the chat window, false
     * otherwise
     */
    private boolean verifyPresenceMessage(String username, String status) {
        String expectedMessage = null;
        if (status.equals("online")) {
            expectedMessage = username + " is now online";
        } else if (status.equals("away")) {
            expectedMessage = username + " is away";
        } else if (status.equals("offline")) {
            expectedMessage = username + " is currently offline. Any messages you send will be delivered when " + username + " comes online.";
        }

        List<WebElement> allStatus = driver.findElements(FRIEND_STATUS_LOCATOR);

        for (WebElement statusBubble : allStatus) {
            if (statusBubble.getText().equals(expectedMessage)) {
                return true;
            }
        }

        return false;
    }

    /**
     * Verifies the presence bubble indicates the correct status.
     *
     * @param status The presence to expect
     * @return true if the bubble matches the status, false otherwise
     */
    public boolean verifyPresenceDot(com.ea.originx.automation.lib.pageobjects.social.ChatWindow.Presence status) {
        String statusStr = "otkpresence-";
        switch (status) {
            case ONLINE:
                statusStr += "online";
                break;
            case INGAME:
                statusStr += "ingame";
                break;
            case BROADCASTING:
                statusStr += "broadcasting";
                break;
            case JOINABLE:
                statusStr += "joinable";
                break;
            case AWAY:
                statusStr += "away";
                break;
            case OFFLINE:
                statusStr += "offline";
                break;
        }
        final WebElement statusDot = waitForElementPresent(FRIEND_STATUS_BUBBLE_LOCATOR);
        final boolean hasCorrectStatus = statusDot.getAttribute("class").contains(statusStr);
        return hasCorrectStatus;
    }

    /**
     * While the user is offline, verifies a message appears indicating the user
     * is offline in the chat window.
     *
     * @return true if the status message appears in the chat window, false
     * otherwise
     */
    public boolean verifyOfflineSystemMessage() {
        final String expectedMessage = "You are offline";
        final WebElement offlineMessage = waitForElementVisible(OFFLINE_NOTIFICATION_LOCATOR);
        return offlineMessage.getText().equals(expectedMessage);
    }

    /**
     * While the user is offline, check that the chat box is disabled.
     *
     * @return true if the status message appears in the chat window, false
     * otherwise
     */
    public boolean verifyOfflineChatDisabled() {
        final WebElement chatbox = driver.findElement(TEXT_INPUT_AREA_LOCATOR);
        final boolean isChatDisabled = !chatbox.isEnabled();
        return isChatDisabled;
    }

    /**
     * Verify the placeholder text in the chat box.
     *
     * @param expected The text to check for ("" for no placeholder)
     *
     * @return true if the chat box has no placeholder text, false otherwise
     */
    public boolean verifyChatPlaceholderIs(String expected) {
        final WebElement chatbox = driver.findElement(TEXT_INPUT_AREA_LOCATOR);
        final boolean matchPlaceholderText = chatbox.getAttribute("placeholder").equals(expected);
        return matchPlaceholderText;
    }

    /**
     * Count the number of timestamps that appear in the chat box.
     *
     * @return The count of the timestamps
     */
    protected int countTimestamps() {
        final List<WebElement> timestamps = driver.findElements(TIMESTAMP_LOCATOR);
        return timestamps.size();
    }

    /**
     * Verify the time stamp count is x.
     *
     * @param x The number of timestamps expected
     * @return true if and only if the count is equal to x, false otherwise
     */
    public boolean verifyTimestampCountIs(int x) {
        return countTimestamps() == x;
    }

    /**
     * Verify the time stamp count is at least x.
     *
     * @param x The number of timestamps expected
     * @return True if and only if the count is at least x
     */
    public boolean verifyTimestampCountAtLeast(int x) {
        return countTimestamps() >= x;
    }

    /**
     * Verify a set of messages appear in a single chat bubble.
     *
     * @param messages An array of messages to look for in a single chat bubble
     * @return true if all the messages were found within a single chat bubble,
     * false otherwise
     */
    public boolean verifyMessagesInBubble(String[] messages) {
        List<WebElement> allChatBubbles = driver.findElements(CONVERSATION_BUBBLE_LOCATOR);
        boolean messagesFound = false;

        for (WebElement chatBubble : allChatBubbles) {
            if (chatBubble.getText().contains(messages[0])) {
                messagesFound = true;
                for (String message : messages) {
                    if (!chatBubble.getText().contains(message)) {
                        messagesFound = false;
                        break;
                    }
                }
                if (messagesFound) {
                    break;
                }
            }
        }

        return messagesFound;
    }

    /**
     * Verifies the given avatar string matches the avatar of the given user in
     * a chat window.
     *
     * @param avatarSrc The string that represents the users avatar.
     * @param username The username we are verifying the avatar against.
     * @return true if we can find an avatar that has the username and avatarSrc
     * matching, false otherwise.
     */
    public boolean verifyAvatarMatches(String avatarSrc, String username) {
        boolean match = false;

        final WebElement chatHead = waitForElementPresent(CHAT_WINDOW_LOCATOR);
        final List<WebElement> allAvatars = chatHead.findElements(AVATAR_LOCATOR);

        // Remove the last part of the url, which is just the size of the avatar.
        String[] splitURL = avatarSrc.split("/");
        avatarSrc = String.join("/", Arrays.copyOfRange(splitURL, 0, splitURL.length - 1));

        for (WebElement anAvatar : allAvatars) {
            if (anAvatar.getAttribute("alt").equals(username)
                    && anAvatar.getAttribute("src").contains(avatarSrc)) {
                match = true;
                break;
            }
        }

        return match;
    }

    /**
     * Verify that there is a conversation banner visible in the chat window.
     *
     * @return true if a conversation banner is visible
     */
    public boolean verifyBannerVisible() {
        return waitIsElementVisible(CONVERSATION_BANNER_LOCATOR);
    }

    /**
     * Verify that the game title in the conversation banner matches the
     * expected game title.
     *
     * @param expectedText The game title that is expected to appear in the
     * conversation banner
     * @return true if the game title in the conversation banner matches the
     * expected game title, false otherwise
     */
    public boolean verifyBannerGameTitle(String expectedText) {
        String gameBannerText = waitForElementVisible(CONVERSATION_BANNER_GAME_TITLE_LOCATOR).getText();
        return expectedText.equals(gameBannerText);
    }

    /**
     * Verify that the conversation history is placed properly. If there is a
     * conversation banner displayed, then the conversation banner should be
     * immediately below the chat history, with no overlap. If there is not a
     * conversation banner displayed, then the text input area should be
     * immediately below the chat history, with no overlap.
     *
     * @return true if the chat history is displayed in the correct location,
     * false otherwise
     */
    public boolean verifyChatHistoryPosition() {
        WebElement conversationHistory = waitForElementVisible(CONVERSATION_HISTORY_LOCATOR);

        WebElement elementBelowConversationHistory = waitIsElementVisible(CONVERSATION_BANNER_LOCATOR)
                ? waitForElementVisible(CONVERSATION_BANNER_LOCATOR)
                : waitForElementVisible(TEXT_INPUT_AREA_LOCATOR);

        int conversationHistoryBottomPosition = conversationHistory.getLocation().getY() + conversationHistory.getSize().getHeight();
        int elementBelowTopPosition = elementBelowConversationHistory.getLocation().getY();

        //Return true if the top of the element that is supposed to be below the
        //conversation history is exactly 1 pixel below the conversation history
        return conversationHistoryBottomPosition + 1 == elementBelowTopPosition;
    }

    /**
     * Shifts focus to OIG chat window before verify the chat window.
     *
     * @return true if the chat window is open, false otherwise
     */
    public boolean verifyOIGChatOpen() {
        Waits.waitForPageThatMatches(driver, ".*social-chatwindow.*", 60);
        return verifyChatOpen();
    }

    /**
     * Close the download manager window in the OIG.
     */
    public void clickCloseButtonOnOigChatWindow() {
        qtChatWindow.clickXButton();
    }

    /**
     * Switches to the handle of the popped out chat window
     *
     * @param driver Selenium WebDriver
     */
    public void switchToPoppedOutChatWindow(WebDriver driver) {
        Waits.waitForWindowHandlesToStabilize(driver, 30);
        Waits.switchToPageThatMatches(driver, CHAT_WINDOW_POPPED_OUT_REGEX);
    }

    /**
     * Verify that the chat window is popped out
     * Note that this function switches the active window to the chat window
     *
     * @param driver Selenium WebDriver
     * @return true if the chat window is popped out, false otherwise
     */
    public boolean verifyChatWindowPoppedOut(WebDriver driver) {
        try {
            switchToPoppedOutChatWindow(driver);
        } catch (RuntimeException e) {
            return false;
        }
        return waitIsElementVisible(CHAT_WINDOW_POPPED_OUT_LOCATOR);
    }

    /**
     * Pop the chat window back in
     *
     */
    public void popChatWindowBackIn() {
        waitForElementClickable(CHAT_WINDOW_POPPED_IN_LOCATOR).click();
    }

    /**
     * Close the popped in chat window
     *
     */
    public void closePoppedInChatWindow() {
        waitForElementClickable(CHAT_WINDOW_POPPED_IN_CLOSE_LOCATOR).click();
    }

    /**
     * Type the message in the text area in order to check on another client that the 3 dots appear,
     * indicating that a message is being typed
     *
     * @param message the string to type
     */
    public void typeMessageWithoutEnter(String message) {
        WebElement textArea = waitForElementClickable(TEXT_INPUT_AREA_LOCATOR);
        textArea.sendKeys(message);
    }

    /**
     * Click enter in the chat window
     */
    public void typeEnter() {
        WebElement textArea = waitForElementClickable(TEXT_INPUT_AREA_LOCATOR);
        textArea.sendKeys(Keys.RETURN);
    }

    /**
     * Verifies that typing dots in a bubble appear when the user is being sent a message
     *
     * @return true if the typing dots are visible when the user is being sent a message
     */
    public boolean verifyTypingDotsVisible() {
        return waitIsElementVisible(TYPING_DOTS_LOCATOR);
    }

    /**
     * Minimize the chat window when it is popped out
     *
     * @param driver Selenium Web Driver (must be of type qtWebDriver)
     */
    public void minimizePoppedOutChatWindow(WebDriver driver) {
        switchToWindowWithUrlThatHasElement(driver, QT_CHAT_WINDOW_POPPED_OUT_URL, QT_POPPED_OUT_WINDOW_LOCATOR);
        driver.findElement(QT_POPPED_OUT_WINDOW_MINIMIZE_LOCATOR).click();
    }

    /**
     * Close the chat window when it is popped out
     *
     * @param driver Selenium Web Driver (must be of type qtWebDriver)
     */
    public void closePoppedOutChatWindow(WebDriver driver) {
        switchToWindowWithUrlThatHasElement(driver, QT_CHAT_WINDOW_POPPED_OUT_URL, QT_POPPED_OUT_WINDOW_LOCATOR);
        driver.findElement(QT_POPPED_OUT_WINDOW_CLOSE_LOCATOR).click();
    }

    /**
     *  Verify that the popped out chat window has been successfully minimized
     *  Note that the javascript must be executed from the 'Main SPA' page otherwise it will not work
     *
     * @param driver Selenium Web Driver
     * @return true if the popped out chat window is minimized, false otherwise
     */
    public boolean verifyPoppedOutChatWindowMinimized(WebDriver driver) {
        driver.manage().timeouts().setScriptTimeout(5, TimeUnit.SECONDS); // necessary for slower executions of executeAsyncScript()
        // Note that the isMiniaturized function takes the UUID of the popped out ChatWindow
        // The script to be executed must be asynchronous since it returns a promise after a few seconds
        // Promise value returns a boolean if the window is minimized or not
        Object isMinimized = ((JavascriptExecutor) driver).executeAsyncScript("var callback = arguments[arguments.length-1]; " +
                "window.Origin.client.desktopServices.isMiniaturized(OriginComponents._factories.ChatWindowFactory.poppedOutChatWindow.OriginWindowUUID).then(function(result)" +
                "{callback(result);});");
        _log.debug("isMinimized: " + isMinimized.toString());
        return Boolean.parseBoolean(isMinimized.toString());
    }
    
    /**
     * Verify that the Voice Chat Settings button (the cogwheel button) is visible in the Chat Window
     * 
     * @param driver
     * @return true if the button is present, false otherwise
     */
    public boolean verifyVoiceChatSettingsButtonIsVisible(WebDriver driver) {
        return waitIsElementVisible(driver.findElement(VOICE_CHAT_SETTINGS_LOCATOR), 10);
    }
    
    /**
     * Click on the Voice Chat Settings button (the cogwheel button)
     * 
     * @param driver 
     */
    public void clickVoiceChatSettingsButton(WebDriver driver) {
        driver.findElement(VOICE_CHAT_SETTINGS_LOCATOR).click();
    }
}