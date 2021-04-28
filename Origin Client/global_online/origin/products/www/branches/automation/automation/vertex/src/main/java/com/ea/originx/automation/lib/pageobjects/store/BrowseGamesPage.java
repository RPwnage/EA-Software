package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.vx.originclient.utils.Waits;
import java.lang.invoke.MethodHandles;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;
import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * A page object that represents the 'Browse Games' page.
 *
 * @author tharan
 * @author palui
 */
public class BrowseGamesPage extends EAXVxSiteTemplate {

    protected static final By BROWSE_GAMES_PAGE_LOCATOR = By.cssSelector("div[class*='store-browse-wrapper']");
    protected static final By BROWSE_GAMES_PAGE_TITLE_LOCATOR = By.cssSelector(".origin-store-browse-pagetitle");

    protected static final By BROWSE_GAME_PAGE_TILE_LOCATOR = By.cssSelector("otkex-hometile");
    protected static final By BROWSE_GAME_PAGE_TILE_TITLE_LOCATOR = By.cssSelector("otkex-hometile .home-tile-header");

    protected static final By LOADING_SPINNER_LOCATOR = By.cssSelector(".origin-loading-spinner");
    protected static final String[] MESSAGE_KEYWORDS = {"no", "results", "found", "search"};

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public BrowseGamesPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Wait for the 'Browse Games' page to load a sufficient amount of store game tiles
     */
    public void waitForGamesToLoad() {
        //Wait for a single tile to load
        Waits.pollingWait(() -> waitForElementPresent(BROWSE_GAME_PAGE_TILE_LOCATOR).getSize().getHeight() != 0);
        Waits.pollingWait(() -> !waitForElementVisible(BROWSE_GAME_PAGE_TILE_TITLE_LOCATOR).getText().isEmpty());

        //Wait for additional tiles to load
        //Waits.sleep(1000);
        //int numberOfInitialTilesLoaded = driver.findElements(By.cssSelector(".origin-storegametile")).size();
        Waits.sleep(4000);
        Waits.sleep(2000);
        int numberOfInitialTilesLoaded = driver.findElements(BROWSE_GAME_PAGE_TILE_LOCATOR).size();
        boolean isStoreGameTileCTALoaded;
        boolean isStoreGameTileTextsLoaded;
        for (int i = 0; i < 3; i++) {
            isStoreGameTileCTALoaded = false;
            isStoreGameTileTextsLoaded = false;
            List<WebElement> storeGameTileCTAs = driver.findElements(BROWSE_GAME_PAGE_TILE_LOCATOR); // these elements are present but not visible
            List<WebElement> storeGameTileTexts = driver.findElements(BROWSE_GAME_PAGE_TILE_TITLE_LOCATOR);
            if (storeGameTileCTAs.size() >= numberOfInitialTilesLoaded && storeGameTileTexts.size() >= numberOfInitialTilesLoaded) {
                // Checks that the last tile initially loaded has a CTA present and a non-empty title
                WebElement lastTileLoaded = storeGameTileCTAs.get(numberOfInitialTilesLoaded - 1);
                isStoreGameTileCTALoaded = lastTileLoaded.getSize().getHeight() != 0 && !lastTileLoaded.getText().equals(null);
                // Checks that the last tile initially loaded has the name of the game visible
                isStoreGameTileTextsLoaded = !storeGameTileTexts.get(numberOfInitialTilesLoaded - 1).getText().isEmpty();
                if (isStoreGameTileCTALoaded && isStoreGameTileTextsLoaded) {
                    return;
                }
            }
            Waits.sleep(4000); //Wait for the tiles to load CTA and Title attributes
        }
    }

    /**
     * Verify if the 'Browse Games' page is displayed.
     *
     * @return true if page displayed, false otherwise
     */
    public boolean verifyBrowseGamesPageReached() {
        return waitIsElementVisible(BROWSE_GAMES_PAGE_LOCATOR);
    }

    /**
     * Verify page title contains the given text.
     *
     * @param searchString The expected text
     * @return true if given text is in the page title, false otherwise
     */
    public boolean verifyTextInPageTitle(String searchString) {
        WebElement pageTitle = waitForElementVisible(BROWSE_GAMES_PAGE_TITLE_LOCATOR);
        return StringUtils.containsIgnoreCase(pageTitle.getText(), searchString);
    }

    /**
     * Get all the store game tiles by scrolling until all the
     * elements are loaded.
     *
     * @return A list of store game tile WebElements
     */
    private List<StoreHomeTile> getAllStoreGameTiles() {
        List<StoreHomeTile> previousElements = new ArrayList<>();
        List<StoreHomeTile> currentElements = getAllStoreGameTiles();

        while (previousElements.size() != currentElements.size()) {
            scrollToBottom();
            sleep(10000);
            previousElements = currentElements;
            currentElements = getAllStoreGameTiles();
        }
        
        return currentElements;
    }

    /**
     * Get parent name from a store game tile WebElement.
     *
     * @return String parent name of the store game title
     * @throws RuntimeException if the parent name is an empty String
     */
    private String getParentName(WebElement storeGameTileElement) {
        String parentName = storeGameTileElement.getAttribute("tile-header");
        if (!parentName.isEmpty()) {
            _log.debug("Get store game tile parentName: " + parentName);
        } else {
            String errorMessage = "Cannot determine parentName for store game tile WebElement: " + storeGameTileElement.toString();
            _log.error(errorMessage);
            throw new RuntimeException(errorMessage);
        }
        return parentName;
    }

//    /**
//     * Get entitlement title from a store game tile WebElement.
//     *
//     * @return String title of the entitlement
//     * @throws RuntimeException if the title is an empty String
//     */
//    private String getEntitlementTitle(WebElement storeGameTileElement) {
//        String title = storeGameTileElement.findElement(GAME_TILE_OFFER_ID_LOCATOR).getText();
//        if (!title.isEmpty()) {
//            _log.debug("Get store game tile offerId: " + title);
//        } else {
//            String errorMessage = "Cannot determine offerid for store game tile WebElement: " + storeGameTileElement.toString();
//            _log.error(errorMessage);
//            throw new RuntimeException(errorMessage);
//        }
//        return title;
//    }

    /**
     * Gets a list of loaded Game Tiles
     *
     * @return a list of game tiles as WebElements
     */
    public List<WebElement> getLoadedStoreTiles() {
        waitForGamesToLoad();
        List<WebElement> storeGameTileWebElements = waitForAllElementsVisible(BROWSE_GAME_PAGE_TILE_LOCATOR);

        return storeGameTileWebElements;
    }

    /**
     * Verifies if an entitlement is displayed among the loaded games in
     * 'Browse' page
     *
     * @param gameTitle game to be checked in 'Browse' page
     * @return true if the game is found in page, false otherwise
     */
    public boolean verifyGameInPage(String gameTitle) {
        for (WebElement title : getLoadedStoreTiles()) {
            if (StringHelper.containsIgnoreCase(title.findElement(BROWSE_GAME_PAGE_TILE_TITLE_LOCATOR).getText(), gameTitle)) {
                return true;
            }
        }

        return false;
    }

    /**
     * Get all (loaded) store game tiles as a list of StoreGameTile objects.
     *
     * @return List of (loaded) StoreGameTile objects
     */
    public List<StoreHomeTile> getLoadedStoreGameTiles() {
        waitForGamesToLoad();
        List<WebElement> storeGameTileWebElements = driver.findElements(BROWSE_GAME_PAGE_TILE_LOCATOR);
        List<StoreHomeTile> storeGameTiles = new ArrayList<StoreHomeTile>();
        String tileTitle = null;
        for (WebElement storeGameTileWebElement: storeGameTileWebElements) {
            // Add store game tiles to the list that have loaded their offer-id's until reaching a tile that
            // hasn't finished loading
            try {
                tileTitle = getParentName(storeGameTileWebElement);
            } catch (RuntimeException e) {
                break;
            }
            storeGameTiles.add(new StoreHomeTile(driver, tileTitle));
        }
        return storeGameTiles;
    }

    /**
     * Get all (loaded) store game tile titles.
     *
     * @return List of store game tile titles
     */
    public List<String> getAllStoreGameTitles() {
        return getLoadedStoreGameTiles().stream()
                .map(tile -> tile.getTileTitle())
                .collect(Collectors.toList());
    }

    /**
     * Verify all (loaded) store game tile titles contain keyword String.
     *
     * @param keyword String to check for containment
     * @return true if all titles contain the keyword, false otherwise
     */
    public boolean verifyAllStoreGameTitlesContain(String keyword) {
        List<StoreHomeTile> storeHomeTiles = getLoadedStoreGameTiles();
        for (StoreHomeTile storeHomeTile : storeHomeTiles) {
            if (!storeHomeTile.getTileTitle().contains(keyword)) {
                return false;
            }
        }
        return true;
    }

    /**
     * Gets the number of game tiles currently displayed.
     *
     * @return The number of game tiles displayed
     */
    public int getNumberOfGameTilesLoaded() {
        return waitForAllElementsVisible(BROWSE_GAME_PAGE_TILE_LOCATOR).size();
    }

    /**
     * Verifies the game tiles are sorted lexically by title.
     *
     * @return true if they are sorted lexically by title, false otherwise
     */
    public boolean verifyTilesSortedByName() {
        // Origin ignores TM, Registered Marks, ':'s, and case when sorting
        List<String> titlesNoTm = getAllStoreGameTitles().stream()
                .map(s -> s.replaceAll("[:\u2122\u00AE]", ""))
                .map(String::toLowerCase)
                .collect(Collectors.toList());
        return TestScriptHelper.verifyListSorted(titlesNoTm, false);
    }

    /**
     * Verifies the game tiles are sorted reverse lexically by title.
     *
     * @return true if they are sorted reverse lexically by title, false otherwise
     */
    public boolean verifyTilesSortedByNameRev() {
        // Origin ignores TM, Registered Marks, ':'s, and case when sorting
        List<String> titlesNoTm = getAllStoreGameTitles().stream()
                .map(s -> s.replaceAll("[:\u2122\u00AE]", ""))
                .map(String::toLowerCase)
                .collect(Collectors.toList());
        return TestScriptHelper.verifyListSorted(titlesNoTm, true);
    }

    /**
     * Checks if the loading spinner graphic appears when loading more results
     * after reaching the bottom on the ones currently displayed.
     *
     * @return true if the spinner is there, false otherwise
     */
    public boolean verifyLoadingSpinnerPresent() {
        return waitIsElementPresent(LOADING_SPINNER_LOCATOR);
    }

    /**
     * Wait for additional results to load.
     */
    public void waitForMoreResultsToLoad() {
        int initialNumResultsLoaded = getLoadedStoreGameTiles().size();
        int numRetries = 3;
        int currentNumResultsLoaded;
        for (int i = 0; i < numRetries; i++) {
            Waits.sleep(1000); // Wait for additional tiles to load
            currentNumResultsLoaded = getLoadedStoreGameTiles().size();
            if (initialNumResultsLoaded < currentNumResultsLoaded) {
                return;
            }
        }
        _log.debug("No additional results loaded");
    }
    
    /**
     * Verify the search did not return any results
     * 
     * @return true if there are no results displayed, false otherwise
     */
    public boolean verifyNoResultsFound() {
        String content = waitForElementPresent(BROWSE_GAMES_PAGE_LOCATOR).getText();
        return StringHelper.containsIgnoreCase(content, MESSAGE_KEYWORDS);
    }
}