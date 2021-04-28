package com.ea.originx.automation.lib.pageobjects.gamelibrary;

import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the 'Download Manager' (previously called the 'Global Progress
 * Side Bar').
 *
 * @author palui
 */
public class DownloadManager extends EAXVxSiteTemplate {

    // Download Manager header, with 'Download Manager' title and Complete count
    protected static final String DOWNLOAD_MANAGER_CSS = "origin-download-manager-mini";
    protected static final String DOWNLOAD_MANAGER_WRAPPER_CSS = DOWNLOAD_MANAGER_CSS + " .origin-download-mini-wrapper";
    protected static final String DOWNLOAD_MANAGER_HEADER_CSS = DOWNLOAD_MANAGER_WRAPPER_CSS + " .origin-download-mini-header";
    protected static final String DOWNLOAD_MANAGER_TITLE_CSS = DOWNLOAD_MANAGER_HEADER_CSS + " .otktitle-4";
    protected static final By DOWNLOAD_MANAGER_TITLE_LOCATOR = By.cssSelector(DOWNLOAD_MANAGER_TITLE_CSS);
    protected static final String DOWNLOAD_MANAGER_COMPLETE_COUNT_CSS = DOWNLOAD_MANAGER_HEADER_CSS + " .otkbadge.origin-download-mini-badge";
    protected static final By DOWNLOAD_MANAGER_COMPLETE_COUNT_LOCATOR = By.cssSelector(DOWNLOAD_MANAGER_COMPLETE_COUNT_CSS);

    // Game in progress
    protected static final String GAME_CSS = DOWNLOAD_MANAGER_CSS + " origin-download-manager-game";
    protected static final String GAME_CSS_TMPL = GAME_CSS + "[offerid='%s']";
    protected static final By GAME_LOCATOR = By.cssSelector(GAME_CSS);
    protected static final String GAME_PROGRESS_CSS = GAME_CSS + " .otkprogress.origin-download-mini-progress";
    protected static final By GAME_TITLE_LOCATOR = By.cssSelector(GAME_PROGRESS_CSS + " .origin-download-mini-title");

    protected static final String PLAYABLE_CSS = GAME_PROGRESS_CSS + " .origin-download-playable";
    protected static final By PLAYABLE_NOW_LOCATOR = By.cssSelector(PLAYABLE_CSS + ".origin-download-playablenow");
    protected static final By PLAYABLE_TEXT_LOCATOR = By.cssSelector(PLAYABLE_CSS + " .origin-download-playabletext");
    protected static final By PLAYABLE_NOW_TEXT_LOCATOR = By.cssSelector(PLAYABLE_CSS + ".origin-download-playablenow .origin-download-playabletext");

    protected static final String DOWNLOAD_MINI_WRAP = GAME_PROGRESS_CSS + " .origin-download-mini-wrap";
    protected static final By DOWNLOAD_MINI_LOCATOR = By.cssSelector(DOWNLOAD_MINI_WRAP);
    protected static final By PROGRESS_VALUE_LOCATOR = By.cssSelector(DOWNLOAD_MINI_WRAP + " .otkprogress-value");
    protected static final By PHASE_LOCATOR = By.cssSelector(DOWNLOAD_MINI_WRAP + " .origin-download-mini-phase");
    protected static final By ETA_LOCATOR = By.cssSelector(DOWNLOAD_MINI_WRAP + " .origin-download-mini-time");
    protected static final By RIGHT_ARROW_LOCATOR = By.cssSelector(DOWNLOAD_MINI_WRAP + " .origin-download-mini-rightarrow");

    protected static final String EXPECTED_PLAYABLE_TIP_TEXT = "Playable";

    //OIG paths
    protected static final By OIG_COMPLETED_OFFERS_LOCATOR = By.cssSelector(".origin-download-completed > origin-download-manager-game");
    protected static final By CLOSE_BUTTON_LOCATOR = By.xpath("//*[@id='buttonContainer']/*[@id='btnClose']");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public DownloadManager(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify if 'Download Manager' title is visible. This is visible when there is
     * one or more completed games.
     *
     * @return true if visible, false otherwise
     */
    public boolean isDownloadManagerTitleVisible() {
        return waitIsElementVisible(DOWNLOAD_MANAGER_TITLE_LOCATOR, 2);
    }

    /**
     * Verify if the game title is visible. This is visible when there is a
     * download in progress.
     *
     * @return true if visible, false otherwise
     */
    public boolean isGameTitleVisible() {
        return waitIsElementVisible(GAME_TITLE_LOCATOR, 2);
    }

    /**
     * Verify if 'Download Manager' is visible.
     *
     * @return true if either the 'Download Manager' title or game title is
     * visible, false otherwise
     */
    public boolean isDownloadManagerVisible() {
        return isDownloadManagerTitleVisible() || isGameTitleVisible();
    }

    /**
     * Get in-progress game title.
     *
     * @return In-progress game title, or throw TimeoutException if not found
     */
    public String getInProgressGameTitle() {
        return waitForElementVisible(GAME_TITLE_LOCATOR).getText();
    }

    /**
     * Get in-progress game offer ID
     *
     * @return In-progress game offer ID, or throw TimeoutException if not found
     */
    public String getInProgressGameOfferId() {
        return waitForElementVisible(GAME_LOCATOR).getAttribute("offerid");
    }

    /**
     * Verify if game in progress matches the expected game.
     *
     * @param game Game to check if its download in progress
     * @return true if game if in progress, false otherwise. Throw
     * TimeoutException if no game in progress found
     */
    public boolean verifyInProgressGame(EntitlementInfo game) {
        return getInProgressGameTitle().equals(game.getName())
                && getInProgressGameOfferId().equals(game.getOfferId());
    }

    /**
     * Verify if the playable tip text is 'Playable'.
     *
     * @return true if playable tip text matches, false otherwise
     */
    public boolean verifyPlayableTipText() {
        return waitForElementVisible(PLAYABLE_TEXT_LOCATOR).getText().equalsIgnoreCase(EXPECTED_PLAYABLE_TIP_TEXT);
    }

    /**
     * Verify if current downloading game is in 'Playable Now' state.
     *
     * @return true if in 'Playable Now' state, false otherwise
     */
    public boolean verifyCurrentDownloadIsPlayableNow() {
        return isElementVisible(PLAYABLE_NOW_LOCATOR);
    }

    /**
     * Get current progress percentage.
     *
     * @return Current progress percentage
     */
    public int getProgressBarValue() {
        return getProgressBarValue(PROGRESS_VALUE_LOCATOR);
    }

    /**
     * Get download phase text (includes time remaining) of the game in progress.
     *
     * @return Current download phase text (includes time remaining) of the game
     * in progress
     */
    public String getDownloadPhaseText() {
        return waitForElementVisible(PHASE_LOCATOR).getText();
    }

    /**
     * Get ETA text of the game in progress
     *
     * @return ETA text of the game
     * in progress
     */
    public String getETAText() {
        return waitForElementVisible(ETA_LOCATOR).getText();
    }

    /**
     * Verify if game is paused.
     *
     * @return true if game phase text is "Paused", false otherwise
     */
    public boolean verifyGameIsPaused() {
        return getDownloadPhaseText().equals("Paused");
    }

    /**
     * Verify download completed count matches the expected count.
     *
     * @param expectedEntitlementCount The expected count
     *
     * @return true if it matches the expected game count, false otherwise
     */
    public boolean verifyDownloadCompleteCountEquals(String expectedEntitlementCount) {
        return isElementVisible(DOWNLOAD_MANAGER_TITLE_LOCATOR)
                && waitForElementVisible(DOWNLOAD_MANAGER_COMPLETE_COUNT_LOCATOR).getText().equals(expectedEntitlementCount);
    }

    /**
     * Click download manager title to open the 'Download Queue' flyout. The title
     * is visible if at least one game has completed download.
     */
    public void clickDownloadManagerTitleToOpenFlyout() {
        waitForElementClickable(DOWNLOAD_MANAGER_TITLE_LOCATOR).click();
    }

    /**
     * Click download manager complete count to open the 'Download Queue' flyout.
     * The count is visible if at least one game has completed download.
     */
    public void clickDownloadCompleteCountToOpenFlyout() {
        waitForElementClickable(DOWNLOAD_MANAGER_COMPLETE_COUNT_LOCATOR).click();
    }

    /**
     * Click game title to open the 'Download Queue' flyout. The game is visible
     * when there is a game in progress.
     */
    public void clickGameTitleToOpenFlyout() {
        waitForElementClickable(GAME_TITLE_LOCATOR).click();
    }

    /**
     * Click right-arrow to open the 'Download Queue' flyout. The arrow is visible
     * when there is a game in progress.
     */
    public void clickRightArrowToOpenFlyout() {
        waitForElementClickable(RIGHT_ARROW_LOCATOR).click();
    }

    /**
     * Click the mini download manager to open the 'Download Queue' flyout.
     */
    public void clickMiniDownloadManagerToOpenFlyout() {
        waitForElementClickable(DOWNLOAD_MINI_LOCATOR).click();
    }

    /**
     * 'Download Queue' fly out can be closed by events such as pausing a download.
     * Open the Flyout if required.<br>
     * NOTE: Here we try all possible locators that may be visible<br>
     * (1) If there are completed games, manager title and complete count are
     * visible.<br>
     * (2) If there is a game in progress, game title and right arrow are
     * visible.<br>
     * Scripts should try to use the specific ones defined above if it knows
     * which scenario it is currently in.
     */
    public void openDownloadQueueFlyout() {
        if (!new DownloadQueue(driver).isOpen()) {
            waitForAnyElementVisible(DOWNLOAD_MANAGER_TITLE_LOCATOR, GAME_TITLE_LOCATOR,
                    RIGHT_ARROW_LOCATOR, DOWNLOAD_MANAGER_COMPLETE_COUNT_LOCATOR).click();
        }
    }

    /**
     * 'Download Queue' flyout can interfere with 'Game Library' operations. Close
     * the flyout (by clicking 'Global Search') if required.<br>
     * NOTE: Cannot close the flyout by clicking on the download manager area -
     * only clicking on white space or another element (preferably one that
     * doesn't change what page you are on). Here, we click 'Global Search"
     * since it is on every page (than white spaces) and does not change the
     * page when clicked on.
     */
    public void closeDownloadQueueFlyout() {
        if (new DownloadQueue(driver).isOpen()) {
            new GlobalSearch(driver).clickOnGlobalSearch();
        }
    }

    /**
     * Check the OIG download manager to see if a specific offer is listed as
     * completed.
     *
     * @param offerId Offer ID of the offer to be found
     * @return true if given offer is found as having completed downloading,
     * false otherwise
     */
    public boolean verifyOfferHasCompletedDownloadOIG(String offerId) {
        isDownloadManagerVisible();
        List<WebElement> completedOffers = driver.findElements(OIG_COMPLETED_OFFERS_LOCATOR);
        for (WebElement completedOffer : completedOffers) {
            if (offerId.equalsIgnoreCase(completedOffer.getAttribute("offerid"))) {
                return true;
            }
        }
        return false;
    }
}
