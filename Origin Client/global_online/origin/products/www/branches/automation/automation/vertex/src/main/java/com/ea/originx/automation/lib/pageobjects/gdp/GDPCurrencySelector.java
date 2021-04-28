package com.ea.originx.automation.lib.pageobjects.gdp;

import com.ea.originx.automation.lib.helpers.StringHelper;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.support.ui.Select;
import java.util.List;
import java.util.Random;
import java.util.regex.Pattern;

/**
 * A page representing the GDP Header currency selector.
 *
 * @author tdhillon
 */
public class GDPCurrencySelector extends GDPHeader {

    //Locators
    private static final String[] GDP_CURRENCY_PACK_SELECTION_TEXT_KEYWORDS = {"select", "pack"};
    private static final String GDP_CURRENCY_HEADER_CSS = ".origin-store-gdp-header-currency";
    private static final By GDP_CURRENCY_PACK_SELECTION_TEXT_SELECTOR = By.cssSelector(GDP_CURRENCY_HEADER_CSS + " p ");
    private static final By GDP_CURRENCY_PACK_DROPDOWN_BUTTON = By.cssSelector(GDP_CURRENCY_HEADER_CSS + " otkex-select ");
    private static final By GDP_CURRENCY_PACK_DROPDOWN = By.cssSelector(GDP_CURRENCY_HEADER_CSS + " select ");
    private static final String ATTRIBUTE_VALUE = "value";
    
    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public GDPCurrencySelector(WebDriver driver) {
        super(driver);
    }

    /**
     * Clicks Currency Pack dropdown
     */
    public void clickCurrencyPackDropdown() {
        waitForElementClickable(GDP_CURRENCY_PACK_DROPDOWN_BUTTON).click();
    }

    /**
     * Returns the select element
     *
     * @return select element
     */
    private Select getSelector() {
        return new Select(waitForElementPresent(GDP_CURRENCY_PACK_DROPDOWN));
    }

    /**
     * Selects option by it's value which is offer-id
     *
     * @param offerId offer-id of the entitlement
     */
    private void selectByValue(String offerId) {
        getSelector().selectByValue(offerId);
    }

    /**
     * Verifies GDP Currency Pack selection text is visible and correct
     *
     * @return true if text is visible and correct
     */
    public boolean verifyCurrencyPackSelectionText() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(GDP_CURRENCY_PACK_SELECTION_TEXT_SELECTOR).getText(), GDP_CURRENCY_PACK_SELECTION_TEXT_KEYWORDS);
    }

    /**
     * Verifies Currency pack dropdown button is visible
     *
     * @return true if Currency pack dropdown button is visible
     */
    public boolean verifyCurrencyPackDropDownButtonVisible() {
        return waitIsElementVisible(GDP_CURRENCY_PACK_DROPDOWN_BUTTON);
    }

    /**
     * Gets the Currency Pack Dropdown button webelement
     *
     * @return WebElement for Currency Pack Dropdown button
     */
    private WebElement getCurrencyPackDropdownButton() {
        return waitForElementVisible(GDP_CURRENCY_PACK_DROPDOWN_BUTTON);
    }

    /**
     * Gets text on Currency Pack Dropdown button
     *
     * @return text on Currency Pack Dropdown button
     */
    private String getCurrencyPackDropdownButtonText() {
        return getCurrencyPackDropdownButton().getAttribute(ATTRIBUTE_VALUE);
    }

    /**
     * Returns Currency Pack Options from the dropdown Menu
     *
     * @return List of webelements of Currency Pack Options from the dropdown Menu
     */
    private List<WebElement> getCurrencyPackOptionsInDropDownMenu() {
        return getSelector().getOptions();
    }

    /**
     * Returns Currency Pack Option Text from the dropdown Menu
     *
     * @param i position of Currency pack Option
     * @return text for Currency Pack Option Text from the dropdown Menu
     */
    private String getCurrencyPackOptionsMenuText(int i) {
        return getCurrencyPackOptionsInDropDownMenu().get(i).getText();
    }

    /**
     * Verifies OfferIds, Names and position of Price of Currency Options in the dropdown menu
     *
     * @param expectedCurrencyPackNames dropdown values
     * @param expectedCurrencyPackOfferIds currency packs offer Ids
     * @return true if OfferIds, Names and position of Price of Currency Options in the dropdown menu are correct
     */
    public boolean verifyCurrencyPackOptions(String[] expectedCurrencyPackNames, String[] expectedCurrencyPackOfferIds) {
        boolean areCurrencyPackOptionsCorrect = false;
        List<WebElement> currencyPackOptions = getCurrencyPackOptionsInDropDownMenu();
        
        for(int i=0; i < currencyPackOptions.size(); i++) {
            String expectedCurrencyPackName = expectedCurrencyPackNames[i];
            String expectedCurrencyPackOfferId = expectedCurrencyPackOfferIds[i];
            String actualCurrencyPackOfferId = currencyPackOptions.get(i).getAttribute(ATTRIBUTE_VALUE);

            String currencyOptionText = currencyPackOptions.get(i).getText();
            String[] parts = currencyOptionText.split(Pattern.quote(" - $"));
            String actualCurrencyPackName = parts[0];
            String actualCurrencyPackPrice = parts[1];
            
            areCurrencyPackOptionsCorrect = verifyOfferIdIsCorrect(expectedCurrencyPackOfferId, actualCurrencyPackOfferId)
                    && verifyNameIsCorrect(expectedCurrencyPackName, actualCurrencyPackName)
                    && verifyPriceIsFloat(actualCurrencyPackPrice);
        }
        return areCurrencyPackOptionsCorrect;
    }

    /**
     * Verifies OfferIds for currency Pack is correct
     *
     * @param expectedCurrencyPackOfferId
     * @param actualCurrencyPackOfferId
     * @return true if Currency Pack OfferId is correct
     */
    private boolean verifyOfferIdIsCorrect(String expectedCurrencyPackOfferId, String actualCurrencyPackOfferId) {
        boolean areCurrencyPackOfferIdsCorrect = false;
        if(actualCurrencyPackOfferId.equals(expectedCurrencyPackOfferId)) {
            areCurrencyPackOfferIdsCorrect = true;
        }
        return areCurrencyPackOfferIdsCorrect;
    }

    /**
     * Verifies OfferName for currency Pack is correct
     *
     * @param expectedCurrencyPackName
     * @param actualCurrencyPackName
     * @return true if Currency Pack name is correct
     */
    private boolean verifyNameIsCorrect(String expectedCurrencyPackName, String actualCurrencyPackName) {
        boolean areCurrencyPackOptionsNamesCorrect = false;
        if(actualCurrencyPackName.equals(expectedCurrencyPackName)) {
            areCurrencyPackOptionsNamesCorrect = true;
        }
        return areCurrencyPackOptionsNamesCorrect;
    }

    /**
     * Verifies Price of currency pack is a Float
     *
     * @param actualCurrencyPackPrice
     * @return true if Price is Float
     */
    private boolean verifyPriceIsFloat(String actualCurrencyPackPrice) {
        boolean isCurrencyPackPriceFloat;
        try{
            Float.parseFloat(actualCurrencyPackPrice);
            isCurrencyPackPriceFloat = true;
        } catch (NumberFormatException e) {
            isCurrencyPackPriceFloat = false;
        }
        return isCurrencyPackPriceFloat;
    }

    /**
     * Checks if first option in dropdown menu is selected by default
     *
     * @return true if first option in dropdown menu is selected by default
     */
    public boolean verifyFirstOptionInDropdownIsDefault() {
        return getCurrencyPackOptionsInDropDownMenu().get(0).getText().equals(getCurrencyPackDropdownButton().getAttribute(ATTRIBUTE_VALUE));
    }

    /**
     * Checks if selecting a dropdown option changes text on Dropdown button
     *
     * @param currencyPackOfferIds option from dropdown
     * @return true if selecting Currency Pack from Dropdown works
     */
    public boolean verifySelectingCurrencyPackFromDropdownWorks(String[] currencyPackOfferIds) {
        Random rand = new Random();
        int i = rand.nextInt(getCurrencyPackOptionsInDropDownMenu().size());

        selectByValue(currencyPackOfferIds[i]);
        return  getCurrencyPackDropdownButtonText().equals(getCurrencyPackOptionsMenuText(i));
    }

    /**
     * Verifies Price on Buy Button and Dropdown Button is same
     *
     * @param priceOnButton price on GDP Buy Button
     * @return true if Price on Buy Button and Dropdown Button is same
     */
    public boolean verifyPriceOnBuyButtonMatchesPriceOnDropdownButton(String priceOnButton) {
        return getCurrencyPackDropdownButtonText().contains(priceOnButton);
    }
}