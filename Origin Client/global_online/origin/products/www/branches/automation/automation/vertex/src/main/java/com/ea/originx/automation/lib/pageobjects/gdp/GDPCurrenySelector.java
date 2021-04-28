package com.ea.originx.automation.lib.pageobjects.gdp;

import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.support.ui.Select;

/**
 * A page representing the GDP Header currency selector.
 *
 * @author svaghayenegar
 */
public class GDPCurrenySelector extends GDPHeader {

    //Locators
    protected final static By ORIGIN_STORE_CURRENCY_SELECTOR = By.cssSelector(".origin-store-gdp-header-currency");
    protected final static By SELECT_LOCATOR = By.cssSelector("select");

    private Select currencySelect;

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public GDPCurrenySelector(WebDriver driver) {
        super(driver);
    }

    /**
     * Verifies if the currency selector is visible (with wait)
     *
     * @return true if selector is visible, false otherwise
     */
    public boolean verifyCurrencySelectorVisible(){
        return waitIsElementVisible(ORIGIN_STORE_CURRENCY_SELECTOR);
    }

    /**
     * Returns true if selector is visible, false otherwise (without wait)
     *
     * @return true if selector is visible, false otherwise
     */
    public boolean isCurrencySelectorVisible(){
        return isElementVisible(ORIGIN_STORE_CURRENCY_SELECTOR);
    }

    /**
     * Returns the select element
     *
     * @return select element
     */
    private Select getSelector(){
        this.currencySelect = new Select(waitForElementVisible(ORIGIN_STORE_CURRENCY_SELECTOR).findElement(SELECT_LOCATOR));
        return currencySelect;
    }

    /**
     * Selects option by it's value which is offer-id
     *
     * @param offerId offer-id of the entitlement
     */
    public void selectByValue(String offerId){
        getSelector().selectByValue(offerId);
    }
}
