package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.vx.originclient.utils.Waits;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebDriverException;
import org.openqa.selenium.WebElement;

/**
 * Represents an 'EULA' dialog.
 *
 * @author palui
 */
public class DownloadEULASummaryDialog extends Dialog {

    private static final Logger _log = LogManager.getLogger(DownloadEULASummaryDialog.class);

    private static final String CONTENT_CSS = "origin-dialog-content-prompt[header*='END USER LICENSE AGREEMENT']";
    private static final String CONTENT_GENERIC_CSS = "origin-dialog-content-prompt";  // For non-English tests
    private static final String SUMMARY_CSS = CONTENT_GENERIC_CSS + " origin-dialog-content-downloadeulasummary";
    private static final By DOWNLOAD_EULA_SUMMARY_LOCATOR = By.cssSelector(SUMMARY_CSS);

    private static final String ACCEPT_CHECK_CSS = SUMMARY_CSS + " .otkcheckbox.downloadeulasummary-accept-check";
    private static final By READ_AND_AGREE_LABEL_LOCATOR = By.cssSelector(ACCEPT_CHECK_CSS + " > label");
    private static final By READ_AND_AGREE_CHECKBOX_LOCATOR = By.cssSelector(ACCEPT_CHECK_CSS + " > input");

    protected static final By DIALOG_EULA_LINKS = By.cssSelector(SUMMARY_CSS + " a.downloadeulasummary-eula");

    private static final By NEXT_BUTTON_LOCATOR = By.cssSelector(CONTENT_GENERIC_CSS + " .otkmodal-footer .otkmodal-btn-yes");
    private static final By CANCEL_BUTTON_LOCATOR = By.cssSelector(CONTENT_GENERIC_CSS + " .otkmodal-footer .otkmodal-btn-no");

    protected static String EXPECTED_TITLE_KEYWORD = "End User License Agreement"; // Can be 'Agreements'
    protected static String EA_PRIVACY_AND_AND_COOKIE_POLICY = "EA Privacy and Cookie Policy";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public DownloadEULASummaryDialog(WebDriver driver) {
        super(driver, DOWNLOAD_EULA_SUMMARY_LOCATOR);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Accept Check
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Verify 'EULA Accept' checkbox appears.
     *
     * @return true if the EULA accept checkbox exists, false otherwise
     */
    public boolean verifyEULAAcceptanceBoxExists() {
        return waitForElementPresent(READ_AND_AGREE_LABEL_LOCATOR).isDisplayed();
    }

    /**
     * Verify 'EULA Accept' checkbox is checked.
     *
     * @return true if the checkbox is checked, false otherwise
     */
    public boolean verifyAcceptCheckboxChecked() {
        return waitForElementPresent(READ_AND_AGREE_CHECKBOX_LOCATOR).isSelected();
    }

    /**
     * Set license accept checkbox.
     *
     * @param checked Expected checkbox state
     */
    public void setLicenseAcceptCheckbox(boolean checked) {
        if (waitForElementPresent(READ_AND_AGREE_CHECKBOX_LOCATOR).isSelected() != checked) {
            waitForElementVisible(READ_AND_AGREE_LABEL_LOCATOR).click();
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // 'Cancel' and 'Next' buttons
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Click 'Cancel' button.
     */
    public void clickCancelButton() {
        waitForElementClickable(CANCEL_BUTTON_LOCATOR).click();
    }

    /**
     * Click 'Next' button.
     */
    public void clickNextButton() {
        waitForElementClickable(NEXT_BUTTON_LOCATOR).click();
    }

    /**
     * Try to click on the 'Next' button, regardless of it being clickable or not.
     * Note that if it is clickable, it will complete the EULA dialog.
     *
     * @return true if the next button is clicked, false otherwise
     */
    public boolean tryToClickNextButton() {
        try {
            waitForElementVisible(NEXT_BUTTON_LOCATOR).click();
        } catch (WebDriverException e) {
            // Do nothing
        }
        return Waits.pollingWait(() -> !isOpen());
    }

    /**
     * Verify the 'Next' button is enabled.
     *
     * @return true if the 'Next' button is enabled, false otherwise
     */
    public boolean verifyNextButtonEnabled() {
        return waitForElementVisible(NEXT_BUTTON_LOCATOR).isEnabled();
    }

    /**
     * Verify 'Next' button is not clickable.
     *
     * @return true if the next button is not clickable, false otherwise
     */
    public boolean verifyNextButtonNotClickable() {
        return !waitForElementVisible(NEXT_BUTTON_LOCATOR).isEnabled();
    }

    ////////////////////////////////////////////////////////////////////////////
    // EULA links
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Click a EULA link on the dialog.
     *
     * @param position The position of the EULA link to click starting at 0
     */
    public void clickEULALink(int position) {
        List<WebElement> allEULAs = waitForAllElementsVisible(DIALOG_EULA_LINKS);
        waitForElementClickable(allEULAs.get(position)).click();
    }

    /**
     * Verify there are multiple EULAs on the dialog.
     *
     * @return true if there are multiple EULAs on the dialog,
     * false otherwise
     */
    public boolean verifyMultipleEULAs() {
        return waitForAllElementsVisible(DIALOG_EULA_LINKS).size() > 1;
    }

    /**
     * Verify the EA Privacy and Cookie Policy appears first in the list of EULAs.
     *
     * @return true if the first EULA is EA Privacy and Cookie Policy EULA,
     * false otherwise
     */
    public boolean verifyEntitlementEULAFirst() {
        List<WebElement> allEULAs = waitForAllElementsVisible(DIALOG_EULA_LINKS);
        return allEULAs.get(0).getText().contains(EA_PRIVACY_AND_AND_COOKIE_POLICY);
    }
    
    /**
     * Verify the entitlement EULA appears second in the list of EULAs.
     *
     * @param entName The name of the entitlement
     * @return true if the second EULA contains the name of the entitlement,
     * false otherwise
     */
    public boolean verifyEntitlementEULASecond(String entName) {
        List<WebElement> allEULAs = waitForAllElementsVisible(DIALOG_EULA_LINKS);
        return allEULAs.get(1).getText().contains(entName);
    }
}