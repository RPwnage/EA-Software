package com.ea.originx.automation.lib.pageobjects.account;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;

import java.lang.invoke.MethodHandles;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represent a line in the 'Order History' section of the 'Account Settings'
 * page
 *
 * @author palui
 */
public class OrderHistoryLine extends EAXVxSiteTemplate {

    protected final String ORDER_DESCRIPTION_CSS = " .orderdetail_list > .des > h4 > span";
    protected final String ORDER_PRICE_CSS = " .orderdetail_list > .price";
    protected final String ORDER_NUMBER_CSS = " .orderdetail_list > .des > span > label";
    protected final String EXPAND_BUTTON_CSS = " .dr_number";
    protected final String REFUND_LINK_CSS = " .refund a";
    protected final String SENT_GIFT_TEXT_CSS = " .orderdetail_list > .date_gift > h4 > span";
    protected final String RECEIVER_NAME_CSS = " .detail_description > .product_rows > .description_info > dl > .status > .receiver";

    protected final String lineLocatorCss;
    protected final By lineDescriptionLocator;
    protected final By linePriceLocator;
    protected final By lineOrderNumberLocator;
    protected final By lineExpandButtonLocator;
    protected final By lineRefundLinkLocator;
    protected final By lineSentGiftTextLocator;
    protected final By lineReceverNameLocator;

    protected final String[] EXPECTED_ORIGIN_ACCESS_DESCRIPTION_KEYWORDS = {"origin", "access"};
    protected final String[] EXPECTED_ORIGIN_ACCESS_FREE_TRIAl_DESCRIPTION_KEYWORDS = {"origin", "access", "free", "trial"};

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor - made package private. Should only be instantiated from
     * OrderHistoryPage
     *
     * @param driver         WebDriver instance
     * @param lineLocatorCss CSS string to locate the line within the 'Order
     *                       History' section
     */
    OrderHistoryLine(WebDriver driver, String lineLocatorCss) {
        super(driver);
        this.lineLocatorCss = lineLocatorCss;
        this.lineDescriptionLocator = By.cssSelector(lineLocatorCss + ORDER_DESCRIPTION_CSS);
        this.linePriceLocator = By.cssSelector(lineLocatorCss + ORDER_PRICE_CSS);
        this.lineOrderNumberLocator = By.cssSelector(lineLocatorCss + ORDER_NUMBER_CSS);
        this.lineExpandButtonLocator = By.cssSelector(lineLocatorCss + EXPAND_BUTTON_CSS);
        this.lineRefundLinkLocator = By.cssSelector(lineLocatorCss + REFUND_LINK_CSS);
        this.lineSentGiftTextLocator = By.cssSelector(lineLocatorCss + SENT_GIFT_TEXT_CSS);
        this.lineReceverNameLocator = By.cssSelector(lineLocatorCss + RECEIVER_NAME_CSS);
    }

    ///////////////////////////////////////////////////////
    // Getters/Verifiers for Order History Line attributes
    ///////////////////////////////////////////////////////

    /**
     * Verify if this 'Order History Line' is visible
     *
     * @return true if verification passed
     */
    public boolean verifyLineVisible() {
        return waitIsElementVisible(lineDescriptionLocator, 2);
    }

    /**
     * Get description of this 'Order History Line'
     *
     * @return String of the line description
     */
    public String getDescription() {
        return waitForElementVisible(lineDescriptionLocator).getText();
    }

    /**
     * Get Price of this 'Order History Line'
     *
     * @return String of the price
     */
    public String getPrice() {
        return waitForElementVisible(linePriceLocator).getText();
    }


    /**
     * Return order number from this 'Order History Line'
     *
     * @return order number
     */
    public String getOrderNumber() {
        return waitForElementVisible(lineOrderNumberLocator).getText();
    }

    /**
     * Click expand button for this 'Order History Line'
     */
    public void clickExpandButton() {
        waitForElementVisible(lineExpandButtonLocator).click();
    }

    /**
     * Verify if there is refund text and link for this 'Order History Line'
     *
     * @return true if there is refund text and link
     */
    public boolean verifyRequestRefundTextLinkExist() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(lineRefundLinkLocator).getAttribute("href"), "refund") &&
                StringHelper.containsIgnoreCase(waitForElementVisible(lineRefundLinkLocator).getText(), "refund");
    }

    /**
     * Verify there is an entry for a gifted order by looking at 'Sent gift' text
     *
     * @return true if 'Sent gift' text is found above the date
     */
    public boolean verifySentGiftText() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(lineSentGiftTextLocator).getText(), "Sent Gift");
    }

    /**
     * Verify the order description is for an Origin Access Subscription order
     *
     * @return true if keywords matches
     */
    public boolean verifyOriginAccessText() {
        boolean isExpected = StringHelper.containsIgnoreCase(getDescription(), EXPECTED_ORIGIN_ACCESS_DESCRIPTION_KEYWORDS);
        double price = StringHelper.extractNumberFromText(getPrice());
        return isExpected && price != 0;
    }

    /**
     * Verify the order description is for an Origin Access Free Trial Subscription order
     *
     * @return true if keywords matches
     */
    public boolean verifyOriginAccessFreeTrialText() {
        return StringHelper.containsIgnoreCase(getDescription(), EXPECTED_ORIGIN_ACCESS_FREE_TRIAl_DESCRIPTION_KEYWORDS);
    }

    /**
     * Verify the name of the entitlement is what gift sender sent
     *
     * @return true if entitlement gift which gift sender sent is found.
     */
    public boolean verifyNameOfEntitlement(String entitlementName) {
        return StringHelper.containsIgnoreCase(waitForElementVisible(lineDescriptionLocator).getText(), entitlementName);
    }

    /**
     * Verify there is an username of who you sent the gift to
     *
     * @return true if the gift receiver name is correct
     */
    public boolean verifyNameOfReceiver(String userName) {
        return StringHelper.containsIgnoreCase(waitForElementVisible(lineReceverNameLocator).getText(), userName);
    }
}
