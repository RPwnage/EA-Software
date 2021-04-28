package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.support.ui.Select;

/**
 * A page object that represents a 'Download Language Selection' dialog.
 *
 * @author palui
 */
public class DownloadLanguageSelectionDialog extends Dialog {

    private static final Logger _log = LogManager.getLogger(DownloadLanguageSelectionDialog.class);

    private static final String CONTENT_CSS = "origin-dialog-content-prompt";
    private static final String LANGUAGE_OPTION_CSS_TMPL = "option[label='%s']";

    private static final By DOWNLOAD_LANGUAGE_SELECTION_LOCATOR = By.cssSelector(CONTENT_CSS + " origin-dialog-content-downloadlanguageselection");
    private static final By LANGUAGE_SELECTION_LOCATOR = By.cssSelector("select");
    private static final By ACCEPT_BUTTON_LOCATOR = By.cssSelector(CONTENT_CSS + " .otkbtn.otkmodal-btn-yes");

    private static final String EXPECTED_TITLE = "CHOOSE YOUR LANGUAGE";
    private static final String EXPECTED_ENGLISH_OPTION_VALUE = "English GB";

    protected final int timeout;

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public DownloadLanguageSelectionDialog(WebDriver driver) {
        this(driver, 0);
    }

    /**
     * Constructor with visibility timeout in seconds.
     *
     * @param driver Selenium WebDriver
     * @param timeout Visibility timeout in seconds
     */
    public DownloadLanguageSelectionDialog(WebDriver driver, int timeout) {
        super(driver, DOWNLOAD_LANGUAGE_SELECTION_LOCATOR);
        this.timeout = timeout;
    }

    /**
     * Checks if the dialog is open.
     *
     * @return true if the 'Download Language Selection' dialog is open,
     * false otherwise
     */
    @Override
    public boolean isOpen() {
        return super.isOpen() && verifyTitleEqualsIgnoreCase(EXPECTED_TITLE);
    }

    /**
     * Function to change language selection in dropdown list.
     *
     * @param language String that matches option in language dropdown list
     */
    public void setLanguage(String language) {
        _log.debug("set language: " + language);
        new Select(waitForChildElementPresent(getDownloadLanguageSelection(), LANGUAGE_SELECTION_LOCATOR)).selectByVisibleText(language);
    }

    /**
     * Click on the 'Accept' button.
     */
    public void clickAccept() {
        _log.debug("clicking Accept button");
        getDownloadLanguageSelection(); // make sure we are in language selection dialog
        waitForElementClickable(ACCEPT_BUTTON_LOCATOR).click();
    }

    /**
     * Get 'Download Language' dialog's specific web element for further use.
     *
     * @return 'Download Language' dialog WebElement
     */
    private WebElement getDownloadLanguageSelection() {
        if (timeout > 0) {
            return waitForElementVisible(DOWNLOAD_LANGUAGE_SELECTION_LOCATOR, timeout);
        } else {
            return waitForElementVisible(DOWNLOAD_LANGUAGE_SELECTION_LOCATOR);
        }
    }

    /**
     * Verify option 'English' exists in the dropdown.
     *
     * @return true if 'English' exists in the language selection dropdown,
     * false otherwise
     */
    public boolean verifyEnglishLanguageSelectionExists() {
        _log.debug("verifying " + EXPECTED_ENGLISH_OPTION_VALUE + " exists");
        final By optionLocator = By.cssSelector(String.format(LANGUAGE_OPTION_CSS_TMPL, EXPECTED_ENGLISH_OPTION_VALUE));
        try {
            return waitForChildElementPresent(getDownloadLanguageSelection(), optionLocator) != null;
        } catch (TimeoutException e) {
            _log.debug(e);
            return false;
        }
    }
}