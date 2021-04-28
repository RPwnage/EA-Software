package com.ea.originx.automation.lib.pageobjects.account;

import com.ea.originx.automation.lib.helpers.StringHelper;
import java.lang.invoke.MethodHandles;
import java.util.List;

import com.ea.vx.originclient.utils.SystemUtilities;
import javax.annotation.Nullable;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represent the 'MyGifts' page ('Account Settings' page with the
 * 'MyGifts' section open)
 *
 * @author nvarthakavi
 */
public class MyGiftsPage extends AccountSettingsPage {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());
    protected static final By UNOPENED_GIFTS_LIST_LOCATOR = By.cssSelector(".context .title .gift-list .open-button");
    protected static final By UNOPENED_GIFTS_LIST_OPEN_BUTTON_LOCATOR = By.cssSelector(".context .title .gift-list .open-button a");
    protected static final By UNOPENED_GIFTS_LOCATOR = By.cssSelector(".context.gifts .title .unopened");
    protected static final By UNOPENED_GIFTS_LIST_MESSAGE_HEADER_LOCATOR = By.cssSelector(".context.gifts .msg_unopened .you-have-unopened-gifts");
    protected static final By UNOPENED_GIFTS_LIST_MESSAGE_TEXT_LOCATOR = By.cssSelector(".context.gifts .msg_unopened .what-are-you-waiting");
    protected static final By OPENED_GIFTS_LIST_LOCATOR = By.cssSelector("#DataGrid .gift-list .status");
    protected static final By OPENED_GIFTS_LIST_ENTITLEMENT_NAME_LOCATOR = By.cssSelector("#DataGrid .gift-list .msg .name");
    protected static final By OPENED_GIFTS_LIST_MESSAGE_DATE_LOCATOR = By.cssSelector("#DataGrid .gift-list .msg .message-date");
    protected static final By OPENED_GIFTS_LIST_MESSAGE_DETAILS_LOCATOR = By.cssSelector("#DataGrid .gift-list .msg .message-detail");
    protected static final By OPENED_GIFTS_LOCATOR = By.cssSelector(".context.gifts .title .title-opened-gifts-t");
    protected static final By NO_OPENED_GIFT_LOCATOR = By.cssSelector(".context.gifts .title .no-opened-gift");

    protected final String OPENED_GIFT_CSS_TMPL = "#DataGrid #OpenedGift_%s";

    protected static final String[] EXPECTED_NO_OPENED_GIFTS_KEYWORDS = {"NO", "OPENED", "GIFT"};
    protected static final String[] EXPECTED_UNOPENED_GIFT_FROM_KEYWORDS = {"unopened", "gift", "from"};
    protected static final String[] EXPECTED_UNOPENED_GIFT_MESSAGE_TEXT = {"open", "it"};

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public MyGiftsPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'MyGifts' section of the 'Account Settings' page is
     * open
     *
     * @return true if open, false otherwise
     */
    public boolean verifyMyGiftsPageSectionReached() {
        return verifySectionReached(NavLinkType.MY_GIFTS);
    }

    /**
     * Get the count of unopened gifts under Unopened Gifts Section
     *
     * @return the number of unopened gifts
     */
    public int getUnopenedGiftsSize() {
        return waitForAllElementsVisible(UNOPENED_GIFTS_LIST_LOCATOR).size();
    }

    /**
     * Get the count of opened gifts under Opened Gifts Section
     *
     * @return the number of opened gifts
     */
    public int getOpenedGiftsSize() {
        return waitForAllElementsVisible(OPENED_GIFTS_LIST_LOCATOR).size();
    }

    /**
     * Verify the 'Opened Gift' section appears with 'No Opened gift to display' message
     * when there are no opened gifts for the user
     *
     * @return true, if displayed with the correct message
     */
    public boolean verifyNoOpenedGifts() {
        boolean isExpectedText = StringHelper.containsAnyIgnoreCase(waitForElementVisible(NO_OPENED_GIFT_LOCATOR).getText(), EXPECTED_NO_OPENED_GIFTS_KEYWORDS);
        return waitIsElementVisible(NO_OPENED_GIFT_LOCATOR) && isExpectedText;
    }

    /**
     * Verify the 'Unopened Gift' section appears
     * when there are  unopened gifts for the user
     *
     * @return true, if displayed with all the unopened gifts
     */
    public boolean verifyUnopenedGifts() {
        return waitIsElementVisible(UNOPENED_GIFTS_LOCATOR) && getUnopenedGiftsSize() > 0;
    }

    /**
     * Verify the 'Opened Gift' section appears
     * when there are opened gifts for the user
     *
     * @return true, if displayed with all the opened gifts
     */
    public boolean verifyOpenedGifts() {
        return waitIsElementVisible(OPENED_GIFTS_LOCATOR) && getOpenedGiftsSize() > 0;
    }

    /**
     * Verify there is name of gift sender in message header for most recent unopened gift
     *
     * @param giftSenderName name of gift sender
     * @return true if the name is found in the message header
     * false otherwise
     */
    public boolean verifyMostRecentUnopenedGiftFromMessageHeader(String giftSenderName) {
        List<WebElement> elements = waitForAllElementsVisible(UNOPENED_GIFTS_LIST_MESSAGE_HEADER_LOCATOR);
        if (elements.size() == 0) {
            throw new RuntimeException("Fail: Unable to find any element for "+UNOPENED_GIFTS_LIST_MESSAGE_HEADER_LOCATOR.toString());
        }
        return StringHelper.containsAnyIgnoreCase(elements.get(0).getText(), EXPECTED_UNOPENED_GIFT_FROM_KEYWORDS)
                && StringHelper.containsIgnoreCase(elements.get(0).getText(), giftSenderName);
    }

    /**
     * Verify there is a fixed message for most recent unopened gift
     *
     * @return true if message is found
     * false otherwise
     */
    public boolean verifyMostRecentUnopenedGiftsMessageExists() {
        List<WebElement> elements = waitForAllElementsVisible(UNOPENED_GIFTS_LIST_MESSAGE_TEXT_LOCATOR);
        if (elements.size() == 0) {
            throw new RuntimeException("Fail: Unable to find any element for "+UNOPENED_GIFTS_LIST_MESSAGE_HEADER_LOCATOR.toString());
        }
           return StringHelper.containsAnyIgnoreCase(elements.get(0).getText(), EXPECTED_UNOPENED_GIFT_MESSAGE_TEXT);
    }

    /**
     * Click button for opening gift
     */
    public void clickLastUnopenedGiftsOpenButton() {
        waitForAllElementsVisible(UNOPENED_GIFTS_LIST_OPEN_BUTTON_LOCATOR).get(0).click();
    }

    /**
     * Get specific opened gift tile by finding entitlement name and gift sender name
     *
     * @param entitlementName name of entitlement received
     * @param giftSenderName name of gift sender
     *
     * @return MyGiftsPageOpenedGiftTile if the opened gift tile is found
     * null if not found
     */
    public @Nullable MyGiftsPageOpenedGiftTile getOpenedGift(String entitlementName, String giftSenderName) {
        List<WebElement> entitlementNameElements, dateElements;
        entitlementNameElements = waitForAllElementsVisible(OPENED_GIFTS_LIST_ENTITLEMENT_NAME_LOCATOR);
        dateElements = waitForAllElementsVisible(OPENED_GIFTS_LIST_MESSAGE_DATE_LOCATOR);
        if (entitlementNameElements.size() > 0 && dateElements.size() > 0 && entitlementNameElements.size() == dateElements.size()) {
            for (int i = 0; entitlementNameElements.size() > i; i++) {
                if (StringHelper.containsAnyIgnoreCase(entitlementNameElements.get(i).getText(), entitlementName)) {
                    String message = dateElements.get(i).getText();
                    if (StringHelper.containsAnyIgnoreCase(message, giftSenderName)) {
                        return new MyGiftsPageOpenedGiftTile(driver, String.format(OPENED_GIFT_CSS_TMPL, i));
                    }
                }
            }
        }
        return null;
    }
}
