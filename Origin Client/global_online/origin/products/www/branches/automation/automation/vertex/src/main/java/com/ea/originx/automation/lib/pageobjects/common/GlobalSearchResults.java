package com.ea.originx.automation.lib.pageobjects.common;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.utils.Waits;
import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

import java.util.List;
import java.util.stream.Collectors;

/**
 * A page object representing the search results after performing a global
 * search.
 *
 * @author lscholte
 * @author micwang
 */
public class GlobalSearchResults extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(GlobalSearchResults.class);

    protected static final By GLOBAL_SEARCH_RESULTS_LOCATOR = By.cssSelector("div.origin-search-results");

    // No Results
    protected static final String FEATURED_GAMES_CAROUSEL_CSS = ".origin-search-no-results-carousel ";
    protected static final String FEATURED_GAMES_CAROUSEL_TITLE = "Popular";
    protected static final By NO_RESULTS_LOCATOR = By.cssSelector(".origin-search-no-results-main .origin-search-no-results-title ");
    protected static final By FEATURED_GAMES_CAROUSEL_TILES_LOCATOR = By.cssSelector(FEATURED_GAMES_CAROUSEL_CSS + ".l-origin-storegametiles-fixed li");
    protected static final By FEATURED_GAMES_CAROUSEL_TILES_IMAGE_LOCATOR = By.cssSelector(FEATURED_GAMES_CAROUSEL_CSS + ".l-origin-storegametiles-fixed img");
    protected static final By FEATURED_GAMES_CAROUSEL_TITLE_LOCATOR = By.cssSelector(FEATURED_GAMES_CAROUSEL_CSS + ".origin-store-carousel-featured-carousel-caption h2");

    // Game Library Results
    protected static final String GAME_LIBRARY_RESULTS = "origin-search-results[results='gameResults']";
    protected static final By GAME_LIBRARY_RESULTS_LOCATOR = By.cssSelector(GAME_LIBRARY_RESULTS);
    protected static final String GAME_LIBRARY_RESULTS_LIST = GAME_LIBRARY_RESULTS + " ul";
    protected static final By GAME_LIBRARY_RESULTS_LIST_LOCATOR = By.cssSelector(GAME_LIBRARY_RESULTS_LIST);
    protected static final By GAMES_LIST_LOCATOR = By.cssSelector(GAME_LIBRARY_RESULTS_LIST + " li");
    protected static final By GAMETILE_TITLE_LOCATOR = By.cssSelector(".origin-gametile > a > img");
    protected static final String GAMETILE_TITLE_TMPL = ".origin-gametile > a > img[alt='%s']";
    protected static final By GAME_LIBRRARY_VIEW_ALL_LOCATOR = By.cssSelector("origin-search-results[results='gameResults'] .origin-search-view-more a");
    protected static final By GAME_LIBRARY_DETAILS_BUTTON = By.xpath("//*[contains(text(), 'Details')]");

    // Store Results
    protected static final By STORE_RESULTS_LOCATOR = By.cssSelector("origin-search-results[results='storeResults']");
    protected static final By STORETILE_TITLE_LOCATOR = By.cssSelector("otkex-hometile .home-tile-header");
    protected static final String STORETILE_TITLE_TMPL = ".origin-storegametile-details > a[title='%s']";
    protected static final String STORETILE_BY_NAME_XPATH_TMPL = "//ORIGIN-STORE-GAME-TILE[.//SPAN[text()='%s']]";
    protected static final By STORE_VIEW_ALL_LOCATOR = By.cssSelector("origin-search-results[results='storeResults'] .origin-search-view-more a");
    protected static final By STORE_VIEW_ALL_HEADER_LOCATOR = By.cssSelector("origin-search-results[results='storeResults'] .origin-search-view-more .otka");
    protected static final By STORE_VIEW_MORE_LOCATOR = By.cssSelector("origin-search-results .l-origin-storegametiles-more a");
    protected static final String STORE_GAMETILE_OVERLAY_CSS = ".origin-storegametile-overlaycontent .origin-storegametile-cta";
    protected static final String STORE_GAMETILE_LEARN_MORE_CSS = STORE_GAMETILE_OVERLAY_CSS + " .otkbtn";
    protected static final By STORE_GAMETILE_LEARN_MORE_LOCATOR = By.cssSelector(STORE_GAMETILE_LEARN_MORE_CSS);

    // People Results
    protected static final String PEOPLE_RESULTS_CSS = "origin-search-results[results='peopleResults']";
    protected static final By PEOPLE_RESULTS_LOCATOR = By.cssSelector(PEOPLE_RESULTS_CSS);
    protected static final String USER_TILE_CSS_TMPL = "//*[contains(text(), '%s')]";
    protected static final By PEOPLE_VIEW_ALL_LOCATOR = By.cssSelector(PEOPLE_RESULTS_CSS + " .origin-search-view-more a");

    protected static final By PEOPLETILE_TITLE_LOCATOR = By.cssSelector(".origin-friendtile-originId > strong");
    protected static final By USER_TILE_VIEW_FRIEND_LINK_LOCATOR = By.cssSelector(".origin-friendtile-originId"); // link near the bottom of a tile with the username/email of the friend
    protected static final By USER_TILE_VIEW_PROFILE_BUTTON_LOCATOR = By.xpath("//*[contains(text(), 'View Profile')]");
    protected static final String NO_RESULTS_MESSAGE = "No results found";
    protected static final By LOADING_SPINNER_LOCATOR = By.cssSelector(".origin-search-loading .origin-search-loading-spinner");

    //View More Results
    protected static final By BROWSE_PAGE_TITLE_LOCATOR = By.cssSelector(".origin-tilelist-pagetitle .origin-store-browse-pagetitle");

    public GlobalSearchResults(WebDriver driver) {
        super(driver);
    }

    /**
     * Clicks the 'View All' link for game library results.
     */
    public void clickViewAllGameLibraryResults() {
        waitForElementClickable(GAME_LIBRRARY_VIEW_ALL_LOCATOR).click();
    }

    /**
     * Checks if 'View All Store Results' is visible
     *
     * @return true if 'View All Store Results' is visible, false if not
     */
    public boolean verifyViewAllStoreResults() {
        return waitIsElementVisible(STORE_VIEW_ALL_LOCATOR);
    }

    /**
     * Clicks the 'View All' link for game library results.
     */
    public void clickViewAllStoreResults() {
        waitForElementClickable(STORE_VIEW_ALL_LOCATOR).click();
    }

    /**
     * Clicks the 'View All' link for people results.
     */
    public void clickViewAllPeopleResults() {
        waitForElementClickable(PEOPLE_VIEW_ALL_LOCATOR).click();
    }

    /**
     * Clicks the 'View More' link for game library results
     */
    public void clickViewMoreStoreResults() {
        waitForElementClickable(STORE_VIEW_MORE_LOCATOR).click();
    }

    /**
     * Verifies the 'View More' button for Store Results is displayed
     *
     * @return true if the button is visible, false otherwise
     */
    public boolean verifyViewMoreButtonStoreResultsVisible() {
        return waitIsElementVisible(STORE_VIEW_MORE_LOCATOR);
    }

    /**
     * Clicks the 'View Profile' button of the specified user after hovering over the friend's user tile.
     *
     * @param user The username whose profile we want to view
     */
    public void viewProfileOfUser(UserAccount user) {
        WebElement userTile = waitForElementVisible(By.xpath(String.format(USER_TILE_CSS_TMPL, user.getUsername())), 30);
        // By hovering over the user's profile tile, the "View Profile" button will appear
        hoverOnElement(userTile.findElement(USER_TILE_VIEW_PROFILE_BUTTON_LOCATOR));
        waitForElementVisible(USER_TILE_VIEW_PROFILE_BUTTON_LOCATOR).click();
    }

    /**
     * Returns the store tile title element with the given title or throws an exception if not found
     *
     * @param gameName given title
     * @return store tile title element
     * @throws Exception Exception
     */
    private WebElement getStoreTileTitleElement(String gameName) {
        for (WebElement tileTitle : waitForAllElementsVisible(STORETILE_TITLE_LOCATOR, 20)) {
            if (tileTitle.getText().trim().equals(gameName))
                return tileTitle;
        }
        return null;
    }

    /**
     * Views the GDP of a store game tile by clicking on its title.
     *
     * @param gameName The name of the store game tile
     */
    public void viewGDPOfGame(String gameName) {
        WebElement storeTileTitle = getStoreTileTitleElement(gameName);
        scrollElementToCentre(storeTileTitle);
        if (storeTileTitle != null) {
            storeTileTitle.click();
        }
    }

    /**
     * Verify that a specified user is found in the search results.
     *
     * @param user The user who is expected to be found
     * @return true if the expected user was found in the search results, false
     * otherwise
     */
    public boolean verifyUserFound(UserAccount user) {
        try {
            waitForElementVisible(By.xpath(String.format(USER_TILE_CSS_TMPL, user.getUsername())), 60);
        } catch (TimeoutException e) {
            return false;
        }
        return true;
    }

    /**
     * Verify the search results are displayed above the store and people
     * results.
     *
     * @return true if the search results are displayed first, false otherwise
     */
    public boolean verifySearchResultsAboveStoreAndPeople() {
        int searchResultsPosition = waitForElementVisible(GLOBAL_SEARCH_RESULTS_LOCATOR).getLocation().getY();
        int storeResultsPosition = waitForElementVisible(STORE_RESULTS_LOCATOR).getLocation().getY();
        if (verifyNoPeopleResultsDisplayed()) { // if there are no people results displayed
            return searchResultsPosition < storeResultsPosition;
        } else { // if people are displayed
            int peopleResultsPosition = waitForElementVisible(PEOPLE_RESULTS_LOCATOR).getLocation().getY();
            return ((searchResultsPosition < storeResultsPosition) && (searchResultsPosition < peopleResultsPosition));
        }
    }

    /**
     * When given a list of store games that could potentially be in the search
     * results, find the first one that is visible and return the name of the
     * store game.
     *
     * @param gameNames The names of the store game tiles that are expected to
     *                  be found
     * @return The name of the store game that is found or an empty string if no
     * store game tiles were found that match what is being looked for
     */
    public String getVisibleStoreGame(String... gameNames) {
        waitForResults();
        final int length = gameNames.length;
        By[] locators = new By[length];
        for (int i = 0; i < length; ++i) {
            locators[i] = By.xpath(String.format(STORETILE_BY_NAME_XPATH_TMPL, gameNames[i]));
        }

        try {
            WebElement element = waitForAnyElementVisible(pageWait(60), locators);
            return element.findElement(STORETILE_TITLE_LOCATOR).getText();
        } catch (TimeoutException e) {
            return "";
        }
    }

    /**
     * Hovers on the game in order to verify if the 'Details' button is visible
     *
     * @return true if the button 'Details' is displayed, false otherwise
     */
    public boolean hoverOnGameAndClickDetails() {
        List<WebElement> gameTitles = driver.findElements(GAMES_LIST_LOCATOR);
        hoverElement(gameTitles.get(0));
        List<WebElement> detailsButton = driver.findElements(GAME_LIBRARY_DETAILS_BUTTON);
        return waitIsElementVisible(detailsButton.get(1));
    }

    /**
     * Clicks on the 'Details' button which appears on each game when hovering
     */
    public void clickOnSecondTileDetailsButton() {
        List<WebElement> detailsButton = driver.findElements(GAME_LIBRARY_DETAILS_BUTTON);
        detailsButton.get(1).click();
    }

    /**
     * Verify the search results header has appeared.
     *
     * @return true if the header element is visible, false otherwise
     */
    public boolean verifySearchDisplayed() {
        return waitIsElementVisible(GLOBAL_SEARCH_RESULTS_LOCATOR);
    }

    /**
     * Verify the search results are displayed on one line
     *
     * @return true if the results are displayed on one line, false otherwise
     */
    public boolean verifyGameLibrarySearchResultDisplayedOneLine() {
        List<WebElement> gameTitles = driver.findElements(GAMES_LIST_LOCATOR);
        boolean displayedOneLine = false; //we suppose they are not displayed on one
        for (int i = 0; i < gameTitles.size() - 1; i++) {
            int positionOfGame1 = gameTitles.get(i).getLocation().getY(); // the first Y coordinate of the game results
            int positionOfGame2 = gameTitles.get(i + 1).getLocation().getY(); // the second Y coordinate of the game results
            if (positionOfGame1 == positionOfGame2) { // if they are identical, they are shown on one line
                displayedOneLine = true;
            }
        }
        return displayedOneLine;
    }

    /**
     * Checks the 'View All' link for game library results exists.
     *
     * @return true if the link is visible, false otherwise
     */
    public boolean verifyViewAllGameLibraryLink() {
        return waitIsElementVisible(GAME_LIBRRARY_VIEW_ALL_LOCATOR);
    }

    /**
     * Verify the game library search results header has appeared for a given
     * search term.
     *
     * @param searchTerm The text we want to search for.
     * @return true if the header element is visible, false otherwise
     */
    public boolean verifyGameLibraryResults(String searchTerm) {
        waitForResults();
        List<WebElement> gameTitles = driver.findElements(GAMETILE_TITLE_LOCATOR);
        _log.debug("found " + gameTitles.size() + " item(s)");
        boolean resultsMatch = true;

        for (WebElement gameTile : gameTitles) {
            String title = gameTile.getAttribute("alt");
            _log.debug("game tile text: " + title);
            if (gameTile.isDisplayed()) {
                _log.debug("'" + title + "' is displayed");
                if (!StringUtils.containsIgnoreCase(title, searchTerm)) {
                    _log.debug("'" + title + "' doesn't contain " + searchTerm);
                    resultsMatch = false;
                }
            }
        }

        return resultsMatch;
    }

    /**
     * Verify the game library search results are in alphabetical order.
     *
     * @return true if the game tiles are in alphabetical order, false otherwise
     */
    public boolean verifyGameLibraryResultsOrder() {
        waitForResults();
        List<WebElement> gameTiles = waitForAllElementsVisible(GAMETILE_TITLE_LOCATOR);
        List<String> gameTitles = gameTiles.stream()
                .map(tile -> tile.getAttribute("alt"))
                .collect(Collectors.toList());

        return TestScriptHelper.verifyListSorted(gameTitles, false);
    }

    /**
     * Checks if a specific title can be found in the game library results.
     *
     * @param gameTitle The title of the game we want to find
     * @return true if the game is found, false otherwise
     */
    public boolean verifyGameLibraryContainsOffer(String gameTitle) {
        waitForResults();
        return waitIsElementVisible(By.cssSelector(String.format(GAMETILE_TITLE_TMPL, gameTitle)));
    }

    /**
     * Verify the store search results header has appeared.
     *
     * @param searchTerm The text we want to search for
     * @return true if header element is visible, false otherwise
     */
    public boolean verifyStoreResults(String searchTerm) {
        waitForResults();
        List<WebElement> storeTitles = driver.findElements(STORETILE_TITLE_LOCATOR);
        _log.debug("found " + storeTitles.size() + " item(s)");
        boolean resultsMatch = false;

        // some tiles don't get updated in time, yeild some cycle and hope it gets updated on retry
        final int retry = 2;
        int matches = storeTitles.size();
        for (int cur = 0; cur < retry; ++cur) {
            resultsMatch = true;
            for (WebElement storeTile : storeTitles) {
                String title = storeTile.getText();
                _log.debug("store tile text: " + title);
                if (storeTile.isDisplayed()) {
                    _log.debug("'" + title + "' is displayed");
                    if (!StringUtils.containsIgnoreCase(title, searchTerm)) {
                        _log.debug("'" + title + "' doesn't contain " + searchTerm);
                        if (matches == 0) {
                            resultsMatch = false;
                        } else {
                            matches--;
                        }
                    }
                }
            }

            if (resultsMatch) {
                break;
            } else if (cur < retry - 1) {
                _log.debug("sleep and retry");
                sleep(3000L);
            }
        }

        return resultsMatch;
    }

    /**
     * Verify the friends search results header has appeared.
     *
     * @param searchTerm The text we want to search for.
     * @return true if the header element is visible, false otherwise
     */
    public boolean verifyPeopleResults(String searchTerm) {
        List<WebElement> friendTitles = getPeopleResultTiles();
        _log.debug("found " + friendTitles.size() + " item(s)");
        boolean resultsMatch = false;

        // some tiles don't get updated in time, yeild some cycle and hope it gets updated on retry
        final int retry = 2;
        for (int cur = 0; cur < retry; ++cur) {
            resultsMatch = true;
            for (WebElement friendTile : friendTitles) {
                String title = friendTile.getText();
                _log.debug("friend tile text: " + title);
                if (friendTile.isDisplayed()) {
                    _log.debug("'" + title + "' is displayed");
                    if (!StringUtils.containsIgnoreCase(title, searchTerm)) {
                        _log.debug("'" + title + "' doesn't contain " + searchTerm);
                        resultsMatch = false;
                    }
                }
            }

            if (resultsMatch) {
                break;
            } else if (cur < retry - 1) {
                _log.debug("sleep and retry");
                sleep(3000L);
            }
        }

        return resultsMatch;
    }

    /**
     * Verify the 'No Results Found' message is displayed.
     *
     * @return true if the 'No results found' is displayed, false otherwise
     */
    public boolean verifyNoResultsDisplayed() {
        return waitForElementVisible(NO_RESULTS_LOCATOR, 15).getText().contains(NO_RESULTS_MESSAGE);
    }

    /**
     * Verify 'Game Library' results are not displayed.
     *
     * @return true if there are no 'Game Library' search results, false if
     * 'Game Library' search results appear
     */
    public boolean verifyGameLibraryResultsNotDisplayed() {
        return Waits.pollingWait(() -> driver.findElements(GAME_LIBRARY_RESULTS_LOCATOR).isEmpty());
    }

    /**
     * Verify 'People' results are not displayed.
     *
     * @return true if there are no 'People' search results, false if 'People'
     * search results appear
     */
    public boolean verifyPeopleResultsNotDisplayed() {
        return Waits.pollingWait(() -> driver.findElements(PEOPLE_RESULTS_LOCATOR).isEmpty());
    }

    /**
     * Verify the 'No people show up in the search' message is displayed.
     *
     * @return true if there are no people results, false otherwise
     */
    public boolean verifyNoPeopleResultsDisplayed() {
        return !isElementPresent(PEOPLE_RESULTS_LOCATOR);
    }

    /**
     * Get a list of the titles of the result in the store section.
     *
     * @return A list of the titles that are shown in the 'Store Results'
     * section
     */
    private List<String> getStoreResultTitles() {
        waitForResults();
        return driver.findElements(STORETILE_TITLE_LOCATOR).stream()
                .map(WebElement::getText)
                .collect(Collectors.toList());
    }

    /**
     * Verify the 'Popular on Origin' title is displayed
     *
     * @return title of the 'Popular on Origin' is displayed, false otherwise
     */
    public boolean verifyFeaturedGamesTitleDisplayed() {
        String actualCarouselTitle = waitForElementVisible(FEATURED_GAMES_CAROUSEL_TITLE_LOCATOR).getText();
        return StringHelper.containsIgnoreCase(actualCarouselTitle, FEATURED_GAMES_CAROUSEL_TITLE);
    }

    /**
     * Verify 'Featured Games' carousel is present when no results are displayed
     *
     * @return true if 'Featured Games Carousel' is present, false otherwise
     */
    public int getNumberOfDisplayedFeaturedGamesResults() {
        return getFeaturedGamesTiles().size();
    }


    /**
     * Get a list of tiles from 'Featured Games' carousel
     *
     * @return a list of the tiles displayed in 'Featured Games' carousel
     */
    private List<WebElement> getFeaturedGamesTiles() {
        waitForResults();
        return driver.findElements(FEATURED_GAMES_CAROUSEL_TILES_LOCATOR);
    }

    /**
     * Verify images exist on all the tiles in 'Featured Games' carousel
     *
     * @return if all tiles have an image, false otherwise
     */
    public boolean verifyFeaturedGamesImagesDisplayed() {
        List<WebElement> images = driver.findElements(FEATURED_GAMES_CAROUSEL_TILES_IMAGE_LOCATOR);
        for (WebElement image : images) {
            if (!image.getAttribute("src").contains("https")) {
                return false;
            }
        }
        return true;
    }

    /**
     * Get a list of the usernames in the people results section.
     *
     * @return A list of usernames that are shown in the 'People Results'
     * section
     */
    private List<WebElement> getPeopleResultTiles() {
        waitForResults();
        return driver.findElements(PEOPLETILE_TITLE_LOCATOR);
    }

    /**
     * Checks that at least one of the search results' titles contains any of
     * the provided words (case ignored).
     *
     * @param words The words to check for
     * @return true if any of the titles contains one of the words, false
     * otherwise
     */
    public boolean verifyAnyTitleContainAnyOf(List<String> words) {
        return getStoreResultTitles().stream()
                .anyMatch(title -> words.stream()
                        .anyMatch(word -> StringHelper.containsIgnoreCase(title, word)));
    }

    /**
     * Waits for the search results page to load.
     */
    public void waitForResults() {
        waitForPageToLoad();
        waitForAngularHttpComplete(30);

        // Wait for one of "Store results", "People results", "Game library results", or "No results" to appear
        waitForAnyElementVisible(GAME_LIBRARY_RESULTS_LOCATOR, STORE_RESULTS_LOCATOR, PEOPLE_RESULTS_LOCATOR, NO_RESULTS_LOCATOR, BROWSE_PAGE_TITLE_LOCATOR);
    }

    /**
     * Waits for the game tiles to load
     */
    public void waitForGamesToLoad() {
        // wait up to 5 seconds for games to start loading on the search result page
        waitForElementVisible(STORETILE_TITLE_LOCATOR, 10);
    }

    /**
     * Checks if the loading spinner graphic appears before loading more
     * results. The spinner itself is loaded separately and doesn't actually
     * appear in the HTML, therefore we check the div tag around it.
     *
     * @return true if the spinner is there, false otherwise
     */
    public boolean verifyLoadingSpinnerPresent() {
        return waitIsElementPresent(LOADING_SPINNER_LOCATOR);
    }

    /**
     * Wait for the additional results to load.
     */
    public void waitForMoreResultsToLoad() {
        waitForElementInvisibleByLocator(LOADING_SPINNER_LOCATOR, 10);
    }

    /**
     * Gets the number of Store search results based on the header
     *
     * @return the number of Store search results
     */
    public int getNumberOfStoreResults() {
        WebElement storeResults = waitForAnyElementVisible(STORE_VIEW_ALL_HEADER_LOCATOR);
        String stringValue = storeResults.getText();
        Double numberOfStoreResults = StringHelper.extractNumberFromText(stringValue);
        return numberOfStoreResults.intValue();
    }

    /**
     * Gets the number of Store search results based on the list of tiles
     * displayed
     *
     * @return the number of tiles displayed
     */
    public int getNumberOfStoreResultsTiles() {
        return getStoreResultTitles().size();
    }

    /**
     * Gets the number of people tiles currently displayed in the people results
     * section.
     *
     * @return The amount of people tiles currently displayed
     */
    public int getNumberOfDisplayedPeopleResults() {
        waitForResults();
        return getPeopleResultTiles().size();
    }

    /**
     * Verify if a game tile with the specific name is visible in the store search results
     *
     * @param gameName The name of the game to search
     * @return true if the game tile is visible, false otherwise
     */
    public boolean storeResultTileVisible(String gameName) {
        return getStoreResultTitles().stream().anyMatch(titleName -> titleName.equals(gameName));
    }

    /**
     * Check if a game tile with specific title is shown in the store results.
     *
     * @param gameTitle The title of the game tile we want to find
     * @return true if the game is found, false otherwise
     */
    public boolean verifyStoreResultContainsOffer(String gameTitle) {
        waitForResults();
        return getStoreTileTitleElement(gameTitle) != null;
    }
}