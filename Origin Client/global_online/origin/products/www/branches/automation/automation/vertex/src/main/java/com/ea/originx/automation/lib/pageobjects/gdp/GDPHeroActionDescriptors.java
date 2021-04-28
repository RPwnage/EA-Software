package com.ea.originx.automation.lib.pageobjects.gdp;

import com.ea.originx.automation.lib.helpers.I18NUtil;
import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import org.openqa.selenium.By;
import org.openqa.selenium.NoSuchElementException;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Object representing the descriptors on the GDP Hero. Includes everything in
 * the action section that is not related to a CTA, an element to reveal a CTA,
 * and the price. Includes all other descriptions and links.
 *
 * @author cdeaconu
 */
public class GDPHeroActionDescriptors extends EAXVxSiteTemplate{
    
    // Savings and Discount
    protected static final String STORE_PDP_SAVINGS_TMPL = "origin-store-pdp-savings .origin-pdp-savings";
    protected static final By ACCESS_DISCOUNT_LOCATOR = By.cssSelector(STORE_PDP_SAVINGS_TMPL + " .otkprice-sale.origin-pdp-savings-accessdiscount");
    protected static final By ACCESS_SAVINGS_PERCENT_LOCATOR = By.cssSelector(STORE_PDP_SAVINGS_TMPL + " .otkprice-sale:not(.origin-pdp-savings-accessdiscount)");
    protected static final By ORIGINAL_PRICE_STRIKETHROUGH = By.cssSelector(STORE_PDP_SAVINGS_TMPL + " .otkprice-original:not(.origin-pdp-savings-accessdiscount)");
    protected static final String[] ACCESS_DISCOUNT_KEYWORDS = {"origin", "access", "discount", "applied"};
    private final String PRICE_STRIKETHROUGH_TEXT_DECORATION = "line-through";
    
    // Info blocks
    protected static final String ORIGIN_STORE_GDP_INFOBLOCKS_TMPL = "origin-store-gdp-infoblocks";
    protected static final By FIRST_INFO_BLOCK_ICON_LOCATOR = By.cssSelector(ORIGIN_STORE_GDP_INFOBLOCKS_TMPL + " otkex-infoblock:nth-child(1) .otkex-infoblock-icon");
    protected static final By FIRST_INFO_BLOCK_TEXT_LOCATOR = By.cssSelector(ORIGIN_STORE_GDP_INFOBLOCKS_TMPL + " otkex-infoblock:nth-child(1) .otkex-infoblock-text");
    protected static final By FIRST_INFO_BLOCK_LINK_LOCATOR = By.cssSelector(ORIGIN_STORE_GDP_INFOBLOCKS_TMPL + " otkex-infoblock:nth-child(1) .otkex-infoblock-text > a");
    protected static final By INFO_BLOCK_TEXT_LOCATOR = By.cssSelector(ORIGIN_STORE_GDP_INFOBLOCKS_TMPL + " otkex-infoblock .otkex-infoblock-text");
    private static final String[] INFO_BLOCK_KEYWORDS = {"you", "have", "game"};
    public static final String ORIGIN_ACCESS_GAME_AVAILABLE = "You have this game through Origin Access";
    private static final String[] FIRST_INFO_BLOCK_UNOWNED_GAME_TEXT = {"use","you", "must", "have"};
    public static final String FIRST_INFO_BLOCK_MULTIPLE_ENTITLEMENTS_REQUIRED = "To use this, you must have %s. To access all content, %s is required.";
    public static final String FIRST_INFO_BLOCK_REQUIRED_UPLAY = "Requires Ubisoft account and Uplay";
    public static final String FIRST_INFO_BLOCK_OWNED_CONTENT = "You have this content";
    public static final String INFO_BLOCK_PREMIER_PRE_RELEASE_KEYWORDS = "";
    public static final String INCLUDED_WITH_ORIGIN_ACCESS_BADGE = "Included with Origin Access";
    public static final String[] INFO_BLOCK_PREMIER_KEYWORDS = {"included", "Origin", "Access", "Premier"};
    public static final String [] PREORDER_INFO_BLOCK_KEYWORDS = {"you", "pre-ordered", "game"};
    public static final String [] UNRELEASE_ENTITLEMENT_INFO_BLOCK_KEYWORDS = {"release", "date"};
    protected static final By LINK_LOCATOR = By.tagName("a");

    public GDPHeroActionDescriptors(WebDriver driver) {
        super(driver);
    }
    
    /**
     * Verify the First info block icon is visible
     *
     * @return true if the icon is visible, false otherwise
     */
    public boolean verifyFirstInfoBlockIconVisible() {
        return waitIsElementVisible(FIRST_INFO_BLOCK_ICON_LOCATOR);
    }
    
    /**
     * Verify the First info block text is visible
     *
     * @return true if the expected keywords are found in the message, false otherwise
     */
    public boolean verifyFirstInfoBlockTextVisible() {
        try {
            String text = waitForElementVisible(FIRST_INFO_BLOCK_TEXT_LOCATOR, 10).getText();
            return StringHelper.containsAnyIgnoreCase(text, INFO_BLOCK_KEYWORDS);
        } catch (TimeoutException e) {
            return false;
        }
    }
    
    /**
     * Verify first info block title matches the expected title.
     *
     * @param infoBlockText String to match
     * @return true if info block text matches expected, false otherwise
     */
    public boolean verifyFirstInfoBlockTextMatchesExpected(String... infoBlockText) {
        return StringHelper.containsIgnoreCase(waitForElementVisible(FIRST_INFO_BLOCK_TEXT_LOCATOR).getText(), infoBlockText);
    }
    
    /**
     * Verify first info block text contains keywords.
     *
     * @param keywords List of Strings to check if they are substrings of the
     * container
     * @return true if the text contains the keywords, false otherwise
     */
    public boolean verifyFirstInfoBlockTextContainsKeywords(String... keywords){
        return StringHelper.containsIgnoreCase(waitForElementVisible(FIRST_INFO_BLOCK_TEXT_LOCATOR, 10).getText(), keywords);
    }
    
    /**
     * Get First Info Block date
     * @return the date as a String
     */
    public String getFirstInfoBlockDate() {
        return waitForElementVisible(FIRST_INFO_BLOCK_TEXT_LOCATOR).getText().split(": ")[1];
    }
    
    /**
     * Verify first info block text is to the right of the first info block
     * check mark icon.
     *
     * @return true if text is to the right, false otherwise
     */
    public boolean verifyFirstInfoBlockTextIsRightOfIcon() {
        return getXCoordinate(waitForElementVisible(FIRST_INFO_BLOCK_TEXT_LOCATOR)) > getXCoordinate(waitForElementVisible(FIRST_INFO_BLOCK_ICON_LOCATOR));
    }
    
    /**
     * Verify the check mark icon is green
     * 
     * @return true if the check mark icon is green, false otherwise 
     */
    public boolean verifyFirstInfoBlockIconGreen() {
         return getColorOfElement(waitForElementVisible(FIRST_INFO_BLOCK_ICON_LOCATOR)).equals(OriginClientData.CHECK_MARK_ICON_COLOUR);
    }
    
    /**
     * Verify first info block text contains multiple entitlements which are required
     * @param baseEntitlement first entitlement
     * @param dlcEntitlement second entitlement
     * @return true if the text matches the expected, false otherwise
     */
    public boolean verifyFirstInfoBlockRequiredEntitlements(String baseEntitlement, String dlcEntitlement) {
        try {
            String text = waitForElementVisible(FIRST_INFO_BLOCK_TEXT_LOCATOR, 10).getText();
            return String.format(FIRST_INFO_BLOCK_MULTIPLE_ENTITLEMENTS_REQUIRED, baseEntitlement, dlcEntitlement).contains(text);
        } catch (TimeoutException e) {
            return false;
        }
    }
    
    /**
     * Click the First Info Block link
     */
    public void clickFirstInfoBlockLink(){
        waitForElementClickable(FIRST_INFO_BLOCK_LINK_LOCATOR).click();
    }
    
    /**
     * Verify the link from the First Info Block is visible
     * @return true if the link is visible, false otherwise
     */
    public boolean verifyFirstInfoBlockLinkVisible() {
        return waitIsElementVisible(FIRST_INFO_BLOCK_LINK_LOCATOR);
    }
    
    /** 
     * Click the given link from First Info Block
     * 
     * @param linkNumber the number of the link that needs to be clicked
     */
    public void clickGivenLinkFirstInfoBlock(int linkNumber) {
        driver.findElements(FIRST_INFO_BLOCK_LINK_LOCATOR).get(linkNumber -1).click();
    }
    
    /** 
     * Verify the given link from First Info Block is visible
     * 
     * @param linkNumber the number of the link that needs to be visible
     * @return true if the link is visible, false otherwise
     */
    public boolean verifyGivenLinkFirstInfoBlockVisible(int linkNumber) {
        return waitIsElementVisible(driver.findElements(FIRST_INFO_BLOCK_LINK_LOCATOR).get(linkNumber -1));
    }
    /**
     * Gets the n-th link from the list of links located 
     * in the first info block
     * 
     * @param n the number of the link we want 
     * @return a web element, representing the link
     */
    public WebElement getGivenLinkFirstInfoBlock(int n) {
        return driver.findElements(FIRST_INFO_BLOCK_LINK_LOCATOR).get(n-1);
    }
    
    //////////////////////////////////////////////////////////////////////////
    // SAVINGS AND DISCOUNT
    // Displays the savings or discount associated with the entitlement.
    //////////////////////////////////////////////////////////////////////////
    
    /**
     * Verify that the 'Origin Access Discount Applied' is provided for an origin
     * access user.
     *
     * @return true if the expected keywords are found in the message, false
     * otherwise
     */
    public boolean verifyOriginAccessDiscountIsVisible() {
        try {
            String header = waitForElementVisible(ACCESS_DISCOUNT_LOCATOR, 10).getText();
            return StringHelper.containsIgnoreCase(header, ACCESS_DISCOUNT_KEYWORDS);
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify that the original price is striked through and color is gray
     *
     * @return true if the original price is striked through and color is gray,
     * false otherwise
     */
    public boolean verifyOriginalPriceIsStrikedThrough() {
        try {
            boolean textDecoration = StringHelper.containsIgnoreCase(waitForElementVisible(ORIGINAL_PRICE_STRIKETHROUGH).getCssValue("text-decoration"), PRICE_STRIKETHROUGH_TEXT_DECORATION);
            boolean textColour = convertRGBAtoHexColor(waitForElementVisible(ORIGINAL_PRICE_STRIKETHROUGH).getCssValue("color")).equals(OriginClientData.ORIGINAL_PRICE_STRIKETHROUGH_COLOUR);
            return textDecoration && textColour;
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify there is a percentage for discount.
     *
     * @return true if percentage for discount is found, false otherwise.
     */
    public boolean verifyOriginAccessSavingsPercent() {
        return waitIsElementVisible(ACCESS_SAVINGS_PERCENT_LOCATOR);
    }


    /**
     * Verify 'Must have the game' message is visible.
     *
     * @return true if visible else, false otherwise
     */
    public boolean verifyBaseGameRequiredMessageVisible() {
        return getBaseGameRequiredInfoBlockTextElement() != null;
    }

    /**
     * Returns the infoblock WebElement of 'Must have the game', null if it doesn't exist
     *
     * @return infoblock WebElement of 'Must have the game', null if it doesn't exist
     */
    private WebElement getBaseGameRequiredInfoBlockTextElement(){
        try {
            waitForElementVisible(INFO_BLOCK_TEXT_LOCATOR);
            for (WebElement infoBlock : waitForAllElementsVisible(INFO_BLOCK_TEXT_LOCATOR)) {
                if (infoBlock.getText().contains(I18NUtil.getMessage("baseGameWarningMessage")))
                    return infoBlock;
            }
            return null;
        } catch (NoSuchElementException | TimeoutException e) {
            return null;
        }
    }

    /**
     * Clicks on the required base game link, if it exists
     */
    public void clickBaseGameRequired(){
        WebElement infoBlock = getBaseGameRequiredInfoBlockTextElement();
        if(infoBlock != null){
            infoBlock.findElement(LINK_LOCATOR).click();
        }
    }
}
