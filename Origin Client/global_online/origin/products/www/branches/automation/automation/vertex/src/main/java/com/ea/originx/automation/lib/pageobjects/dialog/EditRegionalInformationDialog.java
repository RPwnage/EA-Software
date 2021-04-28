package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * This dialog appears when a usr updates the country/language in 'Account and Billing''s 'About Me' page
 *
 * @author nvarthakavi
 */
public class EditRegionalInformationDialog extends Dialog {

    private static String COUNTRY_OF_RESIDENCE_DROPDOWN_CSS = "#lbl_country";
    private static String LANGUAGE_DROPDOWN_CSS = "#lbl_language";

    private static String DROPDOWN_ARROW_CSS = ".origin-ux-drop-down-control .origin-ux-drop-down-selection";
    private static String DROPDOWN_OPTION_CSS = ".drop-down-options-container .drop-down-options div div .drop-down-item a[value='%s']";

    private static By SAVE_BUTTON_LOCATOR = By.id("savebtn_localeinfo");


    public EditRegionalInformationDialog(WebDriver webDriver) {
        super(webDriver);
    }

    /**
     * Select a given country by passing its corresponding country code
     *
     * @param countryCode the country code to be selected
     */
    public void selectCountry(String countryCode) {
        waitForElementClickable(By.cssSelector(COUNTRY_OF_RESIDENCE_DROPDOWN_CSS + DROPDOWN_ARROW_CSS)).click(); // open dropdown
        WebElement dropDownItemWebElement = waitForElementPresent(By.cssSelector(String.format(DROPDOWN_OPTION_CSS, countryCode.toUpperCase())));
        scrollToElement(dropDownItemWebElement);
        dropDownItemWebElement.click(); //select the option by force clicking the css selector
        clickSave();
    }

    /**
     * Select a given language by passing its corresponding country code
     *
     * @param countryCode  the country code of the language to be changed from the current value selected
     * @param languageCode the language code of the language to be changed from the current value selected
     */
    public void selectLanguage(String countryCode, String languageCode) {
        waitForElementClickable(By.cssSelector(LANGUAGE_DROPDOWN_CSS + DROPDOWN_ARROW_CSS)).click(); // open dropdown
        WebElement dropDownItemWebElement = waitForElementPresent(By.cssSelector(String.format(DROPDOWN_OPTION_CSS, languageCode + "_" + countryCode.toUpperCase()))); //select the option
        scrollToElement(dropDownItemWebElement);
        dropDownItemWebElement.click(); //select the option by force clicking the css selector
        clickSave();
    }

    /**
     * Click 'Save' button to save the changes
     */
    public void clickSave() {
        waitForElementClickable(SAVE_BUTTON_LOCATOR).click();
    }

}
