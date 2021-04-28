package com.ea.originx.automation.lib.pageobjects.gamelibrary;

import com.ea.originx.automation.lib.pageobjects.common.ContextMenu;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.JavascriptExecutor;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * A page object that represents the cogwheel at a game slideout.
 *
 * @author palui
 */
public class GameSlideoutCogwheel extends ContextMenu {

    protected final String cogwheelCSS;
    protected final By cogwheelImageLocator;

    protected static final String MOVE_GAME = "Move Game";
    protected static final String LOCATE_GAME = "Locate Game";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver      Selenium WebDriver
     * @param cogwheelCSS Specification for locating the cogwheel in the game
     *                    slideout
     */
    public GameSlideoutCogwheel(WebDriver driver, String cogwheelCSS) {
        super(driver, By.cssSelector(cogwheelCSS + " > i"));
        this.cogwheelCSS = cogwheelCSS;
        this.cogwheelImageLocator = By.cssSelector(cogwheelCSS + " > i");
    }

    /**
     * Verify if cogwheel is visible at the game slideout.
     *
     * @return true if the cogwheel is visible, false otherwise
     */
    public boolean isVisible() {
        return waitIsElementVisible(cogwheelImageLocator, 2);
    }

    /**
     * Start download using the cogwheel context menu.
     */
    public void download() {
        selectItem("Download");

    }

    /**
     * Pause game download using the cogwheel context menu.
     */
    public void pause() {
        selectItem("Pause Download");
    }

    /**
     * Resume game download using the cogwheel context menu.
     */
    public void resume() {
        selectItem("Resume Download");
    }

    /**
     * Cancel game download using the cogwheel context menu.
     */
    public void cancel() {
        selectItem("Cancel Download");
    }

    /**
     * Select 'Remove From Library' from the right-click menu.
     */
    public void removeFromLibrary() {
        selectItem("Remove from Library");
    }
    
    /**
     * Verify the 'Setings' dropdown 'Remove from Library' menu item is visible.
     * 
     * @return true if the menu item is visible, false otherwise
     */
    public boolean verifyRemoveFromLibraryVisible() {
        return isItemPresent("Remove from Library");
    }

    /**
     * Add game to favorites using the right-click menu.
     */
    public void addToFavorites() {
        selectItemContainingIgnoreCase("Add to Favorites");
    }

    /**
     * Remove game from favorites using the right-click menu.
     */
    public void removeFromFavorites() {
        selectItem("Remove from Favorites");
    }

    /**
     * Show game properties using the right-click menu. This opens the 'game
     * properties' dialog.
     */
    public void showGameProperties() {
        selectItem("Game Properties"); // Or should it be Show Game Details?
    }

    /**
     * Hide game from the library using the right-click menu.
     */
    public void hide() {
        selectItem("Hide");
    }

    /**
     * Play the entitlement using the right-click menu.
     */
    public void play() {
        selectItem("Play");
    }

    /**
     * Verify the 'Move game' option is displayed in the right-click menu.
     *
     * @return true if the menu item is visible, false otherwise
     */
    public boolean verifyMoveGameVisible() {
        return isItemPresent(MOVE_GAME);
    }

    /**
     * Verify the 'Locate game' option is displayed in the right-click menu.
     *
     * @return true if the menu item is visible, false otherwise
     */
    public boolean verifyLocateGameVisible() {
        return isItemPresent(LOCATE_GAME);
    }

    /**
     * Move entitlement using the right click context menu
     */
    public void moveGame() {
        selectItemUsingJavaScriptExecutor(MOVE_GAME);
    }

    /**
     * Locatee entitlement using the right click context menu
     */
    public void locateGame() {
        selectItemUsingJavaScriptExecutor(LOCATE_GAME);
    }
}