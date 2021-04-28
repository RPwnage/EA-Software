package com.ea.originx.automation.lib.pageobjects.profile;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import java.util.ArrayList;
import java.util.List;

/**
 * Page object that represents the 'Games' tab on the 'Profile' page.
 *
 * @author lscholte
 */
public class ProfileGamesTab extends EAXVxSiteTemplate {

    protected static final String GAMES_TAB_CSS = "div.origin-profile-tab.origin-profile-games-tab";
    protected static final String MY_GAMES_VIEW_ALL_BUTTON_CSS =".origin-profile-games-tab .show-more-games";
    protected static final By MY_GAMES_VIEW_ALL_BUTTON = By.cssSelector(MY_GAMES_VIEW_ALL_BUTTON_CSS);
    protected static final By GAMES_TAB_NO_GAMES = By.cssSelector(GAMES_TAB_CSS + " .otktitle-2.header-text");
    protected static final By GAMES_TAB_HEADER = By.cssSelector(GAMES_TAB_CSS + " .otktitle-3.row-header");
    protected static final String MY_GAMES_HEADER_XPATH = "//h2[contains(text(), 'My games')]";
    protected static final String GAMES_BOTH_OWN_HEADER_XPATH = "//h2[contains(text(), 'both have')]";
    protected static final String OTHER_GAMES_HEADER_XPATH = "//h2[contains(text(), 'Other games')]";
    protected static final By MY_GAMES_HEADER_LOCATOR = By.xpath(MY_GAMES_HEADER_XPATH);
    protected static final By GAMES_BOTH_OWN_HEADER_LOCATOR = By.xpath(GAMES_BOTH_OWN_HEADER_XPATH);
    protected static final By OTHER_GAMES_HEADER_LOCATOR = By.xpath(OTHER_GAMES_HEADER_XPATH);
    protected static final By GAMES_BOTH_OWN_TILES_LOCATOR = By.xpath(GAMES_BOTH_OWN_HEADER_XPATH + "/..//ORIGIN-PROFILE-PAGE-GAMES-GAMETILE");
    protected static final By OTHER_GAMES_TILES_LOCATOR = By.xpath(OTHER_GAMES_HEADER_XPATH + "/..//ORIGIN-PROFILE-PAGE-GAMES-GAMETILE");
    protected static final By GAMES_BOTH_OWN_VIEW_ALL_LOCATOR = By.xpath(GAMES_BOTH_OWN_HEADER_XPATH + "/..//a[contains(@class, 'show-more-games')]");
    protected static final By OTHER_GAMES_VIEW_ALL_LOCATOR = By.xpath(OTHER_GAMES_HEADER_XPATH + "/..//A[contains(@class, 'show-more-games')]");
    protected static final By ALL_VISIBLE_ENTITLEMENTS = By.cssSelector(GAMES_TAB_CSS + " origin-profile-page-games-gametile");
    protected static final By ALL_LEARN_MORE_BUTTONS = By.cssSelector(GAMES_TAB_CSS + " .origin-profile-games-gametile-info-button");

    public ProfileGamesTab(WebDriver driver) {
        super(driver);
    }

    /**
     * Verifies that the 'No games' message is displayed and that it is correct.
     *
     * @param currentUserProfile True if the displayed profile page is for the
     * current user. False if the displayed profile page is for another user
     * @return true if the no games message is displayed and correct, false
     * otherwise
     */
    public boolean verifyNoGamesMessage(boolean currentUserProfile) {
        WebElement noGamesMessage;
        
        try {
            noGamesMessage = waitForElementVisible(GAMES_TAB_NO_GAMES);
        } catch (TimeoutException e) {
            return false;
        }
        
        final String expectedMessage = currentUserProfile
                ? "You don't have any games yet."
                : "This person doesn't have any games yet.";

        return noGamesMessage.isDisplayed() && expectedMessage.equals(noGamesMessage.getText());
    }

    /**
     * Get the 'No Games' message text.
     *
     * @return The 'My Games' header text
     */
    public String getNoGamesMessageText() {
        return waitForElementVisible(GAMES_TAB_NO_GAMES).getText();
    }

    /**
     * Get 'My Games' header text.
     *
     * @return The 'My Games' header text
     */
    public String getMyGamesHeaderText() {
        return waitForElementVisible(GAMES_TAB_HEADER).getText();
    }

    /**
     * Checks to see if the 'Games Both Own' header appears.
     *
     * @return true if 'Games Both Own' header appears
     */
    public boolean verifyBothGamesOwnHeader() {
        return waitIsElementVisible(GAMES_BOTH_OWN_HEADER_LOCATOR);
    }

    /**
     * Checks to see if the 'Other Games Own' header appears.
     *
     * @return true if 'Other Games Own' header appears
     */
    public boolean verifyOtherGamesHeader() {
        return waitIsElementVisible(OTHER_GAMES_HEADER_LOCATOR);
    }

    /**
     * Verify that the 'Games You Both Own' header displays the correct number
     * of games.
     *
     * @param expectedNumberDisplayed The expected number of displayed games
     * @param expectedNumberTotal The expected number of total games
     * @return true if the header contains the correct number of displayed and
     * total games, false otherwise
     */
    public boolean verifyGamesBothOwnHeaderNumber(int expectedNumberDisplayed, int expectedNumberTotal) {
        return verifyGamesHeaderNumber(GAMES_BOTH_OWN_HEADER_LOCATOR, expectedNumberDisplayed, expectedNumberTotal);
    }

    /**
     * Verify that the 'Other Games' header displays the correct number of games.
     *
     * @param expectedNumberDisplayed The expected number of displayed games
     * @param expectedNumberTotal The expected number of total games
     * @return true if the header contains the correct number of displayed and
     * total games, false otherwise
     */
    public boolean verifyOtherGamesHeaderNumber(int expectedNumberDisplayed, int expectedNumberTotal) {
        return verifyGamesHeaderNumber(OTHER_GAMES_HEADER_LOCATOR, expectedNumberDisplayed, expectedNumberTotal);
    }

    /**
     * Verify that the 'My Games' header displays the correct number of games.
     *
     * @param expectedNumberDisplayed The expected number of displayed games
     * @param expectedNumberTotal The expected number of total games
     * @return true if the header contains the correct number of displayed and
     * total games, false otherwise
     */
    public boolean verifyMyGamesHeaderNumber(int expectedNumberDisplayed, int expectedNumberTotal) {
        return verifyGamesHeaderNumber(MY_GAMES_HEADER_LOCATOR, expectedNumberDisplayed, expectedNumberTotal);
    }

    /**
     * Verify that the 'Games you both own' header displays the correct number of games.
     *
     * @param expectedNumberDisplayed The expected number of displayed games
     * @param expectedNumberTotal The expected number of total games
     * @return true if the header contains the correct number of displayed and
     * total games, false otherwise
     */
    public boolean verifyGamesYouBothOwnHeaderNumbers(int expectedNumberDisplayed, int expectedNumberTotal) {
        return verifyGamesHeaderNumber(GAMES_BOTH_OWN_HEADER_LOCATOR, expectedNumberDisplayed, expectedNumberTotal);
    }

    /**
     * Verify that the games header contains the correct count for the number of games currently displayed.
     *
     * @param headerLocator The locator for the games header
     * @param expectedNumberDisplayed The expected number of displayed games
     * @return true if the header contains the correct number of displayed games
     */
    private boolean verifyGamesHeaderDisplayCountNumber(By headerLocator, int expectedNumberDisplayed){
        return waitForElementVisible(headerLocator).getText().contains(expectedNumberDisplayed + " of ");
    }

    /**
     * Verify that the 'Games you both own' header contains the correct count for the number of games currently
     * displayed.
     *
     * @return true if the header contains the correct number of displayed games
     */
    public boolean verifyGamesYouBothOwnHeaderDisplayCountNumber(){
        return verifyGamesHeaderDisplayCountNumber(GAMES_BOTH_OWN_HEADER_LOCATOR, getNumberOfGamesDisplayed());
    }

    /**
     * Verify that the games header contains the correct number of games.
     *
     * @param headerLocator The locator for the games header
     * @param expectedNumberDisplayed The expected number of displayed games
     * @param expectedNumberTotal The expected number of total games
     * @return true if the header contains the correct number of displayed and
     * total games, false otherwise
     */
    private boolean verifyGamesHeaderNumber(By headerLocator, int expectedNumberDisplayed, int expectedNumberTotal) {
        WebElement header = waitForElementVisible(headerLocator, 10);
        return header.getText().contains(expectedNumberDisplayed + " of " + expectedNumberTotal);
    }

    /**
     * Gets the number of games currently displayed on the user's 'My Games' section in the profile games tab
     *
     * @return the number of displayed game tiles
     */
    public int getNumberOfGamesDisplayed(){
        return driver.findElements(ALL_VISIBLE_ENTITLEMENTS).size();
    }

    /**
     * Verify that the games in the 'Games You Both Own' section are in
     * alphabetical order.
     *
     * @return true if all the games in the 'Games You Both Own' section are in
     * alphabetical order. False if at least one game is out of order.
     */
    public boolean verifyGamesBothOwnAlphabeticalOrder() {
        List<WebElement> games = waitForAllElementsVisible(GAMES_BOTH_OWN_TILES_LOCATOR);
        return verifyGamesAlphabeticalOrder(games);
    }

    /**
     * Verify that the games in the 'Other Games' section are in alphabetical
     * order.
     *
     * @return true if all the games in the 'Other Games' section are in
     * alphabetical order. False if at least one game is out of order.
     */
    public boolean verifyOtherGamesAlphabeticalOrder() {
        List<WebElement> games = waitForAllElementsVisible(OTHER_GAMES_TILES_LOCATOR);
        return verifyGamesAlphabeticalOrder(games);
    }

    /**
     * Verify that the list of games are in alphabetical order.
     *
     * @param games The list of games to verify as being in alphabetical order
     * @return true if all games are in alphabetical order. False if at least
     * one game is out of order.
     */
    private boolean verifyGamesAlphabeticalOrder(List<WebElement> games) {
        for (int i = 0; i < games.size() - 1; ++i) {
            String currentGame = games.get(i).getAttribute("titlestr").toLowerCase();
            String nextGame = games.get(i + 1).getAttribute("titlestr").toLowerCase();
            if (currentGame.compareTo(nextGame) > 0) {
                return false;
            }
        }
        return true;
    }

    /**
     * Verify that the 'View All' button is visible in the 'Games You Both Own'
     * section.
     *
     * @return true if the 'View All' button is visible, false otherwise
     */
    public boolean verifyGamesBothOwnViewAllVisible() {
        return waitIsElementVisible(GAMES_BOTH_OWN_VIEW_ALL_LOCATOR);
    }

    /**
     * Verify that the 'View All' button is visible in the 'Other Games' section.
     *
     * @return true if the 'View All' button is visible, false otherwise
     */
    public boolean verifyOtherGamesViewAllVisible() {
        return waitIsElementVisible(OTHER_GAMES_VIEW_ALL_LOCATOR);
    }

    /**
     * Verifies that the 'View All' button is visible in the user's 'My Games' section of their profile games tab
     *
     * @return true if the 'View All' button is visible, false otherwise
     */
    public boolean verifyMyGamesViewAllButtonVisible(){
        return waitIsElementVisible(MY_GAMES_VIEW_ALL_BUTTON);
    }

    /**
     * Clicks the 'View All' button in the 'My Games' section of the user's profile games tab.
     */
    public void clickMyGamesViewAllButton(){
        waitForElementClickable(MY_GAMES_VIEW_ALL_BUTTON).click();
    }

    /**
     * Verifies that an entitlement exists in the 'Games' tab.
     *
     * @param entitlementName The name of the entitlement to look for
     * @return true if the entitlement was found, false otherwise
     */
    public boolean verifyEntitlementExists(String entitlementName) {
        if(verifyMyGamesViewAllButtonVisible()) {
            clickMyGamesViewAllButton();
        }
        List<WebElement> allEntitlements = driver.findElements(ALL_VISIBLE_ENTITLEMENTS);
        for (WebElement entitlement : allEntitlements) {
            if (entitlement.getAttribute("titlestr").equalsIgnoreCase(entitlementName)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Verifies that all entitlements have a 'Learn More' button.
     *
     * @param expectedNumber The expected number of entitlements with a 'Learn
     * More' button
     * @return true if the number of entitlements with a 'Learn More' button
     * matches the expected number, false otherwise
     */
    public boolean verifyAllEntitlementsHaveLearnMore(int expectedNumber) {
        List<WebElement> allLearnMoreLinks = driver.findElements(ALL_LEARN_MORE_BUTTONS);
        return allLearnMoreLinks.size() == expectedNumber;
    }

    /**
     * Get all game tile's titles.
     *
     * @param locator Locator of game tiles
     * @return A list of all the game tile's titles
     */
    private List<String> getGameTileTitles(By locator){
        List<WebElement> gameTiles = waitForAllElementsVisible(locator);
        List<String> gameTitles = new ArrayList<>();
        gameTiles.stream().forEach((gameTile) -> {
            gameTitles.add(gameTile.getAttribute("titlestr"));
        });
        return gameTitles;
    }

    /**
     * Gets the list of game titles from the 'Games Both Owned' section of the 'Games' tab.
     * 
     * @return List of Strings that represent the titles in 'Games Both Owned'
     */
    public List<String> getBothOwnGamesTitles(){
        return getGameTileTitles(GAMES_BOTH_OWN_TILES_LOCATOR);
    }

    /**
     * Gets the list of game titles from the 'Other Games' section of the 'Games' tab.
     * 
     * @return List of strings that represent the titles in 'Other Games'
     */
    public List<String> getOtherGamesTitles(){
        return getGameTileTitles(OTHER_GAMES_TILES_LOCATOR);
    }

    /**
     * Gets the list of game title titles for all the games that are displayed in the 'My Games' section of the profile
     * games tab.
     *
     * @return List of game titles
     */
    public List<String> getMyGamesGameTileTitles(){
        if(verifyMyGamesViewAllButtonVisible()) {
            clickMyGamesViewAllButton();
        }
        return getGameTileTitles(ALL_VISIBLE_ENTITLEMENTS);
    }
}