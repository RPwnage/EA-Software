package com.ea.originx.automation.lib.pageobjects.discover;

import com.ea.vx.originclient.account.UserAccount;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.vx.originclient.utils.Waits;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;
import org.openqa.selenium.By;
import org.openqa.selenium.NoSuchElementException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the 'Recommended Friends' section on the 'My Home' page.
 *
 * @author palui
 */
public class RecFriendsSection extends EAXVxSiteTemplate {

    private final WebElement rootElement; // bucket for 'Recommended Friends' section

    // The following css selectors are for child elements within the rootElement
    private static final String SECTION_CSS = ".origin-home-section";
    private static final String SECTION_TITLE_CSS = SECTION_CSS + " .origin-home-section-title";
    private static final By SECTION_TITLE_LOCATOR = By.cssSelector(SECTION_TITLE_CSS);

    private static final String REC_FRIENDS_TILES_CSS = SECTION_CSS
            + " .origin-home-discovery-tiles .l-origin-home-discovery-tile origin-home-discovery-tile-recfriends";

    private static final By REC_FRIENDS_TILES_LOCATOR = By.cssSelector(REC_FRIENDS_TILES_CSS);

    private static final String REC_FRIENDS_TILE_CSS_TMPL = REC_FRIENDS_TILES_CSS + "[userid='%s']";

    private static final String VIEW_MORE_BUTTON_CSS = SECTION_CSS + " .l-origin-home-section-more > a.otkbtn";
    private static final By VIEW_MORE_BUTTON_LOCATOR = By.cssSelector(VIEW_MORE_BUTTON_CSS);

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     * @param rootElement Root element of this 'Recommended Friends' tile
     */
    public RecFriendsSection(WebDriver driver, WebElement rootElement) {
        super(driver);
        this.rootElement = rootElement;
    }

    /**
     * Verify the 'Recommended Friends' section is visible.
     *
     * @return true if this 'Recommended Friends' section is visible, false
     * otherwise
     */
    public boolean verifyRecFriendsSectionVisible() {
        return waitIsElementVisible(rootElement, 2);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Child element utilities
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Get the child element by its locator.
     *
     * @param childLocator Child locator within rootElement of this 'Recommended
     * Friends' tile
     * @return Child WebElement if found, or throw NoSuchElementException
     */
    private WebElement getChildElement(By childLocator) {
        return rootElement.findElement(childLocator);
    }

    /**
     * Get the child element's text.
     *
     * @param childLocator Child locator within rootElement of this 'Recommended
     * Friends' tile
     * @return Child WebElement text if found, or null if not found
     */
    private String getChildElementText(By childLocator) {
        try {
            return getChildElement(childLocator).getAttribute("textContent");
        } catch (NoSuchElementException e) {
            return null;
        }
    }

    /**
     * Click child WebElement as specified by the child locator.
     *
     * @param childLocator child locator within rootElement of this 'Recommend
     * Friends' tile
     */
    private void clickChildElement(By childLocator) {
        waitForElementClickable(getChildElement(childLocator)).click();
    }

    /**
     * Verify the child element is visible by it's locator.
     *
     * @param childLocator child locator within rootElement of this 'Recommended
     * Friends' tile
     * @return true if childElement is visible, false otherwise
     */
    private boolean verifyChildElementVisible(By childLocator) {
        return waitIsChildElementVisible(rootElement, childLocator, 2);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Section operations
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Get the section title for the 'Recommended Friends' section.
     *
     * @return Title String for the 'Recommended Friends' section, or throw
     * TimeoutException
     */
    public String getSectionTitle() {
        return waitForElementVisible(SECTION_TITLE_LOCATOR).getAttribute("textContent");
    }

    /**
     * Verify the 'View More' button is visible.
     *
     * @return true if the 'View More' button is visible, false otherwise
     */
    public boolean verifyViewMoreButtonVisible() {
        return verifyChildElementVisible(VIEW_MORE_BUTTON_LOCATOR);
    }

    /**
     * Click 'View More' button, or throw TimeoutException if not found.
     */
    public void clickViewMoreButton() {
        clickChildElement(VIEW_MORE_BUTTON_LOCATOR);
    }

    ////////////////////////////////////////////////////////////////////////////
    // 'Recommended Friends' tile getters and verifiers
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Expand the 'Recommended Friends' tiles until 'View More' button is no
     * longer visible.
     */
    public void expandRecFriendsTiles() {
        int i = 0;
        while (verifyViewMoreButtonVisible()) {
            clickViewMoreButton();
            sleep(1000); // pause for page to refresh
            waitForAnimationComplete(VIEW_MORE_BUTTON_LOCATOR);
        }
    }

    /**
     * Get all 'Recommended Friends' tile WebElements.
     *
     * @return List of all 'Recommended Friends' tile WebElements on this
     * 'Discover' page section
     */
    private List<WebElement> getAllRecFriendsTileElements() {
        int nTries = 10;
        List<WebElement> recFriendsTileElements = new ArrayList<>();
        expandRecFriendsTiles();
        // Retry as we may get an empty list if we retrieve the tiles too soon
        for (int i = 0; i < nTries; i++) {
            recFriendsTileElements = rootElement.findElements(REC_FRIENDS_TILES_LOCATOR);
            if (!recFriendsTileElements.isEmpty()) {
                break;
            }
            sleep(1000); // gap each iteration to allow tiles to finish update
        }
        return recFriendsTileElements;
    }

    /**
     * Get a 'Recommended Friends' tile element WebElement given the Nucleus ID
     * of the recommended friend.
     *
     * @param nucleusId Nucleus ID of the recommended friend
     * @return 'Recommend Friends' tile WebElement for the given nucleusId on
     * the 'Recommend Friends' section, or throw NoSuchElementException
     */
    private WebElement getRecFriendsTileElement(String nucleusId) {
        By tileLocator = By.cssSelector(String.format(REC_FRIENDS_TILE_CSS_TMPL, nucleusId));
        return getChildElement(tileLocator);
    }

    /**
     * Get the RecFriendsTile element given the Nucleus ID of the recommended friend.
     *
     * @param nucleusId The Nucleus ID of the recommended friend
     * @return 'Recommended Friends' tile for the given nucleusId on this
     * 'Discover' page section, or throw NoSuchElementException
     */
    public RecFriendsTile getRecFriendsTile(String nucleusId) {
        return new RecFriendsTile(driver, getRecFriendsTileElement(nucleusId));
    }

    /**
     * Get the RecFriendsTile element given the user account of the recommended friend.
     *
     * @param recFriend User account of user to display in a 'Recommend Friends' tile
     * @return 'Recommended Friends' tile for the given user on this 'Discover'
     * page section, or throw NoSuchElementException if not found
     */
    public RecFriendsTile getRecFriendsTile(UserAccount recFriend) {
        return getRecFriendsTile(recFriend.getNucleusUserId());
    }

    /**
     * Get the RecFriendsTiles elements given a list of user accounts of the
     * recommended friends.
     *
     * @param recFriends Accounts of users to display in the 'Recommend Friends'
     * tiles
     * @return 'Recommended Friends' tiles for the given users on this
     * 'Discover' page section, or throw NoSuchElementException if not all found
     */
    public List<RecFriendsTile> getRecFriendsTiles(UserAccount... recFriends) {
        if (recFriends == null) {
            throw new RuntimeException("List of nucleusIds must not be null");
        }
        List<RecFriendsTile> recFriendsTiles = new ArrayList<>();
        for (UserAccount recFriend : recFriends) {
            recFriendsTiles.add(getRecFriendsTile(recFriend.getNucleusUserId()));
        }
        return recFriendsTiles;
    }

    /**
     * Get all the RecFriendsTiles in the 'Recommended Friends' section.
     *
     * @return List of all 'Recommend Friends' tiles on this 'Discover' page
     * section
     */
    public List<RecFriendsTile> getAllRecFriendsTiles() {
        List<RecFriendsTile> recFriendsTiles = getAllRecFriendsTileElements().
                stream().map(webElement -> new RecFriendsTile(driver, webElement)).collect(Collectors.toList());
        return recFriendsTiles;
    }

    /**
     * Verify that a tile exists given a user account.
     *
     * @param recFriend Account to check it is shown in a 'Recommend Friends'
     * tile
     * @return true if the 'Recommend Friends' tile for the user exists in this
     * 'Discover' page section, false otherwise
     */
    public boolean verifyTileExist(UserAccount recFriend) {
        try {
            getRecFriendsTileElement(recFriend.getNucleusUserId());
        } catch (NoSuchElementException e) {
            return false;
        }
        return true;
    }

    /**
     * Polling wait to check if a user is shown in a 'Recommend Friends' tile.
     *
     * @param recFriend User account to check
     * @return true if the 'Recommend Friends' tile for the user exists in this
     * section
     */
    public boolean waitVerifyTileExist(UserAccount recFriend) {
        return Waits.pollingWait(() -> verifyTileExist(recFriend));
    }

    /**
     * Verify if a list of recommended friends all exist in this 'Discover' page
     * section.
     *
     * @param recFriends Accounts of users to display in the 'Recommend Friends'
     * tiles
     * @return true if all users appear in the list of 'Recommend Friends'
     * tiles, false otherwise
     */
    public boolean verifyTilesExist(UserAccount... recFriends) {
        if (recFriends == null) {
            throw new RuntimeException("List of recommended friends must not be null");
        }
        for (UserAccount recFriend : recFriends) {
            if (!verifyTileExist(recFriend)) {
                return false;
            }
        }
        return true;
    }
}