package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.vx.core.helpers.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Component object class for the 'About' dialog.
 *
 * @author palui
 */
public final class AboutDialog extends Dialog {

    protected static final By VERSION_LOCATOR = By.cssSelector("#dialog #otkmodal .otkmodal-content .otkmodal-body .otkc");
    protected static final By RELEASE_NOTES_LINK_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-body .otka");
    protected static final By ABOUT_CLIENT_DESCRIPTION_LOCATOR = By.cssSelector("origin-dialog .origin-dialog-aboutclient-desc");
    

    protected static final String ABOUT_DIALOG_TITLE = "About Origin";
    protected static final String RELEASE_NOTE_LINK_TEXT = "Release Notes";
    protected static final String[] VERSION_KEYWORDS = {"Version", "10"};
    protected static final String[] ABOUT_CLIENT_DESCRIPTION_KEYWORDS = {"End user License Agreement", "Privacy and Cookie Policy" , "Terms of Service" , "LGPL" , "Copyright"};

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public AboutDialog(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify 'About' dialog has about Origin and version related text.
     *
     * @return true if title matches the 'About' dialog title and
     * version contains "Version", false otherwise
     */
    @Override
    public boolean isOpen() {
        WebElement version;
        String title = null;
        try {
            title = getTitle();
            version = waitForElementVisible(VERSION_LOCATOR);
        } catch (Exception e) {
            Logger.console(String.format("Exception: Cannot locate 'About' dialog title and/or version - %s", e.getMessage()), Logger.Types.WARN);
            return false;
        }
        return title.equals(ABOUT_DIALOG_TITLE) && version.getText().contains("Version");
    }

    /**
     * Click 'Release Notes' command link to close the 'About' dialog and open
     * the 'Release Notes' dialog.
     *
     * @return true if the 'Release Notes' were clicked and had the correct text,
     * false otherwise
     */
    public boolean clickReleaseNotesLink() {
        WebElement releaseNotesLink = waitForElementPresent(RELEASE_NOTES_LINK_LOCATOR);
        if (releaseNotesLink != null && releaseNotesLink.getText().equalsIgnoreCase(RELEASE_NOTE_LINK_TEXT)) {
            releaseNotesLink.click();
            return true;
        } else {
            Logger.console("'Release Notes' command link not found in the 'About' dialog", Logger.Types.WARN);
            return false;
        }
    }

    /**
     * Waits for the release notes link to be visible.
     *
     * @return true if the release notes link is visible, false otherwise
     */
    public boolean waitReleaseNotesLinkVisible() {
        return waitIsElementVisible(RELEASE_NOTES_LINK_LOCATOR);
    }
    
    /**
     * Verifies that certain keywords are present in the version area in the dialog
     * 
     * @return true if all keywords are found, false otherwise
     */
    public boolean verifyVersionText(){
        return StringHelper.containsIgnoreCase(waitForElementVisible(VERSION_LOCATOR).getText(), VERSION_KEYWORDS);
    }
    
    /**
     * Verifies that certain keywords are present description area in the dialog
     * 
     * @return true if all keywords are found, false otherwise
     */
    public boolean verifyClientDescription(){
        return StringHelper.containsIgnoreCase(waitForElementVisible(ABOUT_CLIENT_DESCRIPTION_LOCATOR).getText(), ABOUT_CLIENT_DESCRIPTION_KEYWORDS);
    }
}