package com.ea.originx.automation.lib.pageobjects.discover;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * The tiles on 'My Home' that represents recently played or recently installed
 * games.
 *
 * @author sbentley
 */
public class QuickLaunchTile extends EAXVxSiteTemplate {

    protected static final String QUICK_LAUNCH_TILE_CSS = "origin-home-quick-launch otkex-hometile[index=\"%s\"]";
    protected static final String QUICK_LAUNCH_TITLE_CSS = " h1.home-tile-header";
    protected static final String QUICK_LAUNCH_ACHIEVEMENTS_CSS = " otkex-hometile-achievements[index=\"%s\"]";
    protected static final String QUICK_LAUNCH_HOME_TILE_CSS = " .home-tile";
    protected static final String QUICK_LAUNCH_SMALL_TILE_CSS = " .home-tile-small";
    protected static final String QUICK_LAUNCH_EYEBROW_CSS = " span.home-tile-eyebrow";
    protected static final String QUICK_LAUNCH_SMALL_TILE_DISABLED_CSS = " .home-tile.home-tile-small.home-tile-disabled";
    protected static final String QUICK_LAUNCH_TILE_HOVER_CSS = " .home-tile:hover";

    protected final String offerID;
    protected final By quickLaunchTileLocator;
    protected final By titleLocator;
    protected final By eyebrowLocator;
    protected final By achievementLocator;
    protected final By smallTileLocator;
    protected final By hoverTileLocator;
    protected final By disabledTile;

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * The tile state enum
     */
    public enum TileState {
        PLAY("Play"),
        DOWNLOAD("Download"),
        PAUSE_DOWNLOAD("Pause Download"),
        RESUME_DOWNLOAD("Resume Download"),
        PREPARING("Preparing"),
        INSTALLING("Installing");

        private final String tileStateText;

        private TileState(final String state) {
            this.tileStateText = state;
        }

        @Override
        public String toString() {
            return tileStateText;
        }
    }

    public QuickLaunchTile(WebDriver driver, String offerID) {
        super(driver);
        this.offerID = offerID;
        this.quickLaunchTileLocator = By.cssSelector(String.format(QUICK_LAUNCH_TILE_CSS, offerID));
        this.titleLocator = By.cssSelector(String.format(QUICK_LAUNCH_TILE_CSS + QUICK_LAUNCH_TITLE_CSS, offerID));
        this.eyebrowLocator = By.cssSelector(String.format(QUICK_LAUNCH_TILE_CSS + QUICK_LAUNCH_EYEBROW_CSS, offerID));
        this.achievementLocator = By.cssSelector(String.format(QUICK_LAUNCH_TILE_CSS + QUICK_LAUNCH_ACHIEVEMENTS_CSS, offerID, offerID));
        this.smallTileLocator = By.cssSelector(String.format(QUICK_LAUNCH_TILE_CSS + QUICK_LAUNCH_SMALL_TILE_CSS, offerID));
        this.hoverTileLocator = By.cssSelector(String.format(QUICK_LAUNCH_TILE_CSS + QUICK_LAUNCH_TILE_HOVER_CSS, offerID));
        this.disabledTile = By.cssSelector(String.format(QUICK_LAUNCH_TILE_CSS + QUICK_LAUNCH_SMALL_TILE_DISABLED_CSS, offerID));
    }

    /**
     * Check to see if the current 'Quick Launch' tile is visible.
     *
     * @return true if the current tile is visible, false otherwise
     */
    public boolean verifyTileVisible() {
        return waitIsElementVisible(quickLaunchTileLocator);
    }

    /**
     * Gets the current state of the 'Quick Launch' tile.
     *
     * @return The name of the current Quick Launch state. Returns empty string
     * if nothing is found.
     */
    public String getEyebrowStateVisible() {
        WebElement eyebrow = null;

        try {
            eyebrow = waitForElementVisible(eyebrowLocator);
            String eyebrowText = eyebrow.getText();
            _log.debug("Found Quick Launch Eyebrow Text: " + eyebrowText);
            return eyebrowText;
        } catch (TimeoutException exception) {
            _log.debug("Could not find Eyebrow text, returning empty string");
            return "";
        }
    }

    /**
     * Gets the current state of the Quick Launch tile. Useful if tile is hidden
     * behind the following dialogs DownloadInfoDialog, DownloadGameMsgBox,
     * DownloadLanguageSelectionDialog, DownloadEULASummaryDialog,
     * NetworkProblemDialog, DownloadProblemDialog, PunkbusterWarningDialog,
     * which appear when a download in started through Quick Launch on My Home.
     *
     * @return The name of the current Quick Launch state. Returns empty string
     * if nothing is found.
     */
    public String getEyebrowStatePresent() {
        WebElement eyeBrow = null;

        try {
            eyeBrow = waitForElementPresent(eyebrowLocator);
            String eyebrowText = eyeBrow.getText();
            _log.debug("Found Quick Launch Eyebrow Text: " + eyebrowText);
            return eyebrowText;
        } catch (TimeoutException exception) {
            _log.debug("Unable to find eyebrow element");
            return "";
        }
    }

    /**
     * Compares the text String given to the text of 'Quick Launch' eyebrow text.
     *
     * @return true if the text String given is the same as the 'Quick Launch'
     * eyebrow text, false otherwise
     */
    private boolean textToEyebrowCompareVisible(String text) {
        String eyebrowText = getEyebrowStateVisible();
        _log.debug("Comparing " + text + " to Eyebrow Text: " + eyebrowText);
        return eyebrowText.equalsIgnoreCase(text);
    }

    /**
     * Compares the text String given to the text of 'Quick Launch' eyebrow text.
     *
     * @return true if the text string given is the same as the 'Quick Launch'
     * eyebrow text, false otherwise
     */
    private boolean textToEyebrowCompareVisible(TileState state) {
        return textToEyebrowCompareVisible(state.toString());
    }

    /**
     * Compares the text String given to the text of 'Quick Launch' eyebrow text.
     *
     * @return true if the text String given is the same as the 'Quick Launch'
     * eyebrow text, false otherwise
     */
    private boolean textToEyebrowComparePresent(String text) {
        String eyebrowText = getEyebrowStatePresent();
        _log.debug("Comparing " + text + " to Eyebrow Text: " + eyebrowText);
        return eyebrowText.equalsIgnoreCase(text);
    }

    /**
     * Compares the text String given to the text of 'Quick Launch' eyebrow text.
     *
     * @return true if the text string given is the same as the Quick Launch
     * eyebrow text, false otherwise
     */
    private boolean textToEyebrowComparePresent(TileState state) {
        return textToEyebrowComparePresent(state.toString());
    }

    /**
     * Checks to see if the String given is the same as the current eyebrow text.
     *
     * @param state The state to check the tile against
     * @return true if the eyebrow text is the same as the given string, false
     * otherwise
     */
    public boolean verifyEyebrowStateTextVisible(TileState state) {
        return Waits.pollingWait(() -> textToEyebrowCompareVisible(state));
    }

    /**
     * Checks to see if the String given is the same as the current eyebrow text.
     *
     * @param state The state to check the tile against
     * @return true if the eyebrow text is the same as the given string, false
     * otherwise
     */
    public boolean verifyEyebrowStateTextPresent(TileState state) {
        return Waits.pollingWait(() -> textToEyebrowComparePresent(state));
    }

    /**
     * Checks to see if the String given is the same as the current eyebrow text.
     *
     * @param state The state to check the tile against
     * @param maxWaitTime The length of the test in milliseconds
     * @param pollInterval The time between retrying a test
     * @param minWaitTime The amount of time to wait before starting a test
     * @return true if the eyebrow text is the same as the given String,
     * false otherwise
     */
    public boolean verifyEyebrowStateTextVisible(TileState state, int maxWaitTime, int pollInterval, int minWaitTime) {
        return Waits.pollingWait(() -> textToEyebrowCompareVisible(state), maxWaitTime, pollInterval, minWaitTime);
    }

    /**
     * Checks to see if the current 'Quick Launch' tile is in 'Play' state.
     *
     * @return true if tile is in 'Play' state, false otherwise
     */
    public boolean verifyEyebrowStatePlayVisible() {
        return verifyEyebrowStateTextVisible(TileState.PLAY);
    }

    /**
     * Checks to see if the current 'Quick Launch' tile is in 'Play' state.
     *
     * @param maxWaitTime The length of the test in milliseconds
     * @param pollInterval The time between retrying a test
     * @param minWaitTime The amount of time to wait before starting a test
     * @return true if tile is in 'Play' state, false otherwise
     */
    public boolean verifyEyebrowStatePlayVisible(int maxWaitTime, int pollInterval, int minWaitTime) {
        return verifyEyebrowStateTextVisible(TileState.PLAY, maxWaitTime, pollInterval, minWaitTime);
    }

    /**
     * Checks to see if the current 'Quick Launch' tile is in 'Preparing' state.
     *
     * @return true if tile is in 'Preparing' state, false otherwise
     */
    public boolean verifyEyebrowStatePreparingPresent() {
        return verifyEyebrowStateTextPresent(TileState.PREPARING);
    }

    /**
     * Checks to see if the current 'Quick Launch' tile is in 'Pause Download'
     * state.
     *
     * @return true if tile is in 'Pause Download' state,
     * false otherwise
     */
    public boolean verifyEyebrowStatePauseDownloadVisible() {
        return verifyEyebrowStateTextVisible(TileState.PAUSE_DOWNLOAD);
    }

    /**
     * Checks to see if the current 'Quick Launch' tile is in 'Resume Download'
     * state.
     *
     * @return true if tile is in 'Resume Download' state,
     * false otherwise
     */
    public boolean verifyEyebrowStateResumeDownloadVisible() {
        return verifyEyebrowStateTextVisible(TileState.RESUME_DOWNLOAD);
    }

    /**
     * Checks to see if the current 'Quick Launch' tile is in 'Installing' state.
     *
     * @param maxWaitTime The length of the test in milliseconds
     * @param pollInterval The time between retrying a test
     * @param minWaitTime The amount of time to wait before starting a test
     * @return true if tile is in the 'Installing' state, false otherwise
     */
    public boolean verifyEyebrowStateInstallingVisible(int maxWaitTime, int pollInterval, int minWaitTime) {
        return verifyEyebrowStateTextVisible(TileState.INSTALLING, maxWaitTime, pollInterval, minWaitTime);
    }

    /**
     * Checks to see if the current 'Quick Launch' tile is in 'Download' state
     *
     * @return true if tile is in 'Download' state
     */
    public boolean verifyEyebrowStateDownloadVisible() {
        return verifyEyebrowStateTextVisible(TileState.DOWNLOAD);
    }

    /**
     * Verifies if the 'Quick Launch' title is the same as the given entitlement.
     *
     * @param entitlement The entitlement to check against the 'Quick Launch' title
     * @return true if the title of the entitlement and the 'Quick Launch' match,
     * false otherwise
     */
    public boolean verifyQuickLaunchTitle(EntitlementInfo entitlement) {
        String entitlementName = entitlement.getName();
        return Waits.pollingWait(() -> getQuickLaunchTitle().equalsIgnoreCase(entitlementName));
    }

    /**
     * Verifies if the 'Quick Launch' title is the same as the given entitlement
     * name
     *
     * @param offerName the name of the offer to check against the tile
     * @return true if the title of the entitlement and 'Quick Launch' match
     */
    public boolean verifyQuickLaunchTitle(String offerName) {
        return Waits.pollingWait(() -> getQuickLaunchTitle().equalsIgnoreCase(offerName));
    }

    /**
     * Returns the title of the current 'Quick Launch' tile.
     *
     * @return A String of the current title or empty string if not found
     */
    public String getQuickLaunchTitle() {
        WebElement title = null;

        try {
            title = waitForElementVisible(titleLocator);
            return title.getText();
        } catch (TimeoutException exception) {
            _log.debug("Unable to find Quick Launch title");
            return "";
        }
    }

    /**
     * Verifies if the 'Achievement Progress' appears on 'Quick Launch' tile.
     *
     * @return true if 'Achievement Progress' appears, false otherwise
     */
    public boolean verifyAchievementsVisible() {
        return waitIsElementVisible(achievementLocator);
    }

    /**
     * Verifies if the hover element appears and the hover element has a
     * transform attribute.
     *
     * @return true if hover element and transform attribute appear,
     * false otherwise
     */
    public boolean verifyExpandHoverAnimation() {
        hoverOnElement(waitForElementVisible(quickLaunchTileLocator));

        //home-tile:hover only appears when hovered on quick launch tile
        WebElement hoverTile = null;

        try {
            hoverTile = waitForElementVisible(hoverTileLocator);
            //The transform attribute that scales the element
            String transformAttribute = hoverTile.getCssValue("transform");
            return transformAttribute != null;
        } catch (TimeoutException exception) {
            return false;
        }
    }

    /**
     * Clicks on the current 'Quick Launch' tile.
     */
    public void clickTile() {
        waitForElementClickable(quickLaunchTileLocator).click();
    }

    /**
     * Checks if the current 'Quick Launch' tile is grayed (faded) out.
     *
     * @return true if the current 'Quick Launch' tile is grayed out,
     * false otherwise
     */
    public boolean verifyTileGrayedOut() {
        //Tile might be hidden behind dialog
        WebElement tile = null;
        try {
            tile = waitForElementPresent(disabledTile);
            String opacity = tile.getCssValue("opacity");
            return StringHelper.extractNumberFromText(opacity) < 1.0;
        } catch (TimeoutException exception) {
            _log.debug("Unable to find tile");
            return false;
        }
    }

    /**
     * Checks to see if the current tile is not grayed (faded) out. This is a
     * faster then checking for false on verifyTileGrayedOut().
     *
     * @return true if the current tile is not grayed out,
     * false otherwise
     */
    public boolean verifyTileNotGrayedOut() {
        return Waits.pollingWait(() -> !isElementVisible(disabledTile));
    }

    /**
     * Verifies the title is visible on the 'Quick Launch' tile.
     *
     * @return true if title is visible, false otherwise
     */
    public boolean verifyTitleVisible() {
        return waitIsElementVisible(titleLocator);
    }

    /**
     * Verifies the Eyebrow is visible on the 'Quick Launch' tile.
     *
     * @return true if eyebrow is visible, false otherwise
     */
    public boolean verifyEyebrowVisible() {
        return waitIsElementVisible(eyebrowLocator);
    }
}