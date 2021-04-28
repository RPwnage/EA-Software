package com.ea.originx.automation.lib.pageobjects.gamelibrary;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.utils.Waits;
import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.LogManager;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object that represents a game tile in 'Download Queue'.
 *
 * @author mkalaivanan
 */
public class DownloadQueueTile extends EAXVxSiteTemplate {

    protected static final String DOWNLOAD_QUEUE_TILE_CSS_TMPL = "origin-download-manager-queue origin-download-manager-game[offerid='%s']";
    protected static final String DOWNLOAD_QUEUE_TILE_XPATH_TMPL = "//origin-download-manager-game[contains(@offerid, '%s')]";

    // Buttons
    protected static final String BUTTON_CONTROLS_XPATH_TMPL = DOWNLOAD_QUEUE_TILE_XPATH_TMPL + "//div[contains(@class, 'origin-download-controls')]";
    protected static final String CANCEL_CLEAR_BUTTON_XPATH_TMPL = BUTTON_CONTROLS_XPATH_TMPL + "//*[contains(@class, 'otkicon') and contains(@class, 'otkicon-closecircle')]";
    protected static final String DOWNLOAD_BUTTON_XPATH_TMPL = BUTTON_CONTROLS_XPATH_TMPL + "//i[@class='otkicon otkicon-downloadnegative origin-download-icons']";
    protected static final String PAUSE_BUTTON_XPATH_TMPL = BUTTON_CONTROLS_XPATH_TMPL + "//*[contains(@class, 'otkicon') and contains(@class, 'otkicon-pausecircle')]//..";
    protected static final String PLAY_BUTTON_XPATH_TMPL = BUTTON_CONTROLS_XPATH_TMPL + "//*[contains(@class, 'otkicon') and contains(@class, 'otkicon-play-with-circle')]";

    // Tile information
    protected static final String PROGRESS_BAR_VALUE_CSS_TMPL = DOWNLOAD_QUEUE_TILE_CSS_TMPL + " .otkprogress-bar .otkprogress-value";
    protected static final String PACKART_IMG_CSS_TMPL = DOWNLOAD_QUEUE_TILE_CSS_TMPL + " div[class*='origin-download'] img[class*='packart']";
    protected static final String DOWNLOAD_INFORMATION_CSS_TMPL = DOWNLOAD_QUEUE_TILE_CSS_TMPL + " .origin-download-current-status-right .origin-download-current-info";
    protected static final String DOWNLOAD_SPEED_CSS_TMPL = DOWNLOAD_INFORMATION_CSS_TMPL + " .origin-download-current-rate";
    protected static final String PERCENTAGE_PROGRESS_CSS_TMPL = DOWNLOAD_INFORMATION_CSS_TMPL + " .origin-download-current-percent";
    protected static final String CURRENT_PHASE_CSS_TMPL = DOWNLOAD_INFORMATION_CSS_TMPL + " .origin-download-current-phase";
    protected static final String CURRENT_ETA_CSS_TMPL = DOWNLOAD_INFORMATION_CSS_TMPL + " .origin-download-current-time";
    protected static final String DOWNLOADED_AND_TOTAL_SIZE_CSS_TMPL = DOWNLOAD_INFORMATION_CSS_TMPL + " .origin-download-current-remaining";
    protected static final String PLAYABLE_TEXT_CSS_TMPL = DOWNLOAD_QUEUE_TILE_CSS_TMPL + " .origin-download-current-status-right .origin-download-playable .origin-download-playabletext.otktitle-5.otktitle-5-caps";
    protected static final String PLAYABLE_TIP_CSS_TMPL = DOWNLOAD_QUEUE_TILE_CSS_TMPL + " .origin-download-current-status-right .origin-download-playabletext";
    protected static final String BACKGROUND_ARTWORK_TMPL = DOWNLOAD_QUEUE_TILE_CSS_TMPL + " .origin-download-current-poster";
    protected static final String QUEUED_PERCENTAGE_PROGRESS_CSS_TMPL = DOWNLOAD_QUEUE_TILE_CSS_TMPL + " .origin-download-tile-percent";

    // Buttons
    protected final By cancelClearButtonLocator;
    protected final By downloadButtonLocator;
    protected final By pauseButtonLocator;
    protected final By playButtonLocator;

    // Tile Information
    protected final By queueTileLocator;
    protected final By packartLocator;
    protected final By progressBarValueLocator;
    protected final By percentageProgress;
    protected final By downloadSpeedLocator;
    protected final By currentPhaseLocator;
    protected final By currentETALocator;
    protected final By downloadedAndTotalSizeLocator;
    protected final By playableTextLocator;
    protected final By playableTipLocator;
    protected final By backgroundArtLocator;
    protected final By queuedPercentageProgress;

    private static final org.apache.logging.log4j.Logger _log = LogManager.getLogger(GameTile.class);

    /**
     * Status type for a 'Download Queue' tile. Changes as the game goes from
     * queued to downloading (active), and to complete.
     */
    private enum StatusType {
        QUEUED("inQueue"), ACTIVE("active"), COMPLETE("complete");
        private final String statusString;

        StatusType(String statusString) {
            this.statusString = statusString;
        }

        /**
         * Get 'Download Queue' tile status type given the status String.
         *
         * @param statusString The status string to match
         *
         * @return {@link StatusType)} with matching status string, null
         * otherwise
         */
        private static StatusType getStatusType(String statusString) {
            for (StatusType type : StatusType.values()) {
                if (type.statusString.equals(statusString)) {
                    return type;
                }
            }
            throw new RuntimeException(String.format("Cannot find status type for '%s'. Perhaps a new type should be added?", statusString));
        }
    }

    public DownloadQueueTile(WebDriver driver, String offerId) {
        super(driver);

        // Buttons
        this.cancelClearButtonLocator = By.xpath(String.format(CANCEL_CLEAR_BUTTON_XPATH_TMPL, offerId));
        this.downloadButtonLocator = By.xpath(String.format(DOWNLOAD_BUTTON_XPATH_TMPL, offerId));
        this.pauseButtonLocator = By.xpath(String.format(PAUSE_BUTTON_XPATH_TMPL, offerId));
        this.playButtonLocator = By.xpath(String.format(PLAY_BUTTON_XPATH_TMPL, offerId));

        // Tile Information
        this.queueTileLocator = By.cssSelector(String.format(DOWNLOAD_QUEUE_TILE_CSS_TMPL, offerId));
        this.packartLocator = By.cssSelector(String.format(PACKART_IMG_CSS_TMPL, offerId));
        this.progressBarValueLocator = By.cssSelector(String.format(PROGRESS_BAR_VALUE_CSS_TMPL, offerId));
        this.percentageProgress = By.cssSelector(String.format(PERCENTAGE_PROGRESS_CSS_TMPL, offerId));
        this.downloadSpeedLocator = By.cssSelector(String.format(DOWNLOAD_SPEED_CSS_TMPL, offerId));
        this.currentPhaseLocator = By.cssSelector(String.format(CURRENT_PHASE_CSS_TMPL, offerId));
        this.currentETALocator = By.cssSelector(String.format(CURRENT_ETA_CSS_TMPL, offerId));
        this.downloadedAndTotalSizeLocator = By.cssSelector(String.format(DOWNLOADED_AND_TOTAL_SIZE_CSS_TMPL, offerId));
        this.playableTextLocator = By.cssSelector(String.format(PLAYABLE_TEXT_CSS_TMPL, offerId));
        this.playableTipLocator = By.cssSelector(String.format(PLAYABLE_TIP_CSS_TMPL, offerId));
        this.backgroundArtLocator = By.cssSelector(String.format(BACKGROUND_ARTWORK_TMPL, offerId));
        this.queuedPercentageProgress = By.cssSelector(String.format(QUEUED_PERCENTAGE_PROGRESS_CSS_TMPL, offerId));
    }

    /**
     * Check if the 'Play' button on a PI entitlement is disabled.
     *
     * @return true if is disabled, false otherwise
     */
    public boolean isPlayButtonDisabled() {
        return isButtonDisabled(playButtonLocator);
    }
    
    /**
     * Check if the 'Play' button on a PI entitlement is active.
     *
     * @return true if is enabled, false otherwise
     */
    public boolean isPlayButtonActive() {
        return waitForElementVisible(playButtonLocator).isEnabled();
    }

    /**
     * Check if the 'Pause' button is disabled.
     *
     * @return true if is disabled, false otherwise
     */
    public boolean isPauseButtonDisabled() {
        return isButtonDisabled(pauseButtonLocator);
    }

    /**
     * Check if the 'Cancel' button is disabled.
     *
     * @return true if is disabled, false otherwise
     */
    public boolean isDownloadCancelButtonDisabled() {
        return isButtonDisabled(cancelClearButtonLocator);
    }

    /**
     * Verify if the given button is disabled.
     *
     * @param locator The locator of the button to check
     * @return true if button is disabled, false otherwise
     */
    private boolean isButtonDisabled(By locator) {
        return waitForElementVisible(locator).getAttribute("class").contains("isdisabled");
    }

    /**
     * Get the download percentage (as a String) on the current 'Download Queue' tile.
     *
     * @return Download percentage as a String, or throw TimeoutException if not
     * found
     */
    public String getDownloadPercentage() {
        return waitForElementVisible(percentageProgress).getText();
    }

    /**
     * Get current download percentage (as an int) on the current 'Download Queue'
     * tile.
     *
     * @return Download percentage as an int, or throw TimeoutException if not
     * found
     */
    public int getDownloadPercentageValue() {
        // Use chop to remove the last '%' character before converting String to int
        return Integer.parseInt(StringUtils.chop(getDownloadPercentage()));
    }

    /**
     * Verify the download percentage on a game tile is equal to the expected
     * download percentage.
     *
     * @param expectedDownloadPercentage The expected progress of the download
     * @return true if the download percentage equals the expected value,
     * false otherwise
     */
    public boolean verifytDownloadPercentage(String expectedDownloadPercentage) {
        return getDownloadPercentage().equals(expectedDownloadPercentage);
    }

    /**
     * Get the current progress bar percentage.
     *
     * @return The current progress percentage
     */
    public int getProgressBarPercentageValue() {
        return getProgressBarValue(progressBarValueLocator);
    }

    /**
     * Verify the current download tile has playable text.
     *
     * @param expectedPlayableText The expected playable text
     * @return true if current download tile has the expected playable text
     * value, false otherwise
     */
    public boolean verifyHasPlayableText(String expectedPlayableText) {
        String playableText = waitForElementVisible(playableTextLocator).getText();
        return playableText.toLowerCase().contains(expectedPlayableText.toLowerCase());
    }

    /**
     * Verify if currently active game download tile has the 'Playable' tip
     * indicator.
     *
     * @return true if the indicator is present, false otherwise
     */
    public boolean verifyTileHasPlayableTipIndicator() {
        return waitIsElementPresent(playableTipLocator);
    }

    /**
     * Verify the background color of the playable indicator for an active queue
     * game tile is "#04bd68" - Green color.
     *
     * @return true if background color is green, false otherwise
     */
    public boolean isPlayableIndicatorGreen() {
        return convertRGBAtoHexColor(waitForElementVisible(playableTipLocator).getCssValue("background-color")).equalsIgnoreCase(OriginClientData.PLAYABLE_INDICATOR_ACTIVE_COLOUR);
    }

    /**
     * Get download phase text of the game in progress.
     *
     * @return current download phase text of the game
     * in progress.
     */
    public String getDownloadPhaseText() {
        return waitForElementVisible(currentPhaseLocator).getText();
    }

    /**
     * Get ETA text of the game in progress.
     *
     * @return current ETA text of the game
     * in progress.
     */
    public String getETAText() {
        return waitForElementVisible(currentETALocator).getText();
    }

    /**
     * Check if the entitlement is paused.
     *
     * @return true if the current download has been paused, false otherwise
     */
    public boolean isPaused() {
        return getDownloadPhaseText().equalsIgnoreCase("Paused");
    }

    /**
     * Check if the entitlement is queued.
     *
     * @return true if the entitlement is queued by checking offerID, false otherwise
     */
    public boolean verifyIsInQueue() {
        return waitIsElementVisible(queueTileLocator, 2);
    }

    /**
     * Get StatusType base on the status attribute of the 'Download Queue' tile.
     *
     * @return StatusType of the 'Download Queue' tile
     */
    private StatusType getStatusType() {
        return StatusType.getStatusType(waitForElementPresent(queueTileLocator).getAttribute("status"));
    }

    /**
     * Check if the game is queued for download.
     *
     * @return true if game is queued for download, false otherwise
     */
    public boolean isGameQueued() {
        return getStatusType().equals(StatusType.QUEUED);
    }

    /**
     * Verify if game is actively downloading.
     *
     * @return true if game is actively downloading, false otherwise
     */
    public boolean isGameDownloading() {
        return getStatusType().equals(StatusType.ACTIVE);
    }

    /**
     * Verify if game download has been completed.
     *
     * @return true if game download has been completed, false otherwise
     */
    public boolean isGameDownloadComplete() {
        return getStatusType().equals(StatusType.COMPLETE);
    }

    /**
     * Waits for Game/DLC download to finish
     *
     * @return true is game download finishes successfully
     */
    public boolean waitForGameDownloadToComplete() {
        return Waits.pollingWait(() -> isGameDownloadComplete(), 120000, 1000, 0);
    }

    /**
     * Verify there are some download information in mini queue.
     *
     * @return true if current download has percentage, download speed, time
     * remaining and downloaded/total size, false otherwise.
     */
    public boolean verifyInformationinMiniQueue() {
        return getDownloadPercentage() != null
                && waitForElementVisible(downloadSpeedLocator).getText() != null
                && getDownloadPhaseText() != null
                && waitForElementVisible(downloadedAndTotalSizeLocator).getText() != null;
    }

    /**
     * Verify there is artwork for current downloading entitlement.
     *
     * @return true if there is artwork, false otherwise.
     */
    public boolean hasBackgroundArt() {
        WebElement element = waitForElementPresent(backgroundArtLocator);
        return element != null && element.getAttribute("src") != null && element.getAttribute("src").contains("http");
    }

    /**
     * Verify there is boxart for entitlement which is in queue.
     *
     * @return true if there is boxart, false otherwise.
     */
    public boolean verifyPackart() {
        WebElement element = waitForElementPresent(packartLocator);
        return element != null && element.getAttribute("src") != null && element.getAttribute("src").contains("http");
    }

    /**
     * Pauses download for this tile.
     */
    public void clickPauseButton() {
        waitForElementClickable(pauseButtonLocator).click();
    }

    /**
     * Cancels a download in progress or clears a completed download.
     */
    public void clickCancelClearButton() {
        scrollToElement(waitForElementVisible(cancelClearButtonLocator));
        waitForElementClickable(cancelClearButtonLocator).click();
        Waits.pollingWait(() -> !verifyIsInQueue());
    }

    /**
     * Click the 'Download' button on this tile.
     */
    public void clickDownloadButton() {
        waitForElementClickable(downloadButtonLocator).click();
    }

    /**
     * Click the 'Play' button on the currently downloading tile in
     * the 'Download Queue' flyout.
     */
    public void clickPlayButton() {
        waitForElementClickable(playButtonLocator).click();
    }

    /**
     * Verify there is a downloaded percentage info for entitlement which is currently queued.
     *
     * @return true if there is a percentage info other than 0%, false otherwise.
     */
    public boolean verifyPercentageProgressDownloadedWhenQueued() {
        return !waitForElementVisible(queuedPercentageProgress).getText().equals("0%");
    }
    
    /**
     * Verify the progress bar for an active queue is visible
     *
     * @return true if progress bar is visible, false otherwise
     */
    public boolean isProgressBarVisible() {
        return waitIsElementVisible(progressBarValueLocator);
    }
    
    /**
     * Verify the background color of the progress bar for an active queue
     * game tile is "#04bd68" - Green color.
     *
     * @return true if background color is green, false otherwise
     */
    public boolean isProgressBarGreen() {
         return convertRGBAtoHexColor(waitForElementVisible(progressBarValueLocator).getCssValue("background-color")).equalsIgnoreCase(OriginClientData.PROGRESS_BAR_DOWNLOADING_COLOUR);
    }
}