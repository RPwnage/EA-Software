package com.ea.originx.automation.lib.pageobjects.originaccess;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the 'Access Interstitial Page'
 *
 * When a non-subscriber tries to buy a vault entitlement the user is first
 * navigated to Access Interstitial Page, to select whether to continue purchase
 * entitlement or purchase subscription.
 *
 * @author nvarthakavi
 * @author mdobre
 */
public class AccessInterstitialPage extends EAXVxSiteTemplate {

    protected static final By BUY_NOW_CTA_LOCATOR = By.cssSelector("origin-store-osp-interstitial-hero otkex-cta");
    private static final By TITLE_LOCATOR = By.cssSelector("origin-store-osp-interstitial-premier-hero .otktitle-page");
    protected static final By BUY_GAME_CTA_LOCATOR = By.cssSelector(".origin-telemetry-cta-buy .otkex-cta");
    protected static final By BUY_GAME_OSP_CTA_LOCATOR = By.cssSelector(".origin-telemetry-cta-buy-osp .otkex-cta");
    protected static final By PREORDER_OSP_CTA_LOCATOR = By.cssSelector(".origin-telemetry-cta-preorder-osp .otkex-cta");
    protected static final By JOIN_ORIGIN_ACCESS_CTA_LOCATOR = By.cssSelector("origin-store-access-cta .origin-telemetry-primary-cta");
    protected static final By BACKGROUND_IMAGE_LOCATOR = By.cssSelector("origin-background-mediacarousel .item-background-wrapper-inner img");
    protected static final By ORIGIN_STORE_ACCESS_ITEMS_LOCATOR = By.cssSelector(".origin-store-access-list .origin-store-access-item");
    protected static final By HERO_DOWN_ARROW_LOCATOR = By.cssSelector(".origin-store-osp-interstitial-heroindicator");
    protected static final By STORE_PDP_NAV_LOCATOR = By.cssSelector(".origin-telemetry-store-pdp-nav origin-nav-pills .origin-pills");
    protected static final By COMPARISON_TABLE_TITLE_LOCATOR = By.cssSelector("origin-store-osp-comparisontable-wrapper h2");
    protected static final By BOX_ART_IMAGE_LOCATOR = By.cssSelector("origin-store-osp-comparisontable-wrapper .origin-store-pdp-comparison-table-image");
    protected static final By EDITION_TITLE_LOCATOR = By.cssSelector("origin-store-osp-comparisontable-wrapper origin-store-pdp-section-wrapper table thead tr th h6 origin-store-pdp-editions");
    protected static final By INCLUDED_IN_ORIGIN_ACCESS_BADGE_LOCATOR = By.cssSelector("origin-store-osp-comparisontable-wrapper origin-store-osp-comparison-table div table thead tr th p");
    protected static final By GREEN_CHECK_LOCATOR = By.cssSelector("origin-store-osp-comparisontable-wrapper section origin-store-osp-comparison-table div table tbody tr td i.origin-store-pdp-comparison-item-check.otkicon.otkicon-checkcircle");
    protected static final By GREY_CLOSE_CIRCLE_LOCATOR = By.cssSelector("origin-store-osp-comparisontable-wrapper section origin-store-osp-comparison-table div table tbody tr td i.origin-store-pdp-comparison-item-close.otkicon.otkicon-closecircle");
    protected static final By COMPARISON_TABLE_ROW_LOCATOR = By.cssSelector("origin-store-osp-comparisontable-wrapper section .origin-store-pdp-comparison-table-body-row");
    private final String EXPECTED_ON_COMPARISON_TABLE_TITLE = "Compare Editions";
    private final String EXPECTED_ON_ORIGIN_ACCESS_BADGE = "Included with Origin Access";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public AccessInterstitialPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Get page title using the titleLocator.
     *
     * @return Title String
     */
    public final String getTitle() {
        return waitForElementVisible(TITLE_LOCATOR).getText();
    }

    /**
     * Get 'Origin Store Access Items'
     *
     * @return a list of items
     */
    private final List<WebElement> getOriginAccessBulletPoints() {
        return waitForAllElementsVisible(ORIGIN_STORE_ACCESS_ITEMS_LOCATOR);
    }

    /**
     * Verify the 'OSP Interstitial' page has been reached.
     *
     * @return true if the page is displayed, false otherwise
     */
    public boolean verifyOSPInterstitialPageReached() {
        return waitIsElementVisible(TITLE_LOCATOR, 5);
    }

    /**
     * Verify the 'Buy the Game' CTA is visible
     *
     * @return true if the CTA is visible, false otherwise
     */
    public boolean verifyBuyGameCTAVisible() {
        return waitIsElementVisible(BUY_GAME_CTA_LOCATOR);
    }

    /**
     * Verify the 'Buy the Game' CTA is visible (with no waits)
     *
     * @return true if the CTA is visible, false otherwise
     */
    public boolean isBuyGameCTAVisible() {
        return isElementVisible(BUY_GAME_CTA_LOCATOR);
    }

    /**
     * Verify the 'Buy the Game'(OSP) CTA is visible
     *
     * @return true if the CTA is visible, false otherwise
     */
    public boolean verifyBuyGameOSPCTAVisible() {
        return waitIsElementVisible(BUY_GAME_OSP_CTA_LOCATOR);
    }
    
    /**
     * Verify the 'Join Origin Access & Play' CTA is visible
     *
     * @return true if the CTA is visible, false otherwise
     */
    public boolean verifyJoinOriginAccessCTAVisible() {
        return waitIsElementVisible(JOIN_ORIGIN_ACCESS_CTA_LOCATOR);
    }

    /**
     * Verify if title contains the expected keyword(s).
     *
     * @param titleKeywords To check for containment (case ignored)
     * @return true if title contains keyword, false otherwise
     *
     */
    public final boolean verifyTitleContainsIgnoreCase(String... titleKeywords) {
        return StringHelper.containsIgnoreCase(getTitle(), titleKeywords);
    }

    /**
     * Verify the background image is visible
     *
     * @return true if the background image is visible, false otherwise
     */
    public boolean verifyBackgroundImageIsVisible() {
        return waitIsElementVisible(BACKGROUND_IMAGE_LOCATOR);
    }

    /**
     * Verify 'Origin Access' bullet points are showing
     *
     * @return true if there is at least one bullet point visible, false
     * otherwise
     */
    public boolean verifyOriginAccessPointsAreVisible() {
        return getOriginAccessBulletPoints().size() > 0;
    }

    /**
     * Verify the 'Hero down arrow' indicator is visible
     *
     * @return true if the hero down arrow is visible, false otherwise
     */
    public boolean verifyHeroDownArrowIsVisible() {
        return waitIsElementVisible(HERO_DOWN_ARROW_LOCATOR);
    }

    /**
     * Verify the 'Store PDP NAV' is visible
     *
     * @return true if the NAV is visible, false otherwise
     */
    public boolean verifyStorePdpNavVisible() {
        return waitIsElementVisible(STORE_PDP_NAV_LOCATOR);
    }

    /**
     * Click the 'Buy the Game' CTA
     */
    public void clickBuyGameCTA() {
        waitForElementClickable(BUY_GAME_CTA_LOCATOR).click();
    }

    /**
     * Click the 'Buy the Game' CTA
     */
    public void clickBuyNowCTA() {
        waitForElementClickable(BUY_NOW_CTA_LOCATOR).click();
    }

    /**
     * Click the 'Buy the Game'(OSP) CTA
     */
    public void clickBuyGameOSPCTA() {
        waitForElementClickable(BUY_GAME_OSP_CTA_LOCATOR).click();
    }
    
    /**
     * Click the 'Pre-order'(OSP) CTA
     */
    public void clickPreOrderOSPCTA() {
        waitForElementClickable(PREORDER_OSP_CTA_LOCATOR).click();
    }

    /**
     * Click the 'Join Origin Access & Play' CTA
     */
    public void clickJoinOriginAccessCTA() {
        waitForElementClickable(JOIN_ORIGIN_ACCESS_CTA_LOCATOR).click();
    }

    /**
     * Click the 'Hero down arrow' Indicator
     */
    public void clickHeroDownArrow() {
        waitForElementClickable(HERO_DOWN_ARROW_LOCATOR).click();
    }

    /**
     * Scroll down below the header and view the comparison table
     */
    public void scrollToComparisonTable() {
        scrollElementToCentre(waitForElementVisible(COMPARISON_TABLE_TITLE_LOCATOR));
    }

    /**
     * Verify there is a title above the table which has the text 'Compare Editions'
     *
     * @return true if the title exists and if the text matches, false otherwise
     */
    public boolean verifyComparisonTableTitle() {
        String title = waitForElementVisible(COMPARISON_TABLE_TITLE_LOCATOR).getText();
        return StringHelper.containsIgnoreCase(title, EXPECTED_ON_COMPARISON_TABLE_TITLE);
    }

    /**
     * Verify the table shows editions names and box arts at the top
     *
     * @return true if editions names and box arts are visible, false
     * otherwise
     */
    public boolean verifyEditionsVisible() {
        boolean isEditionTitleVisible = waitForAllElementsVisible(EDITION_TITLE_LOCATOR).stream().allMatch(i -> i.getText() != null); // verifies all editions have the titles displayed
        boolean isBoxArtVisible = waitForAllElementsVisible(BOX_ART_IMAGE_LOCATOR).stream().allMatch(i -> i.getAttribute("src") != null); //verifies all box arts have images attached
        return isEditionTitleVisible && isBoxArtVisible;
    }

    /**
     * Verify the badge 'Included in Origin Access' exists
     *
     * @return true if the badge exists, false otherwise
     */
    public boolean verifyIncludedInOriginAccessBadgeVisible() {
        return waitIsElementVisible(INCLUDED_IN_ORIGIN_ACCESS_BADGE_LOCATOR) && waitIsElementVisible(BOX_ART_IMAGE_LOCATOR);
    }

    /**
     * Verify green or grey checks are showing for every row of the comparison
     * table
     *
     * @return true if checks are visible, false otherwise
     */
    public boolean verifyGreenAndGreyChecksShowing() {
        int numberOfRows = waitForAllElementsVisible(COMPARISON_TABLE_ROW_LOCATOR).size();
        int numberOfGreenAndGreyChecks = waitForAllElementsVisible(GREEN_CHECK_LOCATOR).size() + waitForAllElementsVisible(GREY_CLOSE_CIRCLE_LOCATOR).size();
        return numberOfRows == numberOfGreenAndGreyChecks / 2;
    }
}