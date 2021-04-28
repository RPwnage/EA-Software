package com.ea.originx.automation.lib.pageobjects.gamelibrary;

import com.ea.originx.automation.lib.helpers.I18NUtil;
import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.games.SpecialEntitlementInfo;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * A page object that represents the 'Game Library'.
 *
 * @author tharan
 * @author palui
 */
public class GameLibrary extends EAXVxSiteTemplate {

    protected static final By GAME_LIBRARY_PAGE_LOCATOR = By.cssSelector("origin-gamelibrary-loggedin");
    protected static final By GAME_LIBRARY_TOP_BANNER_LOCATOR = By.cssSelector(".l-origin-gamelibrary-topbanner");

    protected static final By GO_ONLINE_PURCHASE_MESSAGE_LOCATOR = By.cssSelector(".origin-messages-center .otktitle-3");
    protected static final By NAVIGATION_ITEMS_LOCATOR = By.cssSelector("origin-gamelibrary-navigationitem");
    protected static final By LIBRARY_EMPTY_TITLE_LOCATOR = By.cssSelector(".l-no-games-online .otktitle-page");
    protected static final String[] LIBRARY_EMPTY_TITLE_KEYWORDS = {I18NUtil.getMessage("library"), I18NUtil.getMessage("empty")};
    protected static final By ADD_A_GAME_BUTTON_LOCATOR = By.cssSelector("origin-gamelibrary-nav div.otkdropdown");
    protected static final By ADD_A_GAME_DROPDOWN_OPTIONS = By.cssSelector("origin-gamelibrary-nav div.otkdropdown ul li");
    protected static final By REDEEM_PRODUCT_DIALOG_LOCATOR = By.cssSelector("#redeem-iframe");
    protected static final By REDEEM_MODAL_NEXT_BUTTON = By.cssSelector(".otkmodal-footer .otkbtn.otkbtn-primary");

    //Resize Locators
    protected static final String GAME_LIST_CSS_TMPL = "origin-gamelibrary-gameslist  section  div.origin-gameslist-wrapper";
    protected static final By SMALL_TILE_LOCATOR = By.cssSelector("li.origin-gamelibrary-tilesize-item:nth-of-type(1)");
    protected static final By MEDIUM_TILE_LOCATOR = By.cssSelector("li.origin-gamelibrary-tilesize-item:nth-of-type(2)");
    protected static final By LARGE_TILE_LOCATOR = By.cssSelector("li.origin-gamelibrary-tilesize-item:nth-of-type(3)");
    protected static final By GAME_TILE_SMALL_SIZE_LOCATOR = By.cssSelector(GAME_LIST_CSS_TMPL + " > .l-origin-gameslist-list.origin-gameslist-zoom-small");
    protected static final By GAME_TILE_MEDIUM_SIZE_LOCATOR = By.cssSelector(GAME_LIST_CSS_TMPL + " > .l-origin-gameslist-list.origin-gameslist-zoom-medium");
    protected static final By GAME_TILE_LARGE_SIZE_LOCATOR = By.cssSelector(GAME_LIST_CSS_TMPL + " > .l-origin-gameslist-list");

    // Game tiles - can be split into "Favorites" and "Other Games" lists
    protected static final String GAME_TILES_CSS = "origin-game-tile";
    protected static final String FAVORITES_LIST_CSS = "#gamelibrary-favoritegames";
    protected static final String FAVORITES_GAME_TILES_CSS = FAVORITES_LIST_CSS + " " + GAME_TILES_CSS;
    protected static final String OTHER_GAMES_LIST_CSS = "#gamelibrary-othergames";
    protected static final String OTHER_GAMES_GAME_TILES_CSS = OTHER_GAMES_LIST_CSS + " " + GAME_TILES_CSS;

    protected static final String GAMETILE_IMG_CSS_TMPL = ".origin-gametile > a > img[alt='%s']";
    protected static final String GAMETILE_OFFERID_CSS_TMPL = "origin-game-tile[offerid='%s'] > .origin-gametile";
    protected static final String GAMETILE_IMAGE_BY_NAME_XPATH_TMPL = "//*[contains(@class,'origin-gametile')]/a/img[contains(@alt, '%s')]";
    protected static final String GAMETILE_GAME_BADGE_BY_NAME_XPATH_TMPL = GAMETILE_IMAGE_BY_NAME_XPATH_TMPL + "/../origin-game-badge";

    protected static final By GAME_TILES_LOCATOR = By.cssSelector(GAME_TILES_CSS);
    protected static final By FAVORITES_LIST_LOCATOR = By.cssSelector(FAVORITES_LIST_CSS);
    protected static final By FAVORITES_GAME_TILES_LOCATOR = By.cssSelector(FAVORITES_GAME_TILES_CSS);
    protected static final By OTHER_GAMES_LIST_LOCATOR = By.cssSelector(OTHER_GAMES_LIST_CSS);
    protected static final By OTHER_GAME_TILES_LOCATOR = By.cssSelector(OTHER_GAMES_GAME_TILES_CSS);

    // Automation experiment
    protected static final String AUTOMATION_EXPERIMENT_CSS = "eax-experiment-bucket div[ng-if=inExperiment]";
    protected static final By AUTOMATION_EXPERIMENT_TEXT_LOCATOR = By.cssSelector(AUTOMATION_EXPERIMENT_CSS + " p");
    protected static final By AUTOMATION_EXPERIMENT_IMAGE_LOCATOR = By.cssSelector(AUTOMATION_EXPERIMENT_CSS + " img.origin-store-backgroundimagecarousel-image");
    protected static final String[] EXPECTED_AUTOMATION_EXPERIMENT_KEYWORDS_CONTROL = {"Automation", "Experiment", "variant", "control"};
    protected static final String[] EXPECTED_AUTOMATION_EXPERIMENT_KEYWORDS_VARIANT_A = {"Automation", "Experiment", "variant", "variantA"};
    protected static final String[] EXPECTED_AUTOMATION_EXPERIMENT_KEYWORDS_VARIANT_B = {"Automation", "Experiment", "variant", "variantB"};
    protected static final String[] ORIGIN_VAULT_GAME_TEXT = {"Origin", "Access", "Vault", "Game"};
    protected static final String[] REDEEM_PRODUCT_CODE_TEXT = {"Redeem", "Product", "Code"};
    protected static final String[] NON_ORIGIN_GAME_TEXT = {"Non-Origin", "Game"};

    protected static final By NO_RESULTS_FOUND_TEXT_LOCATOR = By.cssSelector(".origin-messages-center.origin-gamelibrary-content-nogames");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public GameLibrary(WebDriver driver) {
        super(driver);
    }

    /**
     * Clears the download queue by clicking on the top banner of the game
     * library.
     */
    public void clearDownloadQueue() {
        waitForElementClickable(GAME_LIBRARY_TOP_BANNER_LOCATOR).click();
    }

    /**
     * Clicks the 'Add A Game' button
     */
    public void clickAddGame() {
        WebElement addAGameButton = waitForElementClickable(ADD_A_GAME_BUTTON_LOCATOR);
        // Scroll in case the game tile is located near the extremities which can cause problems clicking
        scrollElementToCentre(addAGameButton);
        addAGameButton.click();
    }

    /**
     * Verifies the 'Add A Game' button is clicked by checking if the dropdown
     * menu is visible
     *
     * @return true if the button is clicked, false otherwise
     */
    public boolean verifyAddGameButtonClicked() {
        return waitIsElementVisible(ADD_A_GAME_DROPDOWN_OPTIONS);
    }

    /**
     * Clicks on the "ADD A GAME" dropdown, and clicks "Redeem Product Code..."
     * from that list.
     */
    public void redeemProductCode() {
        waitForElementClickable(ADD_A_GAME_BUTTON_LOCATOR).click();
        waitForElementClickable(ADD_A_GAME_DROPDOWN_OPTIONS).click();
    }

    /**
     * Verify there is an option to 'Add a Vault Game'
     *
     * @return true if the option is visible, false otherwise
     */
    public boolean verifyAddVaultGameOptionVisible() {
        return driver.findElements(ADD_A_GAME_DROPDOWN_OPTIONS).stream().anyMatch(option
                -> StringHelper.containsAnyIgnoreCase(option.getText(), ORIGIN_VAULT_GAME_TEXT));
    }

    /**
     * Verify there is an option to 'Redeem a Product Code'
     *
     * @return true if the option is visible, false otherwise
     */
    public boolean verifyRedeemProductCodeOptionVisible() {
        return driver.findElements(ADD_A_GAME_DROPDOWN_OPTIONS).stream().anyMatch(option
                -> StringHelper.containsAnyIgnoreCase(option.getText(), REDEEM_PRODUCT_CODE_TEXT));
    }

    /**
     * Verify there is an option to 'Add a Non-Origin Game'
     *
     * @return true if the option is visible, false otherwise
     */
    public boolean verifyAddNonOriginGameOptionVisible() {
        return driver.findElements(ADD_A_GAME_DROPDOWN_OPTIONS).stream().anyMatch(option
                -> StringHelper.containsAnyIgnoreCase(option.getText(), NON_ORIGIN_GAME_TEXT));
    }

    /**
     * Clicks the 'Add A Vault Game' option
     */
    public void clickAddVaultGameOption() {
        driver.findElements(ADD_A_GAME_DROPDOWN_OPTIONS).stream().filter(option -> {
            return StringHelper.containsAnyIgnoreCase(option.toString(), ORIGIN_VAULT_GAME_TEXT);
        }).
                findFirst()
                .orElseThrow(() -> new RuntimeException()).click();
    }

    /**
     * Get list of game tiles from the game library
     *
     * @return List of game tiles as WebElements
     */
    public List<WebElement> getGameTileElements() {
        return waitForAllElementsVisible(GAME_TILES_LOCATOR);
    }

    /**
     * Get list of favorites game tiles (as WebElements) from the game library.
     * Assumption: the list is visible.
     *
     * @return List of favorites game tiles as WebElement's
     */
    public List<WebElement> getFavoritesGameTileElements() {
        return waitForAllElementsVisible(FAVORITES_GAME_TILES_LOCATOR);
    }

    /**
     * Get list of other (non-favorites) game tiles (as WebElements) from the
     * game library. Assumption: the list is visible.
     *
     * @return List of game tiles as WebElements
     */
    public List<WebElement> getOtherGameTileElements() {
        return waitForAllElementsVisible(OTHER_GAME_TILES_LOCATOR);
    }

    /**
     * Opens the filter menu by clicking the dropdown item.
     *
     * @return New FilterMenu object
     */
    public FilterMenu openFilterMenu() {
        List<WebElement> items = waitForAllElementsVisible(NAVIGATION_ITEMS_LOCATOR);
        getElementWithText(items, "Filter").click();
        return new FilterMenu(driver);
    }

    /**
     * Check if an entitlement is visible in the 'Game Library' given an offer
     * ID using a default wait time.
     *
     * @param offerID The entitlement offer ID
     * @return true if a visible entitlement is found matching the given Offer
     * ID
     */
    public boolean isGameTileVisibleByOfferID(String offerID) {
        return isGameTileVisibleByOfferID(offerID, 10);
    }

    /**
     * Check if an entitlement is visible in the 'Game Library' given an offer
     * ID.
     *
     * @param offerID The offer ID of the entitlement to check for
     * @param timeout The time in seconds to wait for the entitlement to be
     * visible
     * @return true if a visible entitlement is found matching the given offer
     * ID
     */
    public boolean isGameTileVisibleByOfferID(String offerID, int timeout) {
        return waitIsElementVisible(By.cssSelector(String.format(GAMETILE_OFFERID_CSS_TMPL, offerID)), timeout);
    }

    /**
     * Check if an entitlement is visible in the 'Game Library' given a name
     * using a default wait time.
     *
     * @param gameName The entitlement name to check for
     * @return true if a visible entitlement is found matching the given name,
     * false otherwise
     */
    public boolean isGameTileVisibleByName(String gameName) {
        return isGameTileVisibleByName(gameName, 10);
    }

    /**
     * Check if a game tile is visible in the 'Game Library' given a name
     *
     * @param gameName The entitlement name to check for
     * @param timeout The time, in seconds, to wait for the entitlement to be
     * visible
     * @return true if a visible entitlement is found matching the given name,
     * false otherwise
     */
    public boolean isGameTileVisibleByName(String gameName, int timeout) {
        return waitIsElementVisible(By.cssSelector(String.format(GAMETILE_IMG_CSS_TMPL, gameName)), timeout);
    }

    /**
     * Find offer ID of the game tile with matching (@code gameName) (by
     * matching to the 'alt' attribute of the game tile image).
     *
     * @param gameName Name of the game tile to match
     * @return Offer ID of the game tile if found, TimeoutException otherwise
     */
    public String getGameTileOfferIdByName(String gameName) {
        // Game badge is not visible - use waitForElementPresent
        WebElement gameBadgeElement = waitForElementPresent(By.xpath(String.format(GAMETILE_GAME_BADGE_BY_NAME_XPATH_TMPL, gameName)));
        return gameBadgeElement.getAttribute("offerid");
    }

    /**
     * Verify 'Game Library' page is empty.
     *
     * @return true if 'Game Library' has empty library message, false otherwise
     */
    public boolean verifyGameLibraryPageEmpty() {
        return waitIsElementVisible(LIBRARY_EMPTY_TITLE_LOCATOR);
    }

    /**
     * Verify 'Game Library' has 'Go Online' message when user is offline.
     *
     * @param userStatusForGameLibrary The user status for the 'Game Library'
     * @return true if game library has empty library message, false otherwise
     */
    public boolean verifyGameLibraryPageForGoOnlineOfflineMessage(String userStatusForGameLibrary) {
        return waitForElementVisible(GO_ONLINE_PURCHASE_MESSAGE_LOCATOR).getText().contains(userStatusForGameLibrary);
    }

    /**
     * Verify 'Game Library' page is visible.
     *
     * @return true if game library header element is visible, false otherwise
     */
    public boolean verifyGameLibraryPageReached() {
        return waitIsElementVisible(GAME_LIBRARY_TOP_BANNER_LOCATOR);
    }

    /**
     * wait for the 'Game Library' page to load.
     */
    public void waitForPage() {
        waitForPageThatMatches(".*game-library.*", 30);
        waitForPageToLoad();
    }

    /**
     * Check if the 'Favorites' list of game tiles is visible.
     *
     * @return true if list is visible, false otherwise
     */
    public boolean isFavoritesListVisible() {
        return waitIsElementVisible(FAVORITES_LIST_LOCATOR, 2);
    }

    /**
     * Check if the 'Other Games' (non-favorites) list of game tiles is visible.
     *
     * @return true if list is visible, false otherwise
     */
    public boolean isOtherGamesListVisible() {
        return waitIsElementVisible(OTHER_GAMES_LIST_LOCATOR, 2);
    }

    /**
     * Get the list of offer IDs from a list of game tile WebElements.
     *
     * @param gameTileElements List<WebElement> of game tiles
     * @return List<string> of offer IDs from the gameTileElements
     */
    private List<String> getOfferIds(List<WebElement> gameTileElements) {
        List<String> offerIds = new ArrayList<>();
        for (WebElement gameTileElement : gameTileElements) {
            offerIds.add(gameTileElement.getAttribute("offerid"));
        }
        return offerIds;
    }

    /**
     * Get the list of offer IDs from all game tile WebElements in the 'Game
     * Library'.
     *
     * @return List of offer IDs from the gameTileElements
     */
    public List<String> getGameTileOfferIds() {
        return getOfferIds(getGameTileElements());
    }

    /**
     * Get the list of offer IDs of favorites game tile WebElements in the 'Game
     * Library'. Assumption: the favorites list is visible.
     *
     * @return List of offer IDs from the gameTileElements
     */
    public List<String> getFavoritesGameTileOfferIds() {
        return getOfferIds(getFavoritesGameTileElements());
    }

    /**
     * Get the list of offer IDs of other (non-favorites) game tile WebElements
     * in the 'Game Library'. Assumption: the other games list is visible.
     *
     * @return List of offer IDs from the gameTileElements
     */
    public List<String> getOtherGameTileOfferIds() {
        return getOfferIds(getOtherGameTileElements());
    }

    /**
     * Check if a game with specific offer ID appears in the favorites game
     * tiles list.
     *
     * @param offerId The game's offer id
     * @return true if the favorites game tiles list is present and contains the
     * offer ID, false otherwise
     */
    public boolean isFavoritesGameTile(String offerId) {
        return isFavoritesListVisible() && getFavoritesGameTileOfferIds().contains(offerId);
    }

    /**
     * Check if a game with specific offer ID appears in the other
     * (non-favorite) game tiles list
     *
     * @param offerId The game's offer ID
     * @return true if the other game tiles list is present and contains the
     * offer ID, false otherwise
     */
    public boolean isOtherGameTile(String offerId) {
        return isOtherGamesListVisible() && getOtherGameTileOfferIds().contains(offerId);
    }

    /**
     * Clicks the small tile button to change the game tiles size to small.
     */
    public void setToSmallTiles() {
        waitForElementClickable(SMALL_TILE_LOCATOR).click();
    }

    /**
     * Clicks the medium tile button to change the game tiles size to medium.
     */
    public void setToMediumTiles() {
        waitForElementClickable(MEDIUM_TILE_LOCATOR).click();
    }

    /**
     * Clicks the large tile button to change the game tiles size to
     * large/default.
     */
    public void setToLargeTiles() {
        waitForElementClickable(LARGE_TILE_LOCATOR).click();
    }

    /**
     * Verify the size of game tiles are changed to small.
     * @return true if the element is visible, false otherwise
     */
    public boolean verifyTileSizeChangedToSmall() {
        return waitIsElementPresent(GAME_TILE_SMALL_SIZE_LOCATOR);
    }

    /**
     * Verify the size of game tiles are changed to medium.
     * @return true if the element is visible, false otherwise
     */
    public boolean verifyTileSizeChangedToMedium() {
        return waitIsElementPresent(GAME_TILE_MEDIUM_SIZE_LOCATOR);
    }

    /**
     * Verify the size of game tiles are changed to large/default.
     * @return true if the element is visible, false otherwise
     */
    public boolean verifyTileSizeChangedToLarge() {
        return waitIsElementPresent(GAME_TILE_LARGE_SIZE_LOCATOR);
    }

    /**
     * Verify the 'Game Library' has the expected games.
     *
     * @param expectedOfferids The offer IDs expected to be in the 'Game
     * Library'
     * @return true if the element is visible, false otherwise
     */
    public boolean verifyGameLibraryHasExpectedGames(String... expectedOfferids) {

        List<String> gameLibraryOfferIds = getGameTileOfferIds();
        List<String> expectedOfferIds = Arrays.asList(expectedOfferids);
        Collections.sort(gameLibraryOfferIds);
        Collections.sort(expectedOfferIds);
        return gameLibraryOfferIds.equals(expectedOfferIds);
    }

    /**
     * Verify the 'Game Library' lacks a specific game.
     *
     * @param designatedOfferId The offer ID not expected to be found in the
     * game library
     * @return true if the desginatedOfferId is not found in the 'Game Library',
     * false otherwise
     */
    public boolean verifyGameLibraryLacksDesignatedGame(String designatedOfferId) {

        List<String> gameLibraryOfferIds = getGameTileOfferIds();
        for (String offerId : gameLibraryOfferIds) {
            if (offerId.equals(designatedOfferId)) {
                return false;
            }
        }
        return true;
    }

    /**
     * Verify the 'Game Library' contains the expected games.
     *
     * @param expectedOfferids The list of offer IDs expected to be in the game
     * library
     * @return true if the expected offer IDs exist in the game library, false
     * otherwise
     */
    public boolean verifyGameLibraryContainsExpectedGames(List<String> expectedOfferids) {
        return expectedOfferids.stream().allMatch(offerId -> isGameTileVisibleByOfferID(offerId));
    }

    /**
     * Verify the 'Game Library' contains the expected games.
     *
     * @param expectedTitles The list of strings that represent the expected
     * games in 'Game Library'
     * @return true if the expected titles exist in the game library, false
     * otherwise
     */
    public boolean verifyGameLibraryContainsExpectedGamesByTitle(List<String> expectedTitles) {
        return expectedTitles.stream().allMatch(title -> isGameTileVisibleByName(title));
    }

    /**
     * The function for each give pdpOfferIds verify all the corresponding
     * gameLibraryOfferIds exists in the 'Game Library'.<br>
     *
     * While adding certain entitlements to the 'Game Library' - it either adds
     * a different entitlement(which has different offerId) or a set of
     * entitlements For each pdpOfferId there is corresponding set of
     * gameLibraryOfferIds.<br>
     *
     * For example: When you add Sims3 Starter Pack, we entitle Sims3 Standard
     * Edition in the game library Same as for Crysis Deluxe Edition - Crysis
     * Hunter Edition which has Deluxe Edition plus 2 DLCs.<br>
     *
     * @param pdpOfferIds The offer IDs expected to be in the 'Game Library'
     * @return true if for each give pdpOfferIds verify all the corresponding
     * gameLibraryOfferIds exists in the 'Game Library', otherwise false
     */
    public boolean verifyGameLibraryContainsSpecialGames(List<String> pdpOfferIds) {

        List<String> gameLibraryOfOfferIds;
        SpecialEntitlementInfo specialEntitlementInfo;

        for (String offerId : pdpOfferIds) {
            specialEntitlementInfo = new SpecialEntitlementInfo(offerId);
            gameLibraryOfOfferIds = specialEntitlementInfo.getGameLibraryOfferIds();
            if (!verifyGameLibraryContainsExpectedGames(gameLibraryOfOfferIds)) {
                return false;
            }
        }

        return true;
    }

    /**
     * Clears all the games from the 'Favorite List' if there are any favorite
     * games.
     */
    public void clearAllFavoritesIfExists() {
        if (isFavoritesListVisible()) {
            List<String> getAllFavoriteGames = getFavoritesGameTileOfferIds();
            for (String offerid : getAllFavoriteGames) {
                new GameTile(driver, offerid).removeFromFavorites();
            }
        }
    }

    /**
     * Verify there are special text and image element per experiment variant.
     *
     * @param variant Experiment variant
     */
    public boolean verifyAutomationExperiment(String variant) {
        switch (variant) {
            case "control":
                return StringHelper.containsIgnoreCase(waitForElementVisible(AUTOMATION_EXPERIMENT_TEXT_LOCATOR).getText(),
                        EXPECTED_AUTOMATION_EXPERIMENT_KEYWORDS_CONTROL)
                        && StringHelper.containsIgnoreCase(waitForElementVisible(AUTOMATION_EXPERIMENT_IMAGE_LOCATOR).getAttribute("src"),
                                "battlefield-1-early-enlister-deluxe-edition_hometile_2000x500_en_WW.jpg");

            case "variantA":
                return StringHelper.containsIgnoreCase(waitForElementVisible(AUTOMATION_EXPERIMENT_TEXT_LOCATOR).getText(),
                        EXPECTED_AUTOMATION_EXPERIMENT_KEYWORDS_VARIANT_A)
                        && StringHelper.containsIgnoreCase(waitForElementVisible(AUTOMATION_EXPERIMENT_IMAGE_LOCATOR).getAttribute("src"),
                                "mea_n7preorder_store_hero_en_ww.jpg");

            case "variantB":
                return StringHelper.containsIgnoreCase(waitForElementVisible(AUTOMATION_EXPERIMENT_TEXT_LOCATOR).getText(),
                        EXPECTED_AUTOMATION_EXPERIMENT_KEYWORDS_VARIANT_B)
                        && StringHelper.containsIgnoreCase(waitForElementVisible(AUTOMATION_EXPERIMENT_IMAGE_LOCATOR).getAttribute("src"),
                                "myhome_rec_next_FreeGames1.jpg");

            default:
                return false;
        }
    }

    /**
     * Verify the 'No Results Found' message is displayed.
     *
     * @return true if the text message is visible, false otherwise
     */
    public boolean verifyNoResultsFoundTextVisible() {
        return waitIsElementVisible(NO_RESULTS_FOUND_TEXT_LOCATOR);
    }
    
}