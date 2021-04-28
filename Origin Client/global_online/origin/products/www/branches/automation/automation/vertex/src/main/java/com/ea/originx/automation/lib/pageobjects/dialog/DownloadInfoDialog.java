package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.vx.originclient.helpers.RobotKeyboard;
import com.ea.vx.originclient.client.OriginClient;
import static com.ea.vx.originclient.templates.OASiteTemplate.sleep;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.JavascriptExecutor;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents a 'Download Information' dialog.
 *
 * @author palui
 */
public class DownloadInfoDialog extends Dialog {

    private static final Logger _log = LogManager.getLogger(DownloadInfoDialog.class);

    private static final String CONTENT_CSS = "origin-dialog-content-prompt";
    private static final By DOWNLOAD_INFO_LOCATOR = By.cssSelector(CONTENT_CSS + " origin-dialog-content-downloadinfo");
    private static final By CHANGE_INSTALL_LOCATION_LOCATOR = By.cssSelector(".downloadinfo-location-section > span > button");
    private static final By INSTALL_LOCATION_LOCATOR = By.cssSelector(".downloadinfo-location-section > span > span");
    private static final By DESKTOP_SHORTCUT_LABEL_LOCATOR = By.cssSelector("div:nth-of-type(4) > span > label");
    private static final By DESKTOP_SHORTCUT_CHECKBOX_LOCATOR = By.cssSelector("div:nth-of-type(4) > span > input");
    private static final By STARTMENU_SHORTCUT_LABEL_LOCATOR = By.cssSelector("div:nth-of-type(5) > span > label");
    private static final By STARTMENU_SHORTCUT_CHECKBOX_LOCATOR = By.cssSelector("div:nth-of-type(5) > span > input");
    private static final String EXTRA_CONTENT_LABEL_SELECTOR = ".downloadinfo-extra-content-list > li > span > label[for='%s']";
    private static final String EXTRA_CONTENT_CHECKBOX_SELECTOR = ".downloadinfo-extra-content-list > li > span > input[id='%s']";
    private static final By EXTRA_CONTENTS_LOCATOR = By.cssSelector(".downloadinfo-extra-content-list > li > span");
    private static final By DOWNLOAD_OR_INSTALL_SIZE_LOCATOR = By.cssSelector(".origin-cldialog-downloadinfo-table > div:nth-of-type(3)");
    private static final By INSTALL_OR_SPACE_AVAILABLE_SIZE_LOCATOR = By.cssSelector(".origin-cldialog-downloadinfo-table > div:nth-of-type(4)");
    private static final By DOWNLOAD_INFO_WARNING_TEXT_LOCATOR = By.cssSelector(".origin-cldialog-downloadinfo-warning-text");
    private static final By NONDIP_SPACE_REQUIREMENT_TEXT_LOCATOR = By.cssSelector(".otkmodal-body > div:nth-of-type(1)");
    private static final By NEXT_BUTTON_LOCATOR = By.cssSelector(CONTENT_CSS + " .otkbtn.otkmodal-btn-yes");
    private static final By CANCEL_BUTTON_LOCATOR = By.cssSelector(CONTENT_CSS + " .otkbtn.otkmodal-btn-no");
    private static final By PUNK_BUSTER_CHECKBOX_LOCATOR = By.cssSelector("#chkb0");

    private static final String EXPECTED_TITLE_1 = "Download";
    private static final String EXPECTED_TITLE_2 = "Add To Download Queue";
    private static final String EXPECTED_DESKTOP_SHORTCUT_LABEL_TEXT = "Create desktop shortcut";
    private static final String EXPECTED_STARTMENU_SHORTCUT_LABEL_TEXT = "Create start menu shortcut";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public DownloadInfoDialog(WebDriver driver) {
        super(driver, DOWNLOAD_INFO_LOCATOR);
    }

    /**
     * Override isOpen to also check for matching title.
     *
     * @return true if dialog is open with matching title, false otherwise
     */
    @Override
    public boolean isOpen() {
        return super.isOpen() && (verifyTitleEqualsIgnoreCase(EXPECTED_TITLE_1) || verifyTitleEqualsIgnoreCase(EXPECTED_TITLE_2));
    }
    
    /**
     * Function to change install location
     * 
     * @param newLocation New location to install game
     * @return true if install location changed successfully, false otherwise
     */
    public boolean setInstallLocation(String newLocation) {
        WebElement changeInstallLocationButton = waitForElementClickable(CHANGE_INSTALL_LOCATION_LOCATOR);
        WebElement installLocation;
        // Temporary measure due to a bug with Selenium webdriver
        JavascriptExecutor executor = (JavascriptExecutor) driver;
        executor.executeScript("var elem=arguments[0]; setTimeout(function() {elem.click();}, 100)", changeInstallLocationButton);
        sleep(5000); //wait for the Open Directory window to appear
        try {
            OriginClient client = OriginClient.getInstance(driver);
            RobotKeyboard robotHandler = new RobotKeyboard();
            robotHandler.type(client, newLocation, 200);
            robotHandler.type(client, "\n", 2000);
            //wait to navigate to the newLocation
            sleep(1000);
            robotHandler.type(client, "\n", 2000);
            installLocation = getDownloadInfo().findElement(INSTALL_LOCATION_LOCATOR);
        } catch(Exception e) {
            // If the robot fails, the check in the parent's function
            // will catch it
            _log.debug(e);
            return false;
        }
        return installLocation.getText().startsWith(newLocation);
    }

    /**
     * Function to toggle desktop shortcut checkbox.
     *
     * @param checked Expected checkbox 'checked' state
     */
    public void toggleDesktopShortcutCheckbox(boolean checked) {
        _log.debug("toggling desktop shortcut checkbox");
        if (waitForChildElementPresent(getDownloadInfo(), DESKTOP_SHORTCUT_CHECKBOX_LOCATOR).isSelected() != checked) {
            waitForChildElementVisible(getDownloadInfo(), DESKTOP_SHORTCUT_LABEL_LOCATOR).click();
        }
    }

    /**
     * Function to toggle start menu shortcut checkbox.
     *
     * @param checked Expected checkbox 'checked' state
     */
    public void toggleStartMenuShortcutCheckbox(boolean checked) {
        _log.debug("toggling start menu checkbox");
        if (waitForChildElementPresent(getDownloadInfo(), STARTMENU_SHORTCUT_CHECKBOX_LOCATOR).isSelected() != checked) {
            waitForChildElementVisible(getDownloadInfo(), STARTMENU_SHORTCUT_LABEL_LOCATOR).click();
        }
    }

    /**
     * Function to toggle extra content download checkbox.
     *
     * @param offerID Offer ID String
     * @param checked Expected checkbox 'checked' state
     */
    public void setExtraContentCheckbox(String offerID, boolean checked) {
        _log.debug("toggling extra content checkbox");
        if (waitForChildElementPresent(getDownloadInfo(), By.cssSelector(String.format(EXTRA_CONTENT_CHECKBOX_SELECTOR, offerID))).isSelected() != checked) {
            waitForChildElementVisible(getDownloadInfo(), By.cssSelector(String.format(EXTRA_CONTENT_LABEL_SELECTOR, offerID))).click();
        }
    }

    /**
     * Unchecks all of the boxes under the 'Extra Content', preventing any DLC
     * from being downloaded.
     */
    public void uncheckAllExtraContent() {
        _log.debug("un-checking all extra content checkboxes");
        getDownloadInfo(); // make sure we are in download info dialog
        final List<WebElement> extraContents = driver.findElements(EXTRA_CONTENTS_LOCATOR);
        for (final WebElement extraContent : extraContents) {
            _log.debug("un-checking " + extraContent.getText());
            if (waitForChildElementPresent(extraContent, By.cssSelector("input")).isSelected()) {
                waitForChildElementVisible(extraContent, By.cssSelector("label")).click();
            }
        }
    }

    /**
     * Function to verify download size or install size text matches given text.
     *
     * @param expected String that contains expected value
     * @return true if verification passed, false otherwise
     */
    public boolean verifyDownloadOrInstallSize(String expected) {
        return getDownloadInfo().findElement(DOWNLOAD_OR_INSTALL_SIZE_LOCATOR).getText().equals(expected);
    }

    /**
     * Function to verify installation size or disk space available text matches
     * given text.
     *
     * @param expected String that contains expected value
     * @return true if verification passed, false otherwise
     */
    public boolean verifyInstallOrDiskSpaceAvailableSize(String expected) {
        String displayedSize = getDownloadInfo().findElement(INSTALL_OR_SPACE_AVAILABLE_SIZE_LOCATOR).getText();
        return expected.equalsIgnoreCase(displayedSize);
    }

    /**
     * Function to verify installation size is displayed. We're not interested
     * in what's displayed, but it should probably be a non-zero number.
     *
     * @return true if verification passed, false otherwise
     */
    public boolean verifyInstallOrDiskSpaceAvailableSize() {
        String displayedSize = getDownloadInfo().findElement(INSTALL_OR_SPACE_AVAILABLE_SIZE_LOCATOR).getText();
        double size = Double.parseDouble(displayedSize.substring(0, displayedSize.indexOf(' ')));
        return size != 0;
    }

    /**
     * Function to click on 'Next' button.
     */
    public void clickNext() {
        _log.debug("clicking Next button");
        getDownloadInfo(); // make sure we are in download info dialog
        waitForElementClickable(NEXT_BUTTON_LOCATOR).click();
    }

    /**
     * Get 'Download Info' specific web element for further use.
     *
     * @return WebElement of the download information
     */
    private WebElement getDownloadInfo() {
        return waitForElementVisible(DOWNLOAD_INFO_LOCATOR);
    }

    /**
     * Set the desktop shortcut checkbox to the desired state.
     *
     * @param state Whether the checkbox should be checked or not
     */
    public void setDesktopShortcut(boolean state) {
        if (state != verifyCheckBoxChecked(DESKTOP_SHORTCUT_CHECKBOX_LOCATOR)) {
            waitForChildElementVisible(getDownloadInfo(), DESKTOP_SHORTCUT_LABEL_LOCATOR).click();
        }
    }

    /**
     * Verify 'Download Info' dialog box has label text for 'Desktop Shortcut'
     * checkbox.
     *
     * @return true if the start menu checkbox label has the correct text,
     * false otherwise
     */
    public boolean verifyDesktopShortcutLabel() {
        _log.debug("verifying desktop shortcut label: " + DESKTOP_SHORTCUT_LABEL_LOCATOR.toString());
        return waitForChildElementVisible(getDownloadInfo(), DESKTOP_SHORTCUT_LABEL_LOCATOR).getText().equalsIgnoreCase(EXPECTED_DESKTOP_SHORTCUT_LABEL_TEXT);
    }

    /**
     * Set the 'Start Menu Shortcut' checkbox to the desired state.
     *
     * @param state Whether the checkbox should be checked or not
     */
    public void setStartMenuShortcut(boolean state) {
        if (state != verifyCheckBoxChecked(STARTMENU_SHORTCUT_CHECKBOX_LOCATOR)) {
            waitForChildElementVisible(getDownloadInfo(), STARTMENU_SHORTCUT_LABEL_LOCATOR).click();
        }
    }

    /**
     * Verify 'Download Info' dialog box has label text for 'Menu Shortcut' checkbox.
     *
     * @return true if the start menu checkbox label has the correct text,
     * false otherwise
     */
    public boolean verifyStartMenuShortcutLabel() {
        _log.debug("verifying start menu shortcut label: " + STARTMENU_SHORTCUT_LABEL_LOCATOR.toString());
        return waitForChildElementVisible(getDownloadInfo(), STARTMENU_SHORTCUT_LABEL_LOCATOR).getText().equalsIgnoreCase(EXPECTED_STARTMENU_SHORTCUT_LABEL_TEXT);
    }

    /**
     * Verify 'Desktop Shortcut' and 'Start Menu' shortcut checkboxes are checked.
     *
     * @return true if the desktop and start menu buttons are both checked,
     * false otherwise
     */
    public boolean verifyDesktopAndStartMenuCheckBoxesChecked() {
        return verifyCheckBoxChecked(STARTMENU_SHORTCUT_CHECKBOX_LOCATOR) && verifyCheckBoxChecked(DESKTOP_SHORTCUT_CHECKBOX_LOCATOR);
    }

    /**
     * Verify if checkbox is selected or not.
     *
     * @param checkBoxLocator Locator for the checkbox
     * @return true if the given checkbox is checked, false otherwise
     */
    private boolean verifyCheckBoxChecked(By checkBoxLocator) {
        _log.debug("verifying checkbox checked: " + checkBoxLocator.toString());
        return waitForChildElementPresent(getDownloadInfo(), checkBoxLocator).isSelected();
    }

    /**
     * Verify 'Insufficient Space' warning message is present.
     *
     * @return true if the space warning message is visible, false otherwise
     */
    public boolean verifyInsufficientSpaceWarningMessage() {
        WebElement downloadWarningText = waitForChildElementVisible(getDownloadInfo(), DOWNLOAD_INFO_WARNING_TEXT_LOCATOR);
        if(downloadWarningText != null) {
            return StringHelper.containsIgnoreCase(downloadWarningText.getText(), "space", "free");
        } else {
            return false;
        }
    }
    
    /**
     * Verify the unique 'Insufficient Space' warning message for Non-DIP installer is present.
     *
     * @return true if the space warning message is visible, false otherwise
     */
    public boolean verifyInsufficientSpaceNonDIPMessage() {
        WebElement installSizeWarningText = waitForElementVisible(NONDIP_SPACE_REQUIREMENT_TEXT_LOCATOR);
        if(installSizeWarningText != null) {
            return StringHelper.containsIgnoreCase(installSizeWarningText.getText(), "full", "half");
        } else {
            return false;
        }
    }

    /**
     * Verify the 'Next' button is enabled.
     *
     * @return true if the 'Next' button is enabled, false otherwise
     */
    public boolean verifyNextButtonEnabled() {
        return isElementEnabled(NEXT_BUTTON_LOCATOR);
    }

    /**
     * Click on 'Cancel' button.
     */
    public void clickCancelButton() {
        waitForElementClickable(CANCEL_BUTTON_LOCATOR).click();
    }

    /**
     * Verify the 'Punk Buster' checkbox is present.
     *
     * @return true if the 'Punk Buster' checkbox is present, false otherwise
     */
    public boolean isPunkBusterPresent() {
        return isElementPresent(PUNK_BUSTER_CHECKBOX_LOCATOR);
    }

    /**
     * Returns the state of the 'Punk Buster' checkbox.
     *
     * @return true if the 'Punk Buster' checkbox is checked, false otherwise
     */
    public boolean isPunkBusterChecked() {
        return waitForElementPresent(PUNK_BUSTER_CHECKBOX_LOCATOR).isSelected();
    }

    /**
     * Checks the 'Punk Buster' checkbox.
     */
    public void checkPunkBuster() {
        if (!isPunkBusterChecked()) {
            forceClickElement(waitForElementPresent(PUNK_BUSTER_CHECKBOX_LOCATOR));
        }
    }

    /**
     * Unchecks the 'Punk Buster' checkbox.
     */
    public void uncheckPunkBuster() {
        if (isPunkBusterChecked()) {
            forceClickElement(waitForElementPresent(PUNK_BUSTER_CHECKBOX_LOCATOR));
        }
    }
}