package com.ea.originx.automation.lib.pageobjects.originaccess;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.dialog.NewSaveDataFoundDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.TrialPlayNowDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.store.TrialTile;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.pageobjects.youtube.YouTubeComponent;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.util.List;
import java.util.stream.Collectors;

/**
 * Represents the 'Play First Trials' page.
 *
 * @author nvarthakavi
 */
public class PlayFirstTrialsPage extends EAXVxSiteTemplate {

    private static final By TITLE_LOCATOR = By.cssSelector(".origin-store-access-trials-hero-text h1");
    private static final String[] TITLE_TEXTS = {"Play", "first"};
    private static final String GAME_TRIAL_TILE_CSS = ".origin-store-freegames-program .origin-store-program-offer[offer-description-title='Available now']";
    private static final By GAME_TRIAL_TILES_LOCATOR = By.cssSelector(GAME_TRIAL_TILE_CSS);
    protected static final String FREE_GAME_TRIAL_TILE_CSS_TMPL = GAME_TRIAL_TILE_CSS + "[offer-id='%s']";

    // Hero
    private static final String HERO_CSS = "origin-store-access-trials-hero";
    private static final By HERO_LOGO_IMG_LOCATOR = By.cssSelector(HERO_CSS + " img.origin-store-access-trials-hero-logo");
    private static final By HERO_TITLE_LOCATOR = By.cssSelector(HERO_CSS + " .otktitle-page");
    private static final By HERO_ITEM_LOCATOR = By.cssSelector(HERO_CSS + " .origin-store-access-item");
    private static final By HERO_PLATFORM_LOCATOR = By.cssSelector(HERO_CSS + " .origin-store-access-trials-hero-platform");
    private static final By HERO_BACKGROUND_IMG_LOCATOR = By.cssSelector(HERO_CSS + " img.origin-store-access-trials-hero-background");

    //Footer
    private static final String FOOTER_CSS = "origin-store-access-trials-footer";
    private static final By FOOTER_LOGO_IMG_LOCATOR = By.cssSelector(FOOTER_CSS + " img.origin-store-access-trials-footer-logo");
    private static final By FOOTER_TITLE_LOCATOR = By.cssSelector(FOOTER_CSS + " .otktitle-page");
    private static final By FOOTER_SUBTITLE_LOCATOR = By.cssSelector(FOOTER_CSS + " .origin-store-access-trials-footer-description");
    private static final By FOOTER_ITEM_LOCATOR = By.cssSelector(FOOTER_CSS + " .origin-store-access-item");
    private static final By FOOTER_PLATFORM_LOCATOR = By.cssSelector(FOOTER_CSS + " .origin-store-access-trials-footer-platform");
    private static final By FOOTER_LINK_LOCATOR = By.cssSelector(FOOTER_CSS + " .origin-store-access-trials-footer-link a");
    private static final By FOOTER_PLAYER_LOCATOR = By.cssSelector(FOOTER_CSS + " .origin-store-access-trials-footer-player-wrapper");
    private static final String[] FOOTER_LINK_LIST = {"Learn", "more", "Origin Access"};
    private static final By MEDIA_PROP_LOCATOR = By.cssSelector(".origin-store-access-trials-mediaprop");
    private static final By MEDIA_PROP_TITLE_LOCATOR = By.cssSelector(".origin-store-access-trials-mediaprop-content.origin-store-access-trials-mediaprop-content-light .origin-store-access-trials-mediaprop-mainheader");
    private static final By MEDIA_PROP_MEDIA_MODULE_LOCATOR = By.cssSelector(".origin-store-access-trials-mediaprop origin-store-media-carousel");
    private static final By MEDIA_PROP_DESCRIPTOR_TEXT_LOCATOR  = By.cssSelector(".origin-store-access-trials-mediaprop-content.origin-store-access-trials-mediaprop-content-light .origin-store-access-trials-mediaprop-text");

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public PlayFirstTrialsPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'Play First Trials' page has been reached.
     *
     * @return true if the page is displayed, false otherwise
     */
    public boolean verifyPlayFirstTrialPageReached() {
        return StringHelper.containsAnyIgnoreCase(waitForElementVisible(TITLE_LOCATOR).getText(), TITLE_TEXTS);
    }

    /**
     * Get all the 'Play First Trials' tile WebElements.
     *
     * @return List of all the 'Play First Trials' tile WebElements on the 'Play First
     * Trials' page
     */
    public List<WebElement> getAllPlayFirstTrialTileElements() {
        return driver.findElements(GAME_TRIAL_TILES_LOCATOR);
    }

    /**
     * Get all the 'Play First Trial' TrialTile elements.
     *
     * @return List of all 'Play First Trials' tiles on the 'Play First Trials'
     * page
     */
    public List<TrialTile> getAllAllPlayFirstTrialTiles() {
        // Wait for the first tile to load completely
        if (!Waits.pollingWait(() -> verifyFirstPlayFirstTileLoaded())) {
            throw new RuntimeException("Failed to load the first Play First tile");
        }
        List<TrialTile> playFirstTrialTiles = getAllPlayFirstTrialTileElements().
                stream().map(webElement -> new TrialTile(driver, webElement)).collect(Collectors.toList());
        return playFirstTrialTiles;
    }

    /**
     * Returns the first 'Get It Now' trial tile.
     *
     * @return The first 'Get It Now' trial tile if there is one, otherwise return null
     */
    public TrialTile getFirstGetItNowTrialTile() {
        return getAllAllPlayFirstTrialTiles().get(0);
    }

    /**
     * Get a specific 'Play First Trials' tile WebElement given an offer ID.
     *
     * @param offerId Entitlement offer ID
     * @return 'Play First Trials' tile WebElement for the given offer ID on the
     * 'Play First Trials' page tab, or throw NoSuchElementException
     */
    private WebElement getPlayFirstTrialTileElement(String offerId) {
        waitForElementPresent(By.cssSelector(String.format(FREE_GAME_TRIAL_TILE_CSS_TMPL, offerId)));
        scrollToElement(waitForElementVisible(By.cssSelector(String.format(FREE_GAME_TRIAL_TILE_CSS_TMPL, offerId)))); //added this scroll as the element is hidden
        return driver.findElement(By.cssSelector(String.format(FREE_GAME_TRIAL_TILE_CSS_TMPL, offerId)));
    }

    /**
     * Get a specific 'Play First Trials' tile element given an offer ID.
     *
     * @param offerId Entitlement offer ID
     * @return PlayFirstTrialTile for the given offer ID on the 'Play First
     * Trials' page, or throw NoSuchElementException
     */
    public TrialTile getPlayFirstTrialTile(String offerId) {
        return new TrialTile(driver, getPlayFirstTrialTileElement(offerId));
    }

    /**
     * Add the given 'Play First Trials' tile to the library by clicking the
     * corresponding 'Get It Now' button and verify checkout confirmation was visible.
     *
     * @param offerId The offer ID of the entitlement to be selected to add to
     * the 'Game Library'
     * @return true if checkout confirmation dialog is visible, false otherwise.
     */
    public boolean addTrialGameToLibrary(String offerId) {
        TrialTile playFirstTrialTile = getPlayFirstTrialTile(offerId);
        playFirstTrialTile.clickGetItNowButton();
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        checkoutConfirmation.waitForVisible();
        boolean isVisible = checkoutConfirmation.isDialogVisible();
        checkoutConfirmation.clickCloseCircle();
        return isVisible;
    }

    /**
     * To launch a trial and verify the trial has launched. Assumes the 'Game
     * Library' page has been opened.
     *
     * @param trialOfferId Offer ID of the trial game
     * @param trialName Name of the trial game (Assumes the non-trial
     * entitlement is already added to the EntitlementInfo)
     * @return true if the trial game is launched, false otherwise
     */
    public boolean launchTrial(String trialOfferId, String trialName, String processName) {
        try {
            new DownloadManager(driver).closeDownloadQueueFlyout();
            GameTile gameTile = new GameTile(driver, trialOfferId);
            gameTile.waitForReadyToPlay();
            gameTile.startTrial();
            TrialPlayNowDialog trialPlayNowDialog = new TrialPlayNowDialog(driver, trialName);
            if (trialPlayNowDialog.waitIsVisible()) {
                trialPlayNowDialog.clickPlayNowButton();
            }
            sleep(10000); //sleep for one sec to launch the trial
            if (new NewSaveDataFoundDialog(driver).waitIsVisible()) {
                new NewSaveDataFoundDialog(driver).clickDeleteLocalDataButton();
            }

            if (!ProcessUtil.isEntitlementLaunched(client, processName)) {
                _log.debug("Error: Could not launch the entitlement" + trialName);
                return false;
            }

            return true;
        } catch (IOException | InterruptedException ex) {
            return false;
        }
    }

    /**
     * Wait for the the 'Play First Trials' tile to load
     * with CTA containing the text 'Add to Game Library'
     */
    public boolean verifyFirstPlayFirstTileLoaded() {
        WebElement firstPlayFirstTrial;
        TrialTile firstPlayFirstTrialTile;
        try {
            firstPlayFirstTrial = waitForElementVisible(GAME_TRIAL_TILES_LOCATOR, 3);
            firstPlayFirstTrialTile = new TrialTile(driver, firstPlayFirstTrial);
        } catch (TimeoutException e) {
            _log.debug("No tiles loaded");
            return false;
        }
        // Ensure that the offer-id attribute is loaded and the CTA has the correct text
        boolean isOfferIdNotEmptyAndNotNull = !StringHelper.nullOrEmpty(firstPlayFirstTrial.getAttribute("offer-id"));
        boolean isGetItNowCTAPresent = firstPlayFirstTrialTile.verifyAddToGameLibraryCTAExists();
        return isOfferIdNotEmptyAndNotNull && isGetItNowCTAPresent;
    }

    /**
     * Verifies if hero 'Origin Access' logo is displayed or not
     *
     * @return true if hero Origin Access logo is displayed, false otherwise
     */
    public boolean verifyHeroOriginAccessLogoIsVisible(){
        return waitIsElementVisible(HERO_LOGO_IMG_LOCATOR);
    }

    /**
     * Verifies if hero title is displayed and non-empty
     *
     * @return true if hero title is displayed and non-empty, false otherwise
     */
    public boolean verifyHeroTitle(){
        WebElement title = waitForElementPresent(HERO_TITLE_LOCATOR);
        return title.isDisplayed() && !title.getText().isEmpty();
    }

    /**
     * Verifies if hero  bullet points are displayed and non-empty
     *
     * @return true if hero  bullet points are displayed and non-empty, false otherwise
     */
    public boolean verifyHeroBulletPoints(){
        boolean verified = true;
        for(WebElement bulletPoint : waitForAllElementsVisible(HERO_ITEM_LOCATOR)){
            verified &= bulletPoint.isDisplayed() && !bulletPoint.getText().isEmpty();
        }
        return verified;
    }

    /**
     * Verifies if hero platform is displayed and equals to "ONLY FOR PC"
     *
     * @return true if hero platform is displayed and equals to "ONLY FOR PC", false otherwise
     */
    public boolean verifyHeroPlatform(){
        WebElement platform = waitForElementPresent(HERO_PLATFORM_LOCATOR);
        return platform.isDisplayed() && platform.getText().equals("ONLY FOR PC");
    }

    /**
     * Verifies if hero background image is displayed
     *
     * @return true if hero background image is displayed, false otherwise
     */
    public boolean verifyHeroBackgroundImageIsDisplayed(){
        WebElement background = waitForElementPresent(HERO_BACKGROUND_IMG_LOCATOR);
        return background.isDisplayed();
    }

    /**
     * Verifies if footer 'Origin Access' logo is displayed or not
     *
     * @return true if footer Origin Access logo is displayed, false otherwise
     */
    public boolean verifyFooterOriginAccessLogoIsVisible(){
        return waitIsElementVisible(FOOTER_LOGO_IMG_LOCATOR);
    }

    /**
     * Verifies if footer title is displayed and non-empty
     *
     * @return true if footer title is displayed and non-empty, false otherwise
     */
    public boolean verifyFooterTitle(){
        WebElement title = waitForElementPresent(FOOTER_TITLE_LOCATOR);
        return title.isDisplayed() && !title.getText().isEmpty();
    }

    /**
     * Verifies if footer subtitle is displayed and non-empty
     *
     * @return true if footer subtitle is displayed and non-empty, false otherwise
     */
    public boolean verifyFooterSubtitle(){
        WebElement title = waitForElementPresent(FOOTER_SUBTITLE_LOCATOR);
        return title.isDisplayed() && !title.getText().isEmpty();
    }

    /**
     * Verifies if footer bullet points are displayed and non-empty
     *
     * @return true if footer bullet points are displayed and non-empty, false otherwise
     */
    public boolean verifyFooterBulletPoints(){
        boolean verified = true;
        for(WebElement bulletPoint : waitForAllElementsVisible(FOOTER_ITEM_LOCATOR)){
            verified &= bulletPoint.isDisplayed() && !bulletPoint.getText().isEmpty();
        }
        return verified;
    }

    /**
     * Verifies if footer platform is displayed and equals to "ONLY FOR PC"
     *
     * @return true if footer platform is displayed and equals to "ONLY FOR PC", false otherwise
     */
    public boolean verifyFooterPlatform(){
        WebElement platform = waitForElementPresent(FOOTER_PLATFORM_LOCATOR);
        return platform.isDisplayed() && platform.getText().equals("ONLY FOR PC");
    }

    /**
     * Verifies if footer link is displayed and has the words 'Learn', 'more' & 'Origin Access'
     *
     * @return true if footer link is displayed and has the words 'Learn', 'more' & 'Origin Access', false otherwise
     */
    public boolean verifyFooterLink(){
        WebElement link = waitForElementPresent(FOOTER_LINK_LOCATOR);
        return link.isDisplayed() && StringHelper.containsAnyIgnoreCase(link.getText(), FOOTER_LINK_LIST);
    }

    /**
     * Clicks on the footer link
     *
     * @return OriginAccessPage
     */
    public OriginAccessPage clickFooterLink(){
        WebElement link = waitForElementPresent(FOOTER_LINK_LOCATOR);
        scrollElementToCentre(link);
        link.click();
        return new OriginAccessPage(driver);
    }

    /**
     * Returns the YouTube Component
     *
     * @return YouTube Component
     */
    public YouTubeComponent getFooterYouTubeComponent(){
        return new YouTubeComponent(driver, FOOTER_PLAYER_LOCATOR);
    }
    //////////////////////////////////////////////
    // Media Prop
    //////////////////////////////////////////////

    /**
     * Verifies that the Media Prop component is visible
     *
     * @return true if the component is visible, false otherwise
     */
    public boolean verifyMediaPropVisible(){
        return waitIsElementVisible(MEDIA_PROP_LOCATOR);
    }

    /**
     * Verifies if the Media Prop title is not empty
     *
     * @return true if the Media Prop title is visible, false otherwise
     */
    public boolean verifyMediaPropTitleVisible(){
        return !waitForElementVisible(MEDIA_PROP_TITLE_LOCATOR).getText().isEmpty();
    }

    /**
     * Verifies that the carousel within the Media Prop component is visible
     *
     * @return true if the carousel is visible, false otherwise
     */
    public boolean verifyMediaPropMediaCarouselVisible(){
        return waitIsElementVisible(MEDIA_PROP_MEDIA_MODULE_LOCATOR);
    }

    /**
     * Verifies that the descriptor text within the Media Prop component is not empty
     *
     * @return true if the descriptor text is visible, false otherwise
     */
    public boolean verifyMediaPropDescriptorTextVisible(){
        return !waitForElementVisible(MEDIA_PROP_DESCRIPTOR_TEXT_LOCATOR).getText().isEmpty();
    }

}