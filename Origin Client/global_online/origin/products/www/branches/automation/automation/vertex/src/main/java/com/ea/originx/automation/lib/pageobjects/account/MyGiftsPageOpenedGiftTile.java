package com.ea.originx.automation.lib.pageobjects.account;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Created by rocky on 11/20/2016.
 * This is Opened gift tile from my gift section in my account page
 */
public class MyGiftsPageOpenedGiftTile extends EAXVxSiteTemplate {

    protected final String OPENED_GIFT_ENTITLEMENT_NAME_CSS = " .msg .name";
    protected final String OPENED_GIFT_DATE_MESSAGE_CSS = " .msg .message-date";
    protected final String OPENED_GIFT_MESSAGE_DETAIL_CSS = " .msg .message-detail";
    protected final String OPENED_GIFT_STATUS_CSS = " .status";

    protected final String openedGiftLocatorCss;
    protected final By openedGiftEntitlementNameLocator;
    protected final By openedGiftDateMessageLocator;
    protected final By openedGiftMessageDetailLocator;
    protected final By openedGiftStatusLocator;

    /**
     * Constructor - instantiated from MyGiftsPage
     *
     * @param driver               WebDriver instance
     * @param openedGiftLocatorCss CSS string to locate the line within the 'Opened
     *                             Gift' section in account page
     */
    MyGiftsPageOpenedGiftTile(WebDriver driver, String openedGiftLocatorCss) {
        super(driver);
        this.openedGiftLocatorCss = openedGiftLocatorCss;
        this.openedGiftEntitlementNameLocator = By.cssSelector(openedGiftLocatorCss + OPENED_GIFT_ENTITLEMENT_NAME_CSS);
        this.openedGiftDateMessageLocator = By.cssSelector(openedGiftLocatorCss + OPENED_GIFT_DATE_MESSAGE_CSS);
        this.openedGiftMessageDetailLocator = By.cssSelector(openedGiftLocatorCss + OPENED_GIFT_MESSAGE_DETAIL_CSS);
        this.openedGiftStatusLocator = By.cssSelector(openedGiftLocatorCss + OPENED_GIFT_STATUS_CSS);
    }

    /**
     * Verify name of gift sender exists in date message
     *
     * @param giftSender name of gift sender
     * @return true if found
     */
    public boolean verifyGiftSenderNameInDateMessage(String giftSender) {
        return StringHelper.containsIgnoreCase(waitForElementVisible(openedGiftDateMessageLocator).getText(), giftSender);
    }

    /**
     * Verify date info exists in date message
     *
     * @return true if found
     */
    public boolean verifyDateInDateMessage() {
        // getting message like this 'Sent from mike, 2016-11-19' and split string using space
        String[] texts = waitForElementVisible(openedGiftDateMessageLocator).getText().split(" ");
        // getting only date info '2016-11-19' from the string above
        String dateInMessage = texts[texts.length - 1];
        // check date format
        return TestScriptHelper.isValidDateFormatWithDash(dateInMessage);
    }

    /**
     * Verify there is a message gift sender sent in opened gift message detail section
     *
     * @param messageReceived message gift sender wrote
     * @return true if found
     */
    public boolean verifyMessageDetail(String messageReceived) {
        return StringHelper.containsIgnoreCase(waitForElementVisible(openedGiftMessageDetailLocator).getText(), messageReceived);
    }

    /**
     * Verify there is a status info for the opened gift.
     * @return true if found
     */
    public boolean verifyStatusExist() {
        return !waitForElementVisible(openedGiftStatusLocator).getText().isEmpty();
    }


}