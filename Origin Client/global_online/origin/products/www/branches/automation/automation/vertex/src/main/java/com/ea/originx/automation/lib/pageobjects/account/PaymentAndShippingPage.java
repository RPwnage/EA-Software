package com.ea.originx.automation.lib.pageobjects.account;

import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;

/**
 * Represent the 'Payment & Shipping' page ('Account Settings' page with the
 * 'Payment & Shipping' section open)
 *
 * @author palui
 */
public class PaymentAndShippingPage extends AccountSettingsPage {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public PaymentAndShippingPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'Payment & Shipping' section of the 'Account Settings' page is
     * open
     *
     * @return true if open, false otherwise
     */
    public boolean verifyPaymentAndShippingSectionReached() {
        return verifySectionReached(NavLinkType.PAYMENT_AND_SHIPPING);
    }
}
