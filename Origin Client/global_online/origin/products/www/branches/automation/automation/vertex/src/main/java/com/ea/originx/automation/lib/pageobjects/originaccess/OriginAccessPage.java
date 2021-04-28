package com.ea.originx.automation.lib.pageobjects.originaccess;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.net.helpers.RestfulHelper;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * A page object that represents the 'Origin Access' page.
 *
 * @author glivingstone
 */
public class OriginAccessPage extends EAXVxSiteTemplate {

    protected static final By ORIGIN_ACCESS_PAGE_LOCATOR = By.cssSelector("[origin-automation-id='access-landing-hero']");
    protected static final By ORIGIN_PREMIER_HERO_LOCATOR = By.cssSelector("origin-store-premier-landing-razorhero");
    protected static final String HEADER_JOIN_BUTTON_TMPL = "[origin-automation-id='premier-landing-razorhero-cta'] button";
    protected static final String FOOTER_JOIN_BUTTON_TMPL = "[origin-automation-id='access-footer-join-now-cta'] button";
    protected static final By HEADER_JOIN_BUTTON_LOCATOR = By.cssSelector(HEADER_JOIN_BUTTON_TMPL);
    protected static final By HEADER_PRICE_LOCATOR = By.cssSelector(".origin-store-access-landing-hero-container h4");

    // Navigation bar tabs
    protected static final String NAVIGATOR_BAR_CSS = ".store-pdp-nav-bar";
    protected static final By NAVIGATOR_BAR_LOCATOR = By.cssSelector(NAVIGATOR_BAR_CSS);
    protected static final String NAVIGATOR_BAR_TABS_TMPL = NAVIGATOR_BAR_CSS + " .otkpill-light a[data-telemetry-label='%s']";
    protected static final By NAVIGATOR_BAR_COMPARE_PLAN_TAB_LOCATOR = By.cssSelector(String.format(NAVIGATOR_BAR_TABS_TMPL, "compare"));
    protected static final By NAVIGATOR_BAR_GAMES_IN_EA_ACCESS_TAB_LOCATOR = By.cssSelector(String.format(NAVIGATOR_BAR_TABS_TMPL, "includedinpremier"));
    protected static final By NAVIGATOR_BAR_HOW_UPGRADE_WORKS_TAB_LOCATOR = By.cssSelector(String.format(NAVIGATOR_BAR_TABS_TMPL, "benefitsandfaq"));
    protected static final By NAVIGATOR_BAR_SELECTION_LINE = By.cssSelector(NAVIGATOR_BAR_CSS + " .otknav-pills-bar");
    protected static final By NAVIGATOR_BAR_COMPARISON_TABLE_CTA = By.cssSelector(NAVIGATOR_BAR_CSS + " .store-pdp-nav-elements");
    protected static final By NAVIGATION_BAR_JOIN_PREMIER_CTA = By.cssSelector(NAVIGATOR_BAR_CSS + " button");
     
    // Benefits and FAQ/Legal/Play First Section
    protected static final String PLAY_FIRST_CSS = ".origin-store-access-landing-prop";
    protected static final By PLAY_FIRST_SECTION_LOCATOR = By.cssSelector(PLAY_FIRST_CSS);
    protected static final By PLAY_FIRST_TITLE_LOCATOR = By.cssSelector(PLAY_FIRST_CSS + " h2");
    protected static final By PLAY_FIRST_DESCRIPTION_LOCATOR = By.cssSelector(PLAY_FIRST_CSS + " .origin-store-access-item");
    protected static final By PLAY_FIRST_BACKGROUND_IMG_LOCATOR = By.cssSelector(PLAY_FIRST_CSS + " .origin-store-access-landing-prop-background img");
    protected static final By PLAY_FRIST_FOREGROUND_IMG_LOCATOR = By.cssSelector(PLAY_FIRST_CSS + " .origin-store-access-landing-prop-container div img");
    protected static final By PLAY_FRIST_GAME_LOGO_LOCATOR = By.cssSelector(PLAY_FIRST_CSS + " .origin-store-access-landing-prop-logo img");
    protected static final By PLAY_FIRST_GAME_AVAILABILITY_LOCATOR = By.cssSelector(PLAY_FIRST_CSS + " .origin-store-access-landing-prop-container h6");
    protected static final String[] PLAY_FIRST_GAME_AVAILABILITY_KEYWORDS = {"Available"};
    protected static final String[] PLAY_FIRST_TITLE_KEYWORDS = {"play", "first", "access"};
    protected static final By FAQ_SECTION_LOCATOR = By.cssSelector("origin-store-premier-landing-section-wrapper[header-title='FAQ'] .origin-store-premier-landing-section-wrapper");
    protected static final By LEGAL_SECTION_LOCATOR = By.cssSelector("origin-store-access-landing-legalwrapper section");
    
    // Games in EA Access Section
    protected static final String GAMES_IN_EA_ACCESS_CSS = ".origin-store-access-landing-trialwrapper";
    protected static final By GAMES_IN_EA_ACCESS_SECTION_TITLE_LOCATOR = By.cssSelector(GAMES_IN_EA_ACCESS_CSS + " h3");
    protected static final By GAMES_IN_EA_ACCESS_ENTITLEMENT_TILE_LOCATOR = By.cssSelector(GAMES_IN_EA_ACCESS_CSS + " origin-store-access-landing-trialtile");
    protected static final By GAMES_IN_EA_ACCESS_ENTITLEMENT_TILE_VIOLATOR_LOCATOR = By.cssSelector(" .origin-store-access-landing-trialtile-violator");
    protected static final By GAMES_IN_EA_ACCESS_ENTITLEMENT_TILE_TITLE_LOCATOR = By.cssSelector(" .origin-store-access-landing-trialtile-header");
    protected static final By GAMES_IN_EA_ACCESS_ENTITLEMENT_TILE_DESCRIPTION_LOCATOR = By.cssSelector(" .origin-store-access-landing-trialtile-description");
    protected static final String[] GAMES_IN_EA_ACCESS_TITLE_KEYWORDS = {"Origin", "Access", "Premier"};
    protected static final String[] GAMES_IN_EA_ACCESS_TILE_VIOLATOR_KEYWORDS = {"Coming Soon", "Available Now"};

    // How Upgrading Works Section
    protected static final String HOW_UPGRADING_WORKS_CSS = "origin-store-premier-landing-section-wrapper[anchor-name='upgrading-work'] .origin-pdp-feature-hero";
    protected static final By HOW_UPGRADING_WORKS_TITLE_LOCATOR = By.cssSelector(HOW_UPGRADING_WORKS_CSS + " h2");
    protected static final By HOW_UPGRADING_WORKS_DESCRIPTION = By.cssSelector(HOW_UPGRADING_WORKS_CSS + " p");
    protected static final String[] HOW_UPGRADING_WORKS_DESCRIPTION_KEYWORDS = {"refund", "membership", "premier", "faq"};

    // Compare Plan/Comparisson table Section
    protected static final String COMPARISON_TABLE_LOCATOR_CSS = ".origin-store-premier-comparison-table-container";
    protected static final String[] COMPARISON_TABLE_HEADER_PRICE_KEYWORDS = {"month", "year"};
    protected static final String COMPARISON_TABLE_HEADER_TIERS_TMPL = COMPARISON_TABLE_LOCATOR_CSS + " origin-store-premier-comparisontable-header[subscription-level='%s']";
    protected static final By COMPARISON_TABLE_LOCATOR = By.cssSelector(COMPARISON_TABLE_LOCATOR_CSS);
    protected static final By COMPARISON_TABLE_DESCRIPTION_ROWS = By.cssSelector(COMPARISON_TABLE_LOCATOR_CSS + " .origin-store-premier-comparison-item");
    protected static final By COMPARISON_TABLE_HEADER_BASIC_TIER_LOCATOR = By.cssSelector(String.format(COMPARISON_TABLE_HEADER_TIERS_TMPL, "standard"));
    protected static final By COMPARISON_TABLE_HEADER_PREMIER_TIER_LOCATOR = By.cssSelector(String.format(COMPARISON_TABLE_HEADER_TIERS_TMPL, "premium"));
    protected static final By COMPARISON_TABLE_HEADER_BASIC_PRICE_LOCATOR = By.cssSelector(COMPARISON_TABLE_HEADER_BASIC_TIER_LOCATOR + "  .origin-store-premier-comparison-column-top-subtitle");
    protected static final By COMPARISON_TABLE_HEADER_PREMIER_PRICE_LOCATOR = By.cssSelector(COMPARISON_TABLE_HEADER_PREMIER_TIER_LOCATOR + " .origin-store-premier-comparison-column-top-subtitle");
    protected static final By COMPARISON_TABLE_FOOTER_BASIC_BUY_BUTTON_LOCATOR = By.cssSelector(COMPARISON_TABLE_LOCATOR_CSS + "  .origin-cta-transparent button");
    protected static final By COMPARISON_TABLE_FOOTER_PREMIER_BUY_BUTTON_LOCATOR = By.cssSelector(COMPARISON_TABLE_LOCATOR_CSS + "  .origin-cta-primary button");

    // Premier/Basic Games Section
    protected static final String PREMIER_GAMES_SECTION_CSS = "origin-store-access-landing-trialwrapper[header*='Origin Access Premier'] section";
    protected static final String BASIC_AND_PREMIER_SECTION_CSS = "origin-store-premier-landing-vaultlist-wrapper[header-subtitle*='Basic or Premier'] section";
    protected static final By PREMIER_GAMES_SECTION_LOCATOR = By.cssSelector(PREMIER_GAMES_SECTION_CSS);
    protected static final By BASIC_AND_PREMIER_SECTION_LOCATOR = By.cssSelector(BASIC_AND_PREMIER_SECTION_CSS);
    protected static final By PREMIER_GAME_TILE_LOCATOR = By.cssSelector(PREMIER_GAMES_SECTION_CSS + " origin-store-access-landing-trialtile");
    protected static final By BASIC_AND_PREMIER_VAULT_TILES_LOCATOR = By.cssSelector(BASIC_AND_PREMIER_SECTION_CSS + " .origin-store-premier-landing-vaultlist-game");
    
    // Upgrading to Premier Section
    protected static final String UPGRADING_TO_PREMIER_CSS = "origin-store-pdp-feature-premier[header*='upgrading']";
    protected static final By UPGRADING_TO_PREMIER__SECTION_LOCATOR = By.cssSelector(UPGRADING_TO_PREMIER_CSS);
    
    // The CQ variables are to differentiate a trial or origin access normal subscription
    // They are the attribute values in the JOIN_BUTTON_TMPL - cq-targeting-subscribes
    protected static final String USED_TRIAL_CQ = "usedsubstrial";
    protected static final String AVAILABLE_TRIAL_CQ = "notusedsubstrial";
    protected static final String[] EXPECTED_JOIN_NOW_BUTTON_KEYWORDS = {"join", "origin", "access"};
    protected static final String[] EXPECTED_START_YOUR_TRIAL_BUTTON_KEYWORDS = {"free"};

    // Free trial Disclaimer
    protected static final By FREE_TRIAL_TEXT_LOCATOR = By.cssSelector(".origin-store-access-landing-hero-content-legal");
    protected static final String[] EXPECTED_FREE_TRIAL_TEXT_KEYWORDS = {"free", "trial", "cancel", "anytime"};

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OriginAccessPage(WebDriver driver) {
        super(driver);
    }

    
     /**
     * Verify the hero 'Join Premier' CTA exists in the page hero.
     *
     * @return true if the button is visible, false otherwise
     */
    public boolean verifyHeroJoinPremierButtonVisible() {
        return waitIsElementVisible(HEADER_JOIN_BUTTON_LOCATOR);
    }
    
    /**
     * Verify the free trial text under the 'Free Trial' CTA.
     *
     * @return true if the text matches, false otherwise
     */
    public boolean verifyFreeTrialTextExists() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(FREE_TRIAL_TEXT_LOCATOR).getText(), EXPECTED_FREE_TRIAL_TEXT_KEYWORDS);
    }

    /**
     * Verify the 'Free Trial' CTA exists in the page hero.
     *
     * @return true if the button exists, false otherwise
     */
    public boolean verifyFreeTrialHeroButtonExists() {
        return waitIsElementVisible(By.cssSelector(String.format(HEADER_JOIN_BUTTON_TMPL, AVAILABLE_TRIAL_CQ)));
    }

    /**
     * Verify the 'Join Access' CTA exists in the page hero.
     *
     * @return true if the button exists, false otherwise
     */
    public boolean verifyJoinAccessHeroButtonExists() {
        return waitIsElementVisible(By.cssSelector(String.format(HEADER_JOIN_BUTTON_TMPL, USED_TRIAL_CQ)));
    }

    /**
     * Verify the 'Free Trial' CTA exists in the page footer.
     *
     * @return true if the button exists, false otherwise
     */
    public boolean verifyFreeTrialFooterButtonExists() {
        return waitIsElementVisible(By.cssSelector(String.format(FOOTER_JOIN_BUTTON_TMPL, AVAILABLE_TRIAL_CQ)));
    }

    /**
     * Verify the 'Join Access' CTA exists in the page footer.
     *
     * @return true if the button exists, false otherwise
     */
    public boolean verifyJoinAccessFooterButtonExists() {
        return waitIsElementVisible(By.cssSelector(String.format(FOOTER_JOIN_BUTTON_TMPL, USED_TRIAL_CQ)));
    }

    /**
     * Click on the 'Join Basic Today' button on the main page to start the
     * checkout flow.
     */
    public void clickJoinBasicButton() {
        waitForPageToLoad();
        scrollElementToCentre(waitForElementVisible(COMPARISON_TABLE_FOOTER_BASIC_BUY_BUTTON_LOCATOR));
        waitForElementClickable(COMPARISON_TABLE_FOOTER_BASIC_BUY_BUTTON_LOCATOR).click();
    }
    
    /**
     * Click on the 'Join Premier Today' header button on the main page to start the
     * checkout flow.
     */
    public void clickJoinPremierButton() {
        waitForElementClickable(HEADER_JOIN_BUTTON_LOCATOR).click();
    }
    
    /**
     * Click on the 'Join Premier Today' comparison table button on the main
     * page to start the checkout flow.
     */
    public void clickComparisonTableJoinPremierButton() {
        scrollElementToCentre(waitForElementVisible(COMPARISON_TABLE_FOOTER_PREMIER_BUY_BUTTON_LOCATOR));
        waitForElementClickable(COMPARISON_TABLE_FOOTER_PREMIER_BUY_BUTTON_LOCATOR).click();
    }
 
    /**
     * Click on the 'Join Premier Today' header button on the main page to start the
     * checkout flow.
     */
    public void clickNavigationBarJoinPremierButton() {
        // scroll to the comparison table footer
        scrollToElement(waitForElementVisible(COMPARISON_TABLE_FOOTER_PREMIER_BUY_BUTTON_LOCATOR));
        // we need to scroll down the page manually so the navigation bar will show up
        scrollDownThePageManually(5);
        waitForElementClickable(NAVIGATION_BAR_JOIN_PREMIER_CTA).click();
    }
    
    /**
     * Verify the 'Origin Access' page is displayed.
     *
     * @return true if page displayed, false otherwise
     */
    public boolean verifyPageReached() {
        return  waitIsElementVisible(ORIGIN_PREMIER_HERO_LOCATOR);
    }

    /**
     * Refresh the 'Origin Access' page to change from 'Start Your Trial' to
     * 'Join Origin' once the subscription has expired (With retries).
     */
    public void refreshPageForJoinButton() throws InterruptedException {
        for (int i = 1; i <= 20; i++) {
            driver.get(driver.getCurrentUrl());
            TimeUnit.SECONDS.sleep(30);
            if (verifyJoinAccessHeroButtonExists()) {
                return; // returns void to make sure when the check fails after 20 retries it should throw an exception
            }
        }

        throw new RuntimeException("Couldn't refresh the page successfully");
    }

    /**
     * Verifies the 'Play First' section title
     *
     * @return true if the title is found and matches the keywords, false
     * otherwise
     */
    public boolean verifyPlayFirstSectionTitle() {
        scrollElementToCentre(waitForElementPresent(PLAY_FIRST_TITLE_LOCATOR));
        return StringHelper.containsIgnoreCase(waitForElementVisible(PLAY_FIRST_TITLE_LOCATOR).getText(), PLAY_FIRST_TITLE_KEYWORDS);
    }

    /**
     * Gets the WebElements of all description bullet points of 'Play First'
     * section
     *
     */
    private List<WebElement> getAllPlayFirstBulletPoints() {
        return waitForAllElementsVisible(PLAY_FIRST_DESCRIPTION_LOCATOR);
    }

    /**
     * Verifies the number of bullets points in 'Play First' section
     *
     * @param numberOfBulletPoints number of bullet points to check against
     * @return true if the number is a match, false otherwise
     */
    public boolean verifyPlayFirstBulletPoints(int numberOfBulletPoints) {
        return getAllPlayFirstBulletPoints().size() == numberOfBulletPoints;
    }

    /**
     * Verifies the background image in 'Play First' section is displayed
     *
     * @return true if the image is displayed, false otherwise
     */
    public boolean verifyPlayFirstBackgroundImage() {
        return verifyUrlResponseCode(waitForElementVisible(PLAY_FIRST_BACKGROUND_IMG_LOCATOR).getAttribute("src"));
    }

    /**
     * Verifies the foreground image in 'Play First' section is displayed
     *
     * @return true if the image is displayed, false otherwise
     */
    public boolean verifyPlayFirstForegroundImage() {
        return verifyUrlResponseCode(waitForElementVisible(PLAY_FRIST_FOREGROUND_IMG_LOCATOR).getAttribute("src"));
    }

    /**
     * Verifies the game logo image in 'Play First' section is displayed
     *
     * @return true if the logo is displayed, false otherwise
     */
    public boolean verifyPlayFirstGameLogo() {
        return verifyUrlResponseCode(waitForElementVisible(PLAY_FRIST_GAME_LOGO_LOCATOR).getAttribute("src"));
    }

    /**
     * Verifies the text under game logo in 'Play First' section
     *
     * @return if the text is a match, false otherwise
     */
    public boolean verifyPlayFirstGameAvailability() {
        return StringHelper.containsAnyIgnoreCase(waitForElementVisible(PLAY_FIRST_GAME_AVAILABILITY_LOCATOR).getText(), PLAY_FIRST_GAME_AVAILABILITY_KEYWORDS);
    }

    /**
     * Clicks the 'Compare Plan' tab
     *
     */
    public void clickComparePlanTab() {
        waitForElementClickable(NAVIGATOR_BAR_COMPARE_PLAN_TAB_LOCATOR).click();
    }

    /**
     * Verifies the 'Comparison table' CTAs are attached to 'Navigation bar'
     * when 'Navigation bar' is pinned to the top of the page
     *
     * @return true if the CTAs are attached, false otherwise
     */
    public boolean verifyComparisonTableCTANavigationBar() {
        scrollToBottom();
        sleep(1000); // waiting for page to scroll down completly and CTAs to attach
        return StringHelper.containsAnyIgnoreCase(waitForElementVisible(NAVIGATOR_BAR_COMPARISON_TABLE_CTA).getAttribute("class"), "l-origin-section");
    }

    /**
     * Verifies 'Compare Plan' tab is displayed in the Navigation bar
     *
     * @return true if the tab is found, false otherwise
     */
    public boolean verifyComparePlanTabButton() {
        scrollToBottom();
        return waitIsElementVisible(NAVIGATOR_BAR_COMPARE_PLAN_TAB_LOCATOR);
    }

    /**
     * Verifies the 'Navigation bar' is present under the comparison table
     *
     * @return true if navigation bar is displayed, false otherwise
     */
    public boolean verifyNavigationbarExists() {
        scrollElementToCentre(waitForElementPresent(NAVIGATOR_BAR_LOCATOR));
        return waitIsElementVisible(NAVIGATOR_BAR_LOCATOR);
    }
    
     /**
     * Verifies 'Join Premier' CTA is visible in the 'Navigation bar'
     *
     * @return true if button is displayed, false otherwise
     */
    public boolean verifyNavigationbarJoinPremierButtonVisible() {
        // scroll to the comparison table footer
        scrollToElement(waitForElementVisible(COMPARISON_TABLE_FOOTER_PREMIER_BUY_BUTTON_LOCATOR));
        // we need to scroll down the page manually so the navigation bar show up
        scrollDownThePageManually(5);
        return waitIsElementVisible(NAVIGATION_BAR_JOIN_PREMIER_CTA);
    }
    /**
     * Verifies the 'Navigation bar' is visible below the 'Comparison table'
     *
     * @return true if navigation bar is displayed, false otherwise
     */
    public boolean verifyNavigationbarVisibleBelowComparisonTable() {
        return getYCoordinate(waitForElementVisible(COMPARISON_TABLE_LOCATOR)) < getYCoordinate(waitForElementPresent(NAVIGATOR_BAR_LOCATOR));
    }

    /**
     * Verifies the 'Premier games' section is visible
     *
     * @return true if the section is visible, false otherwise
     */
    public boolean verifyPremierGamesSectionVisible() {
        scrollElementToCentre(waitForElementVisible(PREMIER_GAMES_SECTION_LOCATOR));
        return waitIsElementVisible(PREMIER_GAMES_SECTION_LOCATOR);
    }
    
    /**
     * Verifies 'Premier vault' games are visible
     *
     * @return true if the list of premier vault games is visible and not empty,
     * false otherwise
     */
    public boolean verifyPremierGamesListVisible() {
        return !waitForAllElementsVisible(PREMIER_GAME_TILE_LOCATOR).isEmpty();
    }
    
    /**
     * Verifies the 'Basic and Premier games' section is visible
     *
     * @return true if the section is visible, false otherwise
     */
    public boolean verifyBasicAndPremierGamesSectionVisible() {
        scrollElementToCentre(waitForElementVisible(BASIC_AND_PREMIER_SECTION_LOCATOR));
        return waitIsElementVisible(BASIC_AND_PREMIER_SECTION_LOCATOR);
    }
    
    /**
     * Verifies 'Premier and Basic vault' games are visible
     *
     * @return true if the list of 'premier and basic vault' games is visible
     * and not empty, false otherwise
     */
    public boolean verifyBasicAndPremierVaultGamesListVisible() {
        return !waitForAllElementsVisible(BASIC_AND_PREMIER_VAULT_TILES_LOCATOR).isEmpty();
    }
    
    /**
     * Verifies the 'Upgrading to Premier' section is visible
     *
     * @return true if the section is visible, false otherwise
     */
    public boolean verifyUpgradingToPremierSectionVisible() {
        scrollElementToCentre(waitForElementVisible(UPGRADING_TO_PREMIER__SECTION_LOCATOR));
        return waitIsElementVisible(UPGRADING_TO_PREMIER__SECTION_LOCATOR);
    }
    
    /**
     * Verifies the 'Play first' section is visible
     *
     * @return true if the section is visible, false otherwise
     */
    public boolean verifyPlayFirstSectionVisible() {
        scrollElementToCentre(waitForElementVisible(PLAY_FIRST_SECTION_LOCATOR));
        return waitIsElementVisible(PLAY_FIRST_SECTION_LOCATOR);
    }
    
    /**
     * Verifies the 'FAQ' section is visible
     *
     * @return true if the section is visible, false otherwise
     */
    public boolean verifyFAQSectionVisible() {
        scrollElementToCentre(waitForElementVisible(FAQ_SECTION_LOCATOR));
        return waitIsElementVisible(FAQ_SECTION_LOCATOR);
    }
    
    /**
     * Verifies the 'Legal' section is visible
     *
     * @return true if the section is visible, false otherwise
     */
    public boolean verifyLegalSectionVisible() {
        scrollElementToCentre(waitForElementVisible(LEGAL_SECTION_LOCATOR));
        return waitIsElementVisible(LEGAL_SECTION_LOCATOR);
    }
    
    /**
     * Verifies the 'Games In EA Access' section is displayed when clicking on
     * 'Games in EA Access' tab
     *
     * @return true if section title and entitlement is displayed, false
     * otherwise
     */
    public boolean verifyGamesInEAAccessSection() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(GAMES_IN_EA_ACCESS_SECTION_TITLE_LOCATOR).getText(), GAMES_IN_EA_ACCESS_TITLE_KEYWORDS);
    }

    /**
     * Gets all the tiles displayed in 'Games in EA Access' section
     *
     * @return a list of WebElements of all tiles in 'Games in EA Access'
     * section
     */
    private List<WebElement> getAllGamesInEAAccessTiles() {
        return waitForAllElementsVisible(GAMES_IN_EA_ACCESS_ENTITLEMENT_TILE_LOCATOR);
    }

    /**
     * Verifies tiles in 'Games in EA Access' section are displayed horizontally
     * and the image is displayed
     *
     * @return true if the attribute which sets the tile positions and the image
     * is displayed
     * @throws IOException if an I/O exception occurs.
     */
    public boolean verifyGamesInEAAccessSectionTiles() throws IOException {
        for (WebElement element : getAllGamesInEAAccessTiles()) {
            if (element.getCssValue("flex").equals(null) && RestfulHelper.getResponseCode(element.getAttribute("image")) != 200) {
                return false;
            }
        }
        return true;
    }

    /**
     * Verifies the tile violator matches any of the available keywords
     *
     * @return true if a keyword is found, false otherwise
     */
    public boolean verifyGamesInEAAccessSectionTilesViolator() {
        for (WebElement element : getAllGamesInEAAccessTiles()) {
            if (!StringHelper.containsAnyIgnoreCase(element.findElement(GAMES_IN_EA_ACCESS_ENTITLEMENT_TILE_VIOLATOR_LOCATOR).getText(), GAMES_IN_EA_ACCESS_TILE_VIOLATOR_KEYWORDS)) {
                return false;
            }
        }
        return true;
    }
    /**
     * Gets all the entitlement titles of the displayed tiles in 'Games in EA
     * Access' page
     *
     * @return a list of entitlement titles
     */
    public List<String> getGamesInEAAccessSectionTilesTitle() {
        List<String> titles = new ArrayList<>();
        for (WebElement element : getAllGamesInEAAccessTiles()) {
            titles.add(element.findElement(GAMES_IN_EA_ACCESS_ENTITLEMENT_TILE_TITLE_LOCATOR).getText());
        }
        return titles;
    }

    /**
     * Verifies entitlement title is displayed under entitlement tile
     *
     * @return true if the title is displayed, false otherwise
     */
    public boolean verifyGamesInEAAccessSectionTilesTitle() {
        for (WebElement element : getAllGamesInEAAccessTiles()) {
            if (StringHelper.nullOrEmpty(element.findElement(GAMES_IN_EA_ACCESS_ENTITLEMENT_TILE_TITLE_LOCATOR).getText())) {
                return false;
            }
        }
        return true;
    }

    /**
     * Verifies entitlement description is displayed under entitlement tile
     *
     * @return true if the description is displayed, false otherwise
     */
    public boolean verifyGamesInEAAccessSectionTilesDescription() {
        for (WebElement element : getAllGamesInEAAccessTiles()) {
            if (StringHelper.nullOrEmpty(element.findElement(GAMES_IN_EA_ACCESS_ENTITLEMENT_TILE_DESCRIPTION_LOCATOR).getText())) {
                return false;
            }
        }
        return true;
    }

    /**
     * Clicks the Tile in 'Games in EA Access' section
     *
     */
    public void clickGamesInEAAccessSectionTile() {
        getRandomWebElement(getAllGamesInEAAccessTiles()).click();
    }

    /**
     * Clicks the Title in 'Games in EA Access' section
     *
     */
    public void clickGamesInEAAccessSectionTileTitle() {
        getRandomWebElement(getAllGamesInEAAccessTiles()).findElement(GAMES_IN_EA_ACCESS_ENTITLEMENT_TILE_TITLE_LOCATOR).click();
    }

    /**
     * Verifies the Comparison table section is displayed
     *
     * @return true if the table is displayed, false otherwise
     */
    public boolean verifyComparePlanSection() {
        scrollElementToCentre(waitForElementVisible(COMPARISON_TABLE_LOCATOR));
        return waitIsElementVisible(COMPARISON_TABLE_LOCATOR);
    }

    /**
     * Verifies the 'How Upgrading Works' section is displayed when clicking on
     * 'How Upgrading Works' tab
     *
     * @return true if section title and entitlement is displayed, false
     * otherwise
     */
    public boolean verifyHowUpgradingWorksSection() {
        return waitIsElementVisible(HOW_UPGRADING_WORKS_TITLE_LOCATOR)
                && StringHelper.containsIgnoreCase(waitForElementVisible(HOW_UPGRADING_WORKS_DESCRIPTION).getText(), HOW_UPGRADING_WORKS_DESCRIPTION_KEYWORDS);
    }

    /**
     * Clicks the 'Games In EA Access' tab
     *
     */
    public void clickGamesInEAAccessTab() {
        scrollElementIntoView(waitForElementPresent(NAVIGATOR_BAR_LOCATOR));
        waitForElementClickable(NAVIGATOR_BAR_GAMES_IN_EA_ACCESS_TAB_LOCATOR).click();
    }

    /**
     * Clicks the 'How Upgrading Works' tab
     *
     */
    public void clickHowUpgradingWorksTab() {
        waitForElementClickable(NAVIGATOR_BAR_HOW_UPGRADE_WORKS_TAB_LOCATOR).click();
    }

    /**
     * Verifies the orange line is displayed under the selected navigation bar
     * of the selected tab
     *
     * @return true if the line is displayed, false otherwise
     */
    public boolean verifyOrangeLineNavigationBar() {
        return getColorOfElement(waitForElementVisible(NAVIGATOR_BAR_SELECTION_LINE), "background-color")
                .equalsIgnoreCase(OriginClientData.PRIMARY_CTA_BUTTON_COLOUR);

    }

    /**
     * Verifies the Navigation bar is sticked to the top of the page when the
     * page is scrolled down
     *
     * @return true if the Navigation bar is displayed at the top of the page ,
     * false otherwise
     */
    public boolean verifyNavigationBarSticksToTop() {
        scrollDownThePageManually(4);
        sleep(1000); // waiting for page to scroll down completly and Navigation Bar to stick
        return StringHelper.containsAnyIgnoreCase(waitForElementVisible(NAVIGATOR_BAR_LOCATOR).getAttribute("class"), "pinned");
    }

    /**
     * Verifies 'Comparison table' background theme color is either dark or
     * light
     *
     * @return true if theme color is either dark or light, false otherwise
     */
    public boolean verifyComparisonTableBackgroundColor() {
        return getColorOfElement(waitForElementVisible(COMPARISON_TABLE_LOCATOR), "background-color").equalsIgnoreCase(OriginClientData.ORIGIN_ACCESS_COMPARISON_TABLE_BACKGROUND);
    }

    /**
     * Verifies 'Basic' Tier column is displayed
     *
     * @return true if the columns are displayed, false otherwise
     */
    public boolean verifyBasicTierColumn() {
        for (WebElement elem : getAllComparisonTableDescriptionRows()) {
            return waitIsElementVisible(elem.findElement(By.cssSelector(" td[ng-class*='tableItem.isBasicCustom']")));
        }
        return false;
    }

    /**
     * Verifies 'Premier' Tier column is displayed
     *
     * @return true if the columns are displayed, false otherwise
     */
    public boolean verifyPremierTierColumn() {
        for (WebElement elem : getAllComparisonTableDescriptionRows()) {
            return waitIsElementVisible(elem.findElement(By.cssSelector(" td[ng-class*='tableItem.isPremierCustom']")));
        }
        return false;
    }

    /**
     * Verifies if 'Basic' Tier has a logo displayed
     *
     * @return true if the logo is displayed, false otherwise
     */
    public boolean verifyBasicTierLogo() {
        return verifyUrlResponseCode(waitForElementVisible(COMPARISON_TABLE_HEADER_BASIC_TIER_LOCATOR).findElement(By.cssSelector(" img")).getAttribute("src"));
    }

    /**
     * Verifies if 'Premier' Tier has a logo displayed
     *
     * @return true if the logo is displayed, false otherwise
     */
    public boolean verifyPremierTierLogo() {
        return verifyUrlResponseCode(waitForElementVisible(COMPARISON_TABLE_HEADER_PREMIER_TIER_LOCATOR).findElement(By.cssSelector(" img")).getAttribute("src"));
    }

    /**
     * Verifies the 'Basic' Tier price displays contains month / year keywords
     *
     * @return true if the keywords are displayed, false otherwise
     */
    public boolean verifyBasicTierPrice() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(COMPARISON_TABLE_HEADER_BASIC_TIER_LOCATOR).getAttribute("subtitle"), COMPARISON_TABLE_HEADER_PRICE_KEYWORDS);
    }

    /**
     * Verifies the 'Premier' Tier price displays contains month / year keywords
     *
     * @return true if the keywords are displayed, false otherwise
     */
    public boolean verifyPremierTierPrice() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(COMPARISON_TABLE_HEADER_PREMIER_TIER_LOCATOR).getAttribute("subtitle"), COMPARISON_TABLE_HEADER_PRICE_KEYWORDS);
    }

    /**
     * Verifie the 'Basic' Tier CTA button
     *
     * @return true if the CTA is found, false otherwise
     */
    public boolean verifyBasicTierCTAButton() {
        return waitIsElementVisible(COMPARISON_TABLE_FOOTER_BASIC_BUY_BUTTON_LOCATOR);
    }

    /**
     * Verifie the 'Premier' Tier CTA button
     *
     * @return true if the CTA is found, false otherwise
     */
    public boolean verifyPremierTierCTAButton() {
        return waitIsElementVisible(COMPARISON_TABLE_FOOTER_PREMIER_BUY_BUTTON_LOCATOR);
    }

    /**
     * Gets all the description rows in the 'Comparison table'
     *
     * @return list of rows as webElements
     */
    public List<WebElement> getAllComparisonTableDescriptionRows() {
        return waitForAllElementsVisible(COMPARISON_TABLE_DESCRIPTION_ROWS);
    }

    /**
     * Verifies the 'Comparison table' for differences by looking at the 'Basic'
     * Tier column for a hyphen '-' then checks if the second column on the same
     * row has a checkmark
     *
     * @return true if atleast a difference is found, false otherwise
     */
    public boolean verifyComparisonTableTierDifferences() {
        for (WebElement elem : getAllComparisonTableDescriptionRows()) {
            if (waitIsElementVisible(elem.findElement(By.cssSelector(" td:nth-child(2) .origin-store-premier-comparison-item-not-checked")))) {
                return waitIsElementVisible(elem.findElement(By.cssSelector(" td:nth-child(3) .origin-store-premier-comparison-item-check")));
            }
        }
        return false;
    }


    /**
     * Verifies price is displayed under the 'Join Origin Access' CTA
     * @param price price to be checked
     * @return true if the price is displayed, false otherwise
     */
    public boolean verifyOriginAccessHeaderPrice(String price) {
        return StringHelper.containsIgnoreCase(getOriginAccessHeaderText(), price);
    }

    /**
     * Gets the text displayed under the 'Join Origin Access' CTA
     *
     * @return a String with the text displayed
     */
    public String getOriginAccessHeaderText() {
        return waitForElementVisible(HEADER_PRICE_LOCATOR).getText();
    }

    /**
     * Click on the 'Join Premier Today' button on the main page
     */
    public void clickJoinPremierSubscriberCta(){
        WebElement joinPremierCta = waitForElementClickable(HEADER_JOIN_BUTTON_LOCATOR);
        scrollToElement(joinPremierCta);
        joinPremierCta.click();
    }
}
