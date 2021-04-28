package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.vx.originclient.resources.LanguageInfo;
import com.ea.vx.originclient.resources.LanguageInfo.LanguageEnum;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the 'Language Preferences' dialog, which pops up clicking
 * 'Langauge Preferences' at the navigation sidebar of a browser login.
 *
 * @author palui
 */
public final class LanguagePreferencesDialog extends Dialog {

    protected static final String CONTENT_CSS = ".otkmodal-content origin-dialog-content-selectlanguage.origin-dialog-content";
    protected static final String TITLE_CSS = CONTENT_CSS + " .otkmodal-header > h2";
    protected static final By CONTENT_LOCATOR = By.cssSelector(CONTENT_CSS);
    protected static final By TITLE_LOCATOR = By.cssSelector(TITLE_CSS);

    protected static final String RADIOS_CSS = CONTENT_CSS + " .otkmodal-body .origin-dialog-languages .origin-dialog-language .otkradio";
    protected static final By RADIOS_LOCATOR = By.cssSelector(RADIOS_CSS);
    protected static final String RADIO_INPUTS_CSS = RADIOS_CSS + " input";
    protected static final String RADIO_INPUT_CSS_TMPL = RADIO_INPUTS_CSS + "[id='origin-language-%s']";

    protected static final String SELECTED_SETTING_CSS = RADIO_INPUTS_CSS + "[checked='checked']";
    protected static final By SELECTED_SETTING_LOCATOR = By.cssSelector(SELECTED_SETTING_CSS);
    protected static final String RADIO_LABELS_CSS = RADIOS_CSS + " label";
    protected static final String LABEL_CSS_TMPL = RADIO_LABELS_CSS + "[for='origin-language-%s']";

    protected static final String BUTTONS_CSS = CONTENT_CSS + " .otkmodal-footer button.otkbtn";

    protected static final By SAVE_BUTTON_LOCATOR = By.cssSelector(BUTTONS_CSS + ":nth-of-type(1)");
    protected static final By CANCEL_BUTTON_LOCATOR = By.cssSelector(BUTTONS_CSS + ":nth-of-type(2)");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public LanguagePreferencesDialog(WebDriver driver) {
        super(driver, CONTENT_LOCATOR, TITLE_LOCATOR);
    }

    /**
     * Get the selected language code of the currently selected language.
     *
     * @return Language code of the currently selected language
     */
    public String getSelectedLanguageCode() {
        return waitForElementPresent(SELECTED_SETTING_LOCATOR).getAttribute("value");
    }

    /**
     * Get the selected language description of the currently selected language.
     *
     * @return Language description (localized name) of the currently selected
     * language
     */
    public String getSelectedLanguageDescription() {
        By locator = By.cssSelector(String.format(LABEL_CSS_TMPL, getSelectedLanguageCode()));
        return waitForElementVisible(locator).getAttribute("textContent");
    }

    /**
     * Get the selected language info of the currently selected language.
     *
     * @return LanguageInfo representation of the currently selected language
     */
    public LanguageInfo getSelectedLanguageInfo() {
        return new LanguageInfo(getSelectedLanguageCode());
    }

    /**
     * Verify the currently selected language matches the given language enum.
     *
     * @param languageEnum Language enum to match
     * @return true if selected language matches parameter, false otherwise
     */
    public boolean verifySelectedLanguage(LanguageEnum languageEnum) {
        return getSelectedLanguageInfo().verifyLanguage(languageEnum);
    }

    /**
     * Verify the currently selected language matches the given language enum.
     *
     * @param languageInfo Language info to match
     * @return true if selected language matches given language enum, false otherwise
     */
    public boolean verifySelectedLanguage(LanguageInfo languageInfo) {
        return verifySelectedLanguage(languageInfo.getLanguageEnum());
    }

    /**
     * Get label for languageEnum language.
     *
     * @param languageEnum Language languageEnum
     * @return WebElement of the label
     */
    private WebElement getLabel(LanguageEnum languageEnum) {
        By locator = By.cssSelector(String.format(LABEL_CSS_TMPL, languageEnum.getCode()));
        return waitForElementClickable(locator);
    }

    /**
     * Set language as specified by the parameter and click the 'Save' button.
     *
     * @param languageEnum language to set
     */
    public void setLanguageAndSave(LanguageEnum languageEnum) {
        getLabel(languageEnum).click();
        clickSaveButton();
    }

    /**
     * Set language specified by the parameter and click the 'Save' button.
     *
     * @param languageInfo Language to set
     */
    public void setLanguageAndSave(LanguageInfo languageInfo) {
        setLanguageAndSave(languageInfo.getLanguageEnum());
    }

    /**
     * Click the 'Save' button.
     */
    public void clickSaveButton() {
        waitForElementClickable(SAVE_BUTTON_LOCATOR).click();
    }

    /**
     * Click the 'Cancel' button.
     */
    public void clickCancelButton() {
        waitForElementClickable(CANCEL_BUTTON_LOCATOR).click();
    }
}