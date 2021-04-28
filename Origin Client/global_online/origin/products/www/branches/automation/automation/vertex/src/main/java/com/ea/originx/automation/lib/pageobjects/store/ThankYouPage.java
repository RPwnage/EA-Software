package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.helpers.I18NUtil;
import com.ea.vx.originclient.utils.Waits;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * A page object that represents the 'Thank You' page after placing successful
 * purchase of a game.
 *
 * @author mkalaivanan
 */
public class ThankYouPage extends EAXVxSiteTemplate {

    protected static final By IFRAME_LOCATOR = By.xpath("//div[@class='origin-iframemodal-content']/iframe");
    protected static final By IFRAME_TITLE_LOCATOR = By.cssSelector(".checkout-content .otkmodal-header .otkmodal-title");
    protected static final By CLOSE_BUTTON_LOCATOR = By.cssSelector("origin-iframe-modal .otkmodal-dialog .otkmodal-close .otkicon-closecircle");
    protected static final By ORDER_NUMBER_LOCATOR = By.cssSelector("#orderNumber h4.otkc");
    protected static final By ENTITLEMENT_NAME_LIST_LOCATOR = By.cssSelector(".what-you-purchased>.cart>.cart-item>.cart-item-details>.cart-item-details-wrapper strong");
    protected static final By PAY_NOW_BUTTON_LOCATOR = By.cssSelector(".checkout-content .otkmodal-footer .otkbtn-primary");
    protected static final By GIFT_DETAILS_LOCATOR = By.cssSelector(".gift-details > h5");
    protected static final String[] EXPECTED_GIFT_DETAILS_MESSAGE_TMPL = {"Gift", "%s"};
    protected static final By PRICE_LOCATOR = By.cssSelector(".cart-total-cost h4.otktitle-2");
    protected static final By DOWNLOAD_ORIGIN_LINK = By.cssSelector(".order-confirmation-text a[href*='download']");
    protected static final By VIEW_ORDER_HISTORY_LINK = By.cssSelector(".order-confirmation-text a[href*='history']");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ThankYouPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'Thank You' page has been reached with the
     * correct header text value.
     *
     * @return true if title contains the correct header text
     */
    public boolean verifyThankYouPageReached() {
        waitForInAnyFrame(ENTITLEMENT_NAME_LIST_LOCATOR);
        String headerText = waitForElementVisible(IFRAME_TITLE_LOCATOR).getText();
        return StringHelper.containsIgnoreCase(I18NUtil.getMessage("thankYouPageHeaderText"), headerText);
    }

    /**
     * Verify the 'Thank You' page has been reached in the scenario
     * where a user has already purchased points for buying dlc and is
     * about to redeem them (ex.: Mass Effect 3 dlc)
     *
     * @return true if the title contains the correct header text
     */
    public boolean verifyPurchasingPointsThankYouPageReached() {
        waitForInAnyFrame(IFRAME_TITLE_LOCATOR);
        String headerText = waitForElementVisible(IFRAME_TITLE_LOCATOR).getText();
        // The header text of the 'Thank You' page is different if the dialog appears
        // for purchasing extra content using purchased points (Ex.: Mass Effect 3 dlc)
        if (waitIsElementVisible(PAY_NOW_BUTTON_LOCATOR, 5)) {
            return StringHelper.containsIgnoreCase(I18NUtil.getMessage("thankYouPagePointsPurchaseHeaderText"), headerText);
        }
        return false;
    }

    /**
     * Wait for the page to load.
     */
    public void waitForThankYouPageToLoad() {
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 30);
        waitForPageToLoad();
        if (!checkIfFrameIsActive("iframe")) {
            waitForFrameAndSwitchToIt(waitForElementVisible(IFRAME_LOCATOR, 30));
        }
        waitForElementVisible(ENTITLEMENT_NAME_LIST_LOCATOR);
    }

    /**
     * Click on the 'Close' button (outside the iFrame).
     */
    public void clickCloseButton() {
        driver.switchTo().defaultContent();
        for (int i = 1; i <= 10; i++) { // retry because sometimes click doesn't work right away
            try {
                clickOnElement(waitForElementClickable(CLOSE_BUTTON_LOCATOR,3));
                if (!waitIsElementVisible(CLOSE_BUTTON_LOCATOR,3)) {
                    return;
                }
            } catch (Exception ex) {
                _log.debug("Clicking on 'Close' button failed in Thank You Page and trying it for " + i + "th time");
            }
        }

    }

    /**
     * To get the total amount for an entitlement for an Origin Access user.
     *
     * @return The total amount
     */
    public double getTotalAmount() {
        return StringHelper.extractNumberFromText(waitForElementVisible(PRICE_LOCATOR).getText());
    }

    /**
     * Get order number.
     *
     * @return Order number from the 'Thank You' page if found, null otherwise
     */
    public String getOrderNumber() {
        try {
            WebElement orderNumberLocator = waitForElementVisible(ORDER_NUMBER_LOCATOR);
            return orderNumberLocator.getText();
        } catch (TimeoutException e) {
            _log.error("Unable to find order number: " + e);
            return null;
        }
    }

    /**
     * Verify purchased entitlement exists on the 'Thank You' page.
     *
     * @return true if purchased entitlement exists, false otherwise
     */
    public boolean verifyPurchasedEntitlementExists(String entitlementName) {
        List<WebElement> elements = waitForAllElementsVisible(ENTITLEMENT_NAME_LIST_LOCATOR);
        for (WebElement element : elements) {
            if (element.getText().equals(entitlementName)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Click the 'Pay Now' button for purchasing extra content using purchased points.
     */
    public void clickPayNowButton() {
        waitForElementClickable(PAY_NOW_BUTTON_LOCATOR).click();
    }

    /**
     * Verifies if the 'Pay Now' button is visible.
     *
     * @return true if the 'Pay Now' button is visible, false otherwise
     */
    public boolean verifyPayNowButtonVisible() {
       return waitIsElementVisible(PAY_NOW_BUTTON_LOCATOR, 3);
    }

    /**
     * Get gifting details on the 'Thank You' page.
     *
     * @return Gifting details, or throw TimeoutException if not visible
     */
    public String getGiftDetails() {
        return waitForElementVisible(GIFT_DETAILS_LOCATOR).getText();
    }

    /**
     * Verify the (green) 'Gift for <user>' message matches the recipient name.
     *
     * @param username Name of gift recipient
     * @return true if gift message matches the recipient name, false otherwise,
     * or throw TimeoutException if message is not visible
     */
    public boolean verifyGiftForUser(String username) {
        String expectedGiftingMessageKeywords = String.format(EXPECTED_GIFT_DETAILS_MESSAGE_TMPL[1], username);
        return StringHelper.containsIgnoreCase(getGiftDetails(), expectedGiftingMessageKeywords);
    }

    /**
     * Verify there is an Origin 'Download' link on browser.
     *
     * @return true if exists, false otherwise
     */
    public boolean verifyOriginDownloadLink() {
        return waitIsElementVisible(DOWNLOAD_ORIGIN_LINK);
    }

    /**
     * Verify there is an 'Order History' link.
     *
     * @return true if exists, false otherwise.
     */
    public boolean verifyViewOrderHistoryLink() {
        return waitIsElementVisible(VIEW_ORDER_HISTORY_LINK);
    }
}