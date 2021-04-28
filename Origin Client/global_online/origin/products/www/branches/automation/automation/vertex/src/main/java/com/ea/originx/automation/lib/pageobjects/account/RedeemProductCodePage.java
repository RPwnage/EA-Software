package com.ea.originx.automation.lib.pageobjects.account;

import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;

/**
 * Represent the 'Redeem Product Code' settings page ('Account Settings' page
 * with the 'Redeem Product Code' section open)
 *
 * @author palui
 */
public class RedeemProductCodePage extends AccountSettingsPage {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public RedeemProductCodePage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'Redeem Product Code' section of the 'Account Settings' page
     * is open
     *
     * @return true if open, false otherwise
     */
    public boolean verifyRedeemProductCodeSectionReached() {
        return verifySectionReached(NavLinkType.REDEEM_PRODUCT_CODE);
    }
}
