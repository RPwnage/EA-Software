package com.ea.originx.automation.lib.pageobjects.gamelibrary;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 *
 * Represents the 'Download Queue' flyout
 *
 * @author rmcneil
 * @author palui
 */
public class DownloadQueue extends EAXVxSiteTemplate {

    protected static final String DOWNLOAD_MANAGER_MINI_CSS = "origin-download-manager-mini";
    protected static final String DOWNLOAD_QUEUE_FLYOUT_WRAPPER_CSS = DOWNLOAD_MANAGER_MINI_CSS + " .origin-downloadflyout-wrapper";
    protected static final By ISOPEN_LOCATOR = By.cssSelector(DOWNLOAD_QUEUE_FLYOUT_WRAPPER_CSS + ".origin-downloadflyout-isopen");
    protected static final By QUEUE_LOCATOR = By.cssSelector(DOWNLOAD_QUEUE_FLYOUT_WRAPPER_CSS + " origin-download-manager-queue");
    protected static final By CLEAR_BUTTON_LOCATOR = By.cssSelector(DOWNLOAD_QUEUE_FLYOUT_WRAPPER_CSS + " .origin-download-completed .origin-download-clear");

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver
     */
    public DownloadQueue(WebDriver driver) {
        super(driver);
    }

    /**
     * Check if the 'Download Queue' flyout is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean isOpen() {
        return waitIsElementPresent(ISOPEN_LOCATOR, 2);
    }

    /**
     * Clear completed downloads from the 'Download Queue' flyout.
     */
    public void clearCompletedDownloads() {
        waitForElementClickable(CLEAR_BUTTON_LOCATOR).click();
    }
}