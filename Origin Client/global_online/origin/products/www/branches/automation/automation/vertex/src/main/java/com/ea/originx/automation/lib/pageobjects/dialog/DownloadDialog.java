package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the 'Download' dialogs in Origin.
 *
 * @author caleung
 */
public class DownloadDialog extends Dialog {
    private static final Logger _log = LogManager.getLogger(DownloadEULASummaryDialog.class);

    protected static final By DESCRIPTION_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-body .otkc");
    protected static final By NEXT_BUTTON_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-footer .otkbtn-primary");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public DownloadDialog(WebDriver driver) {
        super(driver);
    }

    /**
     * Gets the download information from the download dialog.
     *
     * @return Download dialog WebElement
     */
    private WebElement getDownloadInfo() {
        return waitForElementVisible(DESCRIPTION_LOCATOR);
    }

    /**
     * Clicks the 'Next' button in the download dialog.
     */
    public void clickNext() {
        _log.debug("clicking Next button");
        getDownloadInfo(); // make sure we are in download info dialog
        waitForElementClickable(NEXT_BUTTON_LOCATOR).click();
    }

    /**
     * Checks to see if the download dialog is visible or not.
     *
     * @return true if the download dialog is visible, false otherwise
     */
    public boolean isDialogVisible() {
        return waitIsElementVisible(DESCRIPTION_LOCATOR);
    }
}