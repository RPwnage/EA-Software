package com.ea.originx.automation.lib.pageobjects.gamelibrary;

import com.ea.originx.automation.lib.helpers.I18NUtil;
import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.common.ContextMenu;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.utils.Waits;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.JavascriptExecutor;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * A page object that represents a game tile.
 *
 * @author glivingstone
 * @author palui
 */
public class GameTile extends EAXVxSiteTemplate {

    protected static final String GAME_TILE_CSS_TMPL = "origin-game-tile[offerid='%s']";
    protected static final String GAMETILE_IMG_CSS_TMPL = GAME_TILE_CSS_TMPL + " .origin-gametile-img";
    protected static final String GAMETILE_TITLE_CSS_TMPL = GAME_TILE_CSS_TMPL + " .l-origin-gametile-overlay .origin-gametile-title";

    // Game Tile overlay
    protected static final String GAMETILE_OVERLAY_CSS_TMPL = GAME_TILE_CSS_TMPL + " .l-origin-gametile-overlay";
    protected static final String GAMETILE_OVERLAY_PROGRESS_CSS_TMPL = GAMETILE_OVERLAY_CSS_TMPL + " origin-game-progress";
    protected static final String GAMETILE_OVERLAY_PERCENT_CSS_TMPL = GAMETILE_OVERLAY_PROGRESS_CSS_TMPL + " .otkprogress-percent";
    protected static final String GAMETILE_OVERLAY_STATUS_CSS_TMPL = GAMETILE_OVERLAY_CSS_TMPL + " .otkprogress-status";
    protected static final String GAMETILE_OVERLAY_PLAYABLE_STATUS_CSS_TMPL = GAMETILE_OVERLAY_PROGRESS_CSS_TMPL + " .l-progress-playable-status .progress-playable-status";

    // Violator
    protected static final String VIOLATOR_WRAPPER_LOCATOR = GAME_TILE_CSS_TMPL + " .l-origin-game-violator-wrapper";
    protected static final String VIOLATOR_TEXT_LOCATOR = GAME_TILE_CSS_TMPL + " .origin-game-violator-text";
    protected static final String[] VIOLATOR_KEYWORDS = {"Upgrade", "Game"};
    
    // Game States
    protected static final String GAME_IS_DOWNLOADABLE_CSS_TMPL = GAME_TILE_CSS_TMPL + " .origin-gametile[data-gamestate*='\"downloadable\":true']";
    protected static final String GAME_IS_DOWNLOADING_CSS_TMPL = GAME_TILE_CSS_TMPL + " .origin-gametile[data-gamestate*='\"downloading\":true']";
    protected static final String GAME_IS_INSTALLING_CSS_TMPL = GAME_TILE_CSS_TMPL + " .origin-gametile[data-gamestate*='\"phase\":\"INSTALLING\"']";
    protected static final String GAME_IS_PAUSED_CSS_TMPL = GAME_TILE_CSS_TMPL + " .origin-gametile[data-gamestate*='\"phase\":\"PAUSED\"']";
    protected static final String GAME_IS_PREPARING_CSS_TMPL = GAME_TILE_CSS_TMPL + " .origin-gametile[data-gamestate*='\"phase\":\"PREPARING\"']";
    protected static final String GAME_IS_INITIALIZING_CSS_TMPL = GAME_TILE_CSS_TMPL + " .origin-gametile[data-gamestate*='\"phase\":\"INITIALIZING\"']";
    protected static final String GAME_IS_READY_TO_PLAY_CSS_TMPL = GAME_TILE_CSS_TMPL + " .origin-gametile[data-gamestate*='\"playable\":true']";
    protected static final String GAME_IS_NOT_READY_TO_PLAY_CSS_TMPL = GAME_TILE_CSS_TMPL + " .origin-gametile[data-gamestate*='\"playable\":false']";
    protected static final String GAME_IS_READY_TO_INSTALL_CSS_TMPL = GAME_TILE_CSS_TMPL + " .origin-gametile[data-gamestate*='\"phase\":\"READYTOINSTALL\"']";
    protected static final String GAME_IS_QUEUED_CSS_TMPL = GAME_TILE_CSS_TMPL + " .origin-gametile[data-gamestate*='\"phase\":\"ENQUEUED\"']";
    protected static final String GAME_IS_UPDATING_CSS_TMPL = GAME_TILE_CSS_TMPL + " .origin-gametile[data-gamestate*='\"updating\":true']";
    protected static final String GAME_HAS_UPDATE_AVAILABLE_CSS_TMPL = GAME_TILE_CSS_TMPL + " .origin-gametile[data-gamestate*='\"updateAvailable\":true']";
    protected static final String GAME_IS_UNINSTALLABLE_CSS_TMPL = GAME_TILE_CSS_TMPL + " .origin-gametile[data-gamestate*='\"uninstallable\":true']";

    protected final String offerID;
    protected final By gameTileLocator;
    protected final By gameTileImgLocator;
    protected final By gameIsDownloadableLocator;
    protected final By gameIsDownloadingLocator;
    protected final By gameIsInstallingLocator;
    protected final By gameIsPausedLocator;
    protected final By gameIsReadyToPlayLocator;
    protected final By gameIsNotReadyToPlayLocator;
    protected final By gameIsPreparingLocator;
    protected final By gameIsInitializingLocator;
    protected final By gameIsReadyToInstallLocator;
    protected final By gameIsUpdatingLocator;
    protected final By gameHasUpdateAvailableLocator;
    protected final By gameViolatorWrapperLocator;
    protected final By gameViolatorTextLocator;
    protected final By gameIsUninstallable;
    protected final ContextMenu contextMenu;

    protected static final String MOVE_GAME = "Move Game";
    protected static final String LOCATE_GAME = "Locate Game";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     * @param offerID Offer ID String
     */
    public GameTile(WebDriver driver, String offerID) {
        super(driver);
        this.offerID = offerID;
        this.gameTileLocator = By.cssSelector(String.format(GAME_TILE_CSS_TMPL, offerID));
        this.gameTileImgLocator = By.cssSelector(String.format(GAMETILE_IMG_CSS_TMPL, offerID));
        this.gameIsDownloadableLocator = By.cssSelector(String.format(GAME_IS_DOWNLOADABLE_CSS_TMPL, offerID));
        this.gameIsDownloadingLocator = By.cssSelector(String.format(GAME_IS_DOWNLOADING_CSS_TMPL, offerID));
        this.gameIsInstallingLocator = By.cssSelector(String.format(GAME_IS_INSTALLING_CSS_TMPL, offerID));
        this.gameIsPausedLocator = By.cssSelector(String.format(GAME_IS_PAUSED_CSS_TMPL, offerID));
        this.gameIsReadyToPlayLocator = By.cssSelector(String.format(GAME_IS_READY_TO_PLAY_CSS_TMPL, offerID));
        this.gameIsNotReadyToPlayLocator = By.cssSelector(String.format(GAME_IS_NOT_READY_TO_PLAY_CSS_TMPL, offerID));
        this.gameIsPreparingLocator = By.cssSelector(String.format(GAME_IS_PREPARING_CSS_TMPL, offerID));
        this.gameIsInitializingLocator = By.cssSelector(String.format(GAME_IS_INITIALIZING_CSS_TMPL, offerID));
        this.gameIsReadyToInstallLocator = By.cssSelector(String.format(GAME_IS_READY_TO_INSTALL_CSS_TMPL, offerID));
        this.gameIsUpdatingLocator = By.cssSelector(String.format(GAME_IS_UPDATING_CSS_TMPL, offerID));
        this.gameHasUpdateAvailableLocator = By.cssSelector(String.format(GAME_HAS_UPDATE_AVAILABLE_CSS_TMPL, offerID));
        this.gameViolatorWrapperLocator = By.cssSelector(String.format(VIOLATOR_WRAPPER_LOCATOR, offerID));
        this.gameViolatorTextLocator = By.cssSelector(String.format(VIOLATOR_TEXT_LOCATOR, offerID));
        this.gameIsUninstallable = By.cssSelector(String.format(GAME_IS_UNINSTALLABLE_CSS_TMPL, offerID));
        this.contextMenu = new ContextMenu(driver, gameTileImgLocator);
    }

    ///////////////////////////////////////////////////////
    // Getters/Verifies for game tile attributes
    ///////////////////////////////////////////////////////
    /**
     * Verify if object has correct offer ID.
     *
     * @return true if verification passed, false otherwise
     */
    public boolean verifyOfferID() {
        return waitForElementVisible(gameTileLocator).getAttribute("offerid").equals(offerID);
    }

    /**
     * Verify if this game tile is visible.
     *
     * @return true if verification passed, false otherwise
     */
    public boolean isGameTileVisible() {
        return waitIsElementVisible(gameTileLocator, 2);
    }

    /**
     * Get game tile image src.
     *
     * @return String of the image src
     */
    public String getImageSrc() {
        return waitForElementVisible(gameTileImgLocator).getAttribute("src");
    }

    /**
     * Verify if a game tile has a Windows data image.
     *
     * @return true if the image is visible and its src indicates it is a data
     * image, false otherwise
     */
    public boolean isGameTileDataImageVisible() {
        return getImageSrc().startsWith("data:image");
    }

    /**
     * Get the 'GameSlideout' page object.
     *
     * @return The 'Game Slideout' page object
     */
    public GameSlideout openGameSlideout() {
        boolean alreadyClicked = false;
        if (hasViolator()) { // Violator may be in the way, try to click it first
            try {
                waitForElementClickable(gameViolatorWrapperLocator).click();
                alreadyClicked = true;
            } catch (TimeoutException e) {
                // Violator isn't clickable, move along.
            }
        }

        if (!alreadyClicked) {
            if (isTileOverlayVisible()) {
                waitForElementClickable(By.cssSelector(String.format(GAMETILE_OVERLAY_CSS_TMPL, offerID))).click();
            } else {
                waitForElementClickable(gameTileImgLocator).click();
            }
        }
        GameSlideout slideOut = new GameSlideout(driver);
        slideOut.waitForSlideout();
        return slideOut;
    }

    /**
     * Get the title of the current game tile.
     *
     * @return String of the name of the game
     */
    public String getTitle() {
        WebElement someElement = waitForElementPresent(By.cssSelector(String.format(GAMETILE_TITLE_CSS_TMPL, offerID)));
        return someElement.getAttribute("textContent").trim();
    }

    /**
     * Verify if game tile has matching game title.
     *
     * @param gameTitle Title to match
     * @return true if game title matches, false otherwise
     */
    public boolean verifyTitle(String gameTitle) {
        return getTitle().equals(gameTitle);
    }

	/**
	* Verifies opacity of the vault game tile when subscription is expired
	*
	*/
    public boolean verifyTileOpacity() {
        return waitForElementVisible(gameTileImgLocator).getCssValue("opacity").equals(".3");
    }

    ///////////////////////////////////////////////////////
    // Game tile context menu
    ///////////////////////////////////////////////////////
    /**
     * Start download using the context menu.
     */
    public void startDownload() {
        contextMenu.selectItem(I18NUtil.getMessage("download"));
    }

    /**
     * Start a queued download using the right-click menu.
     */
    public void downloadNow() {
        contextMenu.selectItem("Download Now");
    }

    /**
     * Select 'Remove from Library' from the game's right-click menu. Available
     * for vault games, vault upgrades, and non-Origin games.
     */
    public void removeFromLibrary() {
        contextMenu.selectItem("Remove from Library");
    }

    /**
     * Show game details using the right-click menu. This opens the 'game
     * slideout'.
     */
    public void showGameDetails() {
        contextMenu.selectItem("Show Game Details");
    }

    /**
     * Add game to favorites using the right-click menu.
     */
    public void addToFavorites() {
        contextMenu.selectItemContainingIgnoreCase("Add to Favorites");
    }

    /**
     * Remove game from favorites using the right-click menu.
     */
    public void removeFromFavorites() {
        contextMenu.selectItem("Remove from Favorites");
    }

    /**
     * Open game properties using the right-click menu. This opens the 'Game
     * Properties' dialog.
     */
    public void openGameProperties() {
        contextMenu.selectItem("Game Properties");
    }

    /**
     * Hide game from the library using the right-click menu.
     */
    public void hide() {
        contextMenu.selectItem("Hide");
    }

    /**
     * Start installation using the context menu.
     */
    public void install() {
        contextMenu.selectItem("Install");
    }

    /**
     * Uninstall using the context menu.
     */
    public void uninstall() {
        contextMenu.selectItem("Uninstall");
    }

    /**
     * Uninstall for a given locale
     */
    public void uninstallForLocale() {
        Waits.pollingWait(() -> contextMenu.verifyContextMenuLocatorVisible());
        contextMenu.selectItem(I18NUtil.getMessage("uninstall"));
    }

    /**
     * Pause the download using the context menu.
     */
    public void pauseDownload() {
        contextMenu.selectItem("Pause Download");
    }

    /**
     * Pause the download using the context menu without needing an exact match.
     */
    public void pauseDownloadCaseInsensitive() {
        contextMenu.selectItemContainingIgnoreCase("pause");
    }

    /**
     * Resume the download using the context menu.
     */
    public void resumeDownload() {
        contextMenu.selectItem("Resume Download");
    }

    /**
     * Resume the download using the context menu without needing an exact match.
     */
    public void resumeDownloadCaseInsensitive() {
        contextMenu.selectItemContainingIgnoreCase("resume");
    }

    /**
     * Cancel the download using the context menu.
     */
    public void cancelDownload() {
        contextMenu.selectItem("Cancel Download");
    }

    /**
     * Cancel the download using the context menu without needing an exact match.
     */
    public void cancelDownloadCaseInsensitive() {
        contextMenu.selectItemContainingIgnoreCase("cancel");
    }

    /**
     * Cancel the update using the context menu.
     */
    public void cancelUpdate() {
        contextMenu.selectItem("Cancel Update");
    }

    /**
     * Pause the update using the context menu.
     */
    public void pauseUpdate() {
        contextMenu.selectItem("Pause Update");
    }

    /**
     * Resume the update using the context menu.
     */
    public void resumeUpdate() {
        contextMenu.selectItem("Resume Update");
    }

    /**
     * Update the game using the context menu.
     */
    public void updateGame() {
        contextMenu.selectItem("Update Game");
    }

    /**
     * Play the entitlement using the right-click menu.
     */
    public void play() {
        String playLocale = I18NUtil.getMessage("play");
        contextMenu.selectItem(playLocale);
    }

    /**
     * Move entitlement using the right click context menu
     */
    public void moveGame() {
        contextMenu.selectItemUsingJavaScriptExecutor(MOVE_GAME);
    }

    /**
     * Locate entitlement using the right click context menu
     */
    public void locateGame() {
        contextMenu.selectItemUsingJavaScriptExecutor(LOCATE_GAME);
    }

    /**
     * Launch entitlement by JS-SDK, e.g., 'Origin.client.games.play("Origin.OFR.50.0000390")'
     *
     * https://docs.x.origin.com/jssdk/module-Origin.module_client.module_games.html#.play
     *
     * @param offerID offer id of the entitlement
     */
    public void playByJSSDK(String offerID) {
        String scriptToRun = String.format("Origin.client.games.play('%s')", offerID);
        jsExec.executeScript(scriptToRun);
    }

    /**
     * Repair the game using the right-click menu.
     */
    public void repair() {
        contextMenu.selectItem("Repair");
    }

    /**
     * Verify that the 'Cancel Install' context menu option is disabled.
     *
     * @return true if the context menu option is disabled, false otherwise
     */
    public boolean verifyCancelInstallDisabled() {
        return contextMenu.isItemDisabled("Cancel Install");

    }

    /**
     * Click 'Start Trial' for the entitlement using the right-click menu.
     */
    public void startTrial() {
        contextMenu.selectItem("Start Trial");
    }

    /**
     * Verify that the 'Pause Install' context menu option is disabled.
     *
     * @return true if the context menu option is disabled, false otherwise
     */
    public boolean verifyPauseInstallDisabled() {
        return contextMenu.isItemDisabled("Pause Install");
    }

    /**
     * Verify 'Remove from Library' option is displayed in the dropdown
     *
     * @return true if the option is present, false otherwise
     */
    public boolean verifyRemoveFromLibrary() {
        return contextMenu.isItemPresent("Remove from Library");
    }

    /**
     * Verify the 'Move game' option is displayed in the right-click dropdown menu.
     *
     * @return true if the menu item is visible, false otherwise
     */
    public boolean verifyMoveGameVisible() {
        return contextMenu.isItemPresent(MOVE_GAME);
    }

    /**
     * Verify the 'Locate game' option is displayed in the right-click dropdown menu.
     *
     * @return true if the menu item is visible, false otherwise
     */
    public boolean verifyLocateGameVisible() {
        return contextMenu.isItemPresent(LOCATE_GAME);
    }

    ///////////////////////////////////////////////////////
    // Game tile states
    ///////////////////////////////////////////////////////
    /**
     * Wait for the entitlement to get into a certain state, identified using a
     * By selector. The state is primarily determined by the 'data-gamestate'
     * attribute of the element.
     *
     * @param expectedLocator The By we are looking for to determine if the
     * entitlement is in the correct state
     * @return true if the entitlement is in the given state, false otherwise
     */
    private boolean waitForState(By expectedLocator) {
        return waitIsElementVisible(expectedLocator, 15);
    }

    /**
     * Wait for the entitlement to get into a certain state, identified using a
     * By selector. The state is primarily determined by the 'data-gamestate'
     * attribute of the element.
     *
     * @param expected The By we are looking for to determine if the entitlement
     * is in the correct state
     * @param seconds The time to wait for the state to be true, in seconds
     * @return true if the entitlement is in the given state, false otherwise
     */
    private boolean waitForState(By expected, int seconds) {
        int milliseconds = seconds * 1000;
        return Waits.pollingWait(() -> isElementVisible(expected), milliseconds, 1000, 0);
    }

    /**
     * Verify the entitlement is downloadable.
     *
     * @return true if the entitlement is downloadable, false otherwise
     */
    public boolean waitForDownloadable() {
        return waitForState(gameIsDownloadableLocator, 30);
    }

    /**
     * Wait to verify that the entitlement is installing.
     *
     * @return true if the entitlement is installing, false otherwise
     */
    public boolean waitForInstalling() {
        return waitForState(gameIsInstallingLocator);
    }

    /**
     * Wait to verify that the entitlement is installing.
     *
     * @param seconds The number of seconds to wait
     * @return True if the entitlement is installing, false otherwise
     */
    public boolean waitForInstalling(int seconds) {
        return waitForState(gameIsInstallingLocator, seconds);
    }

    /**
     * Verify the entitlement is downloading.
     *
     * @return true if the entitlement is downloading, false otherwise.
     */
    public boolean waitForDownloading() {
        return waitForState(gameIsDownloadingLocator);
    }

    /**
     * Wait for the entitlement to start downloading.
     *
     * @param seconds The number of seconds to wait
     * @return true if the entitlement starts downloading within wait time,
     * false otherwise
     */
    public boolean waitForDownloading(int seconds) {
        return waitForState(gameIsDownloadingLocator, seconds);
    }

    /**
     * Wait for the entitlement to start updating
     *
     * @param seconds The number of seconds to wait
     * @return true if the entitlement starts updating within wait time, false
     * otherwise
     */
    public boolean waitForUpdating(int seconds) {
        return waitForState(gameIsUpdatingLocator, seconds);
    }

    /**
     * Verify the download of the entitlement is paused.
     *
     * @return true if the entitlement is paused, false otherwise
     */
    public boolean waitForPaused() {
        return waitForState(gameIsPausedLocator);
    }

    /**
     * Wait for the entitlement to be ready to play.
     *
     * @param seconds The number of seconds to wait
     * @return true if the entitlement becomes ready to play, false otherwise
     */
    public boolean waitForReadyToPlay(int seconds) {
        return waitForState(gameIsReadyToPlayLocator, seconds);
    }

    /**
     * Verify the entitlement is ready to play
     *
     * @return true if the entitlement is ready to play, false otherwise
     */
    public boolean waitForReadyToPlay() {
        return waitForState(gameIsReadyToPlayLocator);
    }

    /**
     * Verify the entitlement is not ready to play.
     *
     * @param seconds the number of seconds to wait
     * @return true if the entitlement is not ready to play, false otherwise
     */
    public boolean waitForNotReadyToPlay(int seconds) {
        return waitForState(gameIsNotReadyToPlayLocator, seconds);
    }

    /**
     * Verify the entitlement is not ready to play.
     *
     * @return true if the entitlement is not ready to play, false otherwise
     */
    public boolean waitForNotReadyToPlay() {
        return waitForState(gameIsNotReadyToPlayLocator);
    }

    /**
     * Verify the entitlement is preparing to download.
     *
     * @return true if the entitlement is preparing, false otherwise
     */
    public boolean waitForPreparing() {
        return waitForState(gameIsPreparingLocator);
    }

    /**
     * Verify the entitlement is queued to download.
     *
     * @return true if the entitlement is queued, false otherwise
     */
    public boolean waitForQueued() {
        return waitForState(By.cssSelector(String.format(GAME_IS_QUEUED_CSS_TMPL, offerID)));
    }

    /**
     * Check if the entitlement is in a certain state, by using a By selector.
     * The state is primarily determined by the data-gamestate attribute of the
     * element.
     *
     * @param expected The By we are looking for to determine if the entitlement
     * is in the correct state.
     * @return true if the entitlement is in the given state, false otherwise
     */
    private boolean checkState(By expected) {
        return driver.findElements(expected).size() > 0;
    }

    /**
     * Check if the entitlement is downloadable.
     *
     * @return true is downloadable, false otherwise
     */
    public boolean isDownloadable() {
        return checkState(gameIsDownloadableLocator);
    }

    /**
     * Check if the entitlement is currently downloading.
     *
     * @return true if currently downloading, false otherwise
     */
    public boolean isDownloading() {
        return checkState(By.cssSelector(String.format(GAME_IS_DOWNLOADING_CSS_TMPL, offerID)));
    }

    /**
     * Check if the entitlement is installing.
     *
     * @return true if installing, false otherwise
     */
    public boolean isInstalling() {
        return checkState(By.cssSelector(String.format(GAME_IS_INSTALLING_CSS_TMPL, offerID)));
    }

    /**
     * Check if the entitlement is preparing.
     *
     * @return true if preparing, false otherwise
     */
    public boolean isPreparing() {
        return checkState(gameIsPreparingLocator);
    }

    /**
     * Check if the entitlement is ready to install.
     *
     * @return true if ready to install, false otherwise
     */
    public boolean isReadyToInstall() {
        return checkState(gameIsReadyToInstallLocator);
    }

    /**
     * Check if the entitlement is ready to play.
     *
     * @return True if ready to play, false otherwise
     */
    public boolean isReadyToPlay() {
        return checkState(gameIsReadyToPlayLocator);
    }

    /**
     * Check if the entitlement is queued.
     *
     * @return true if queued, false otherwise
     */
    public boolean isQueued() {
        return checkState(By.cssSelector(String.format(GAME_IS_QUEUED_CSS_TMPL, offerID)));
    }

    /**
     * Check if the entitlement is updating.
     *
     * @return true if updating, false otherwise
     */
    public boolean isUpdating() {
        return checkState(gameIsUpdatingLocator);
    }

    /**
     * Check if the entitlement is initializing.
     *
     * @return true if initializing, false otherwise
     */
    public boolean isInitializing() {
        return checkState(gameIsInitializingLocator);
    }

    /**
     * Check if the entitlement has an update available for download.
     *
     * @return true if an update is available, false otherwise
     */
    public boolean hasUpdateAvailable() {
        return checkState(gameHasUpdateAvailableLocator);
    }

    /**
     * Check if a download for the entitlement is paused.
     *
     * @return true if a download is paused, false otherwise
     */
    public boolean isPaused() {
        return checkState(gameIsPausedLocator);
    }

    /**
     * Check if the entitlement is uninstallable
     *
     * @return true if the entitlement is uninstallable, false otherwise
     */
    public boolean isUninstallable() {
        return checkState(gameIsUninstallable);
    }

    ///////////////////////////////////////////////////////
    // Game tile overlay
    ///////////////////////////////////////////////////////
    /**
     * Gets the current download percentage.
     *
     * @return The current download percentage
     */
    public int getDownloadPercentage() {
        WebElement percentComplete = waitForElementVisible(By.cssSelector(String.format(GAMETILE_OVERLAY_PERCENT_CSS_TMPL, offerID)));
        String downloadPercentage = percentComplete.getText();
        return Integer.parseInt(downloadPercentage.substring(0, downloadPercentage.length() - 1));

    }

    /**
     * Check if the entitlement is partially downloaded.
     *
     * @return true if partially downloaded, false otherwise
     */
    public boolean verifyPartialDownload() {
        WebElement percentComplete = waitForElementVisible(By.cssSelector(String.format(GAMETILE_OVERLAY_PERCENT_CSS_TMPL, offerID)));
        return percentComplete.getText().matches("\\d+%");
    }

    /**
     * Verify if entitlement has progress overlay playable status matching the
     * RegEx parameter.
     *
     * @param regex The regular expression to match
     *
     * @return true if overlay exists with matching text, false otherwise
     */
    private boolean verifyOverlayPlayableStatusMatches(String regex) {
        final By statusLocator = By.cssSelector(String.format(GAMETILE_OVERLAY_PLAYABLE_STATUS_CSS_TMPL, offerID));
        try {
            WebElement statusIndicator = waitForElementVisible(statusLocator, 2);
            return statusIndicator.getText().matches(regex);
        } catch (TimeoutException e) {
            _log.warn("Could not detect visibility of the status indicator located by: " + statusLocator.toString());
            return false;
        }
    }

    /**
     * Verify if the entitlement has a progress overlay playable status
     * indicating it is playable at certain % (less than 100%).
     *
     * @return true if overlay status is 'PLAYABLE AT ..%' (less than 100%),
     * false otherwise
     */
    public boolean verifyOverlayPlayableStatusPercent() {
        return verifyOverlayPlayableStatusMatches("PLAYABLE AT \\d{1,2}%");
    }

    /**
     * Verify if the entitlement has a progress overlay playable status
     * indicating it is now playable.
     *
     * @return true if overlay status is 'NOW PLAYABLE', false otherwise
     */
    public boolean verifyOverlayPlayableStatusNowPlayable() {
        return verifyOverlayPlayableStatusMatches("NOW PLAYABLE");
    }

    /**
     * Verify if entitlement has progress overlay status matching the text
     * parameter.
     *
     * @param text The text to match
     *
     * @return true if overlay exists with matching text, false otherwise
     */
    private boolean verifyOverlayStatus(String text) {
        WebElement statusIndicator = waitForElementVisible(By.cssSelector(String.format(GAMETILE_OVERLAY_STATUS_CSS_TMPL, offerID)));
        return statusIndicator.getText().equals(text);
    }

    /**
     * Check if the entitlement has a progress overlay status indicating it is
     * preparing for download.
     *
     * @return true if overlay status is 'PREPARING', false otherwise
     */
    public boolean verifyOverlayStatusPreparing() {
        return verifyOverlayStatus("PREPARING");
    }

    /**
     * Check if the entitlement has a progress overlay status indicating it is
     * now downloading.
     *
     * @return true if overlay status is 'DOWNLOADING', false otherwise
     */
    public boolean verifyOverlayStatusDownloading() {
        return verifyOverlayStatus("DOWNLOADING");
    }

    /**
     * Check if the entitlement has a progress overlay status indicating it is
     * now installing.
     *
     * @return true if overlay status is 'INSTALLING', false otherwise
     */
    public boolean verifyOverlayStatusInstalling() {
        return verifyOverlayStatus("INSTALLING");
    }

    /**
     * Check if the entitlement has a progress overlay status indicating it is
     * now paused.
     *
     * @return true if overlay status is 'PAUSED', false otherwise
     */
    public boolean verifyOverlayStatusPaused() {
        return verifyOverlayStatus("PAUSED");
    }

    /**
     * Check if the entitlement has a progress overlay status indicating that
     * downloading and installing has completed.
     *
     * @return true if overlay status is 'COMPLETE', false otherwise
     */
    public boolean verifyOverlayStatusComplete() {
        return verifyOverlayStatus("COMPLETE");
    }

    /**
     * Check if the entitlement has a progress overlay status indicating that
     * the download is queued.
     *
     * @return true if the overlay status is 'DOWNLOAD IN QUEUE', false
     * otherwise
     */
    public boolean verifyOverlayStatusQueued() {
        return verifyOverlayStatus("DOWNLOAD IN QUEUE");
    }

    /**
     * Check if the game tile's overlay is visible.
     *
     * @return true if queued, false otherwise
     */
    public boolean isTileOverlayVisible() {
        return isElementVisible(By.cssSelector(String.format(GAMETILE_OVERLAY_CSS_TMPL, offerID)));
    }

    ///////////////////////////////////////////////////////
    // Game tile violator
    ///////////////////////////////////////////////////////
    /**
     * Check if the entitlement has a visible violator.
     *
     * @return true if there is a visible violator on the entitlement,
     * false otherwise
     */
    public boolean hasViolator() {
        return isElementVisible(gameViolatorWrapperLocator);
    }

    /**
     * Check if the entitlement has a violator stating membership is expired.
     *
     * @return true if there is a visible violator on the entitlement with the
     * word 'expired', false otherwise
     */
    public boolean verifyViolatorStatingMembershipIsExpired() {
        sleep(1000);  // wait for animation to be finished
        WebElement violator = waitForElementVisible(gameViolatorTextLocator);
        return StringHelper.containsIgnoreCase(violator.getText(), "expired");
    }
    
    /**
     * Check if the entitlement has a violator stating upgrade the game is
     * possible
     *
     * @return true if there is a visible violator on the entitlement contains
     * keywords, false otherwise
     */
    public boolean verifyViolatorUpgradeGameVisible() {
        String violatorText = waitForElementVisible(gameViolatorTextLocator).getText();
        return StringHelper.containsAnyIgnoreCase(violatorText, VIOLATOR_KEYWORDS);
    }

    /**
     * Verify the game tile has a violator present
     *
     * @return true if the violator is present, false otherwise
     */
    public boolean verifyViolatorPresent() {
        return waitIsElementPresent(gameViolatorTextLocator);
    }

    
    /**
     * Verify the color of the violator warning icon
     * 
     * @return true if the violator warning icon is red, false otherwise 
     */
    public boolean verifyViolatorIconColor() {
        WebElement violatorIcon = waitForElementVisible(gameViolatorWrapperLocator).findElement(By.cssSelector(" .otkicon-warning"));
        return getColorOfElement(violatorIcon).equalsIgnoreCase(OriginClientData.VIOLATOR_WARNING_ICON_COLOR);
    }
}