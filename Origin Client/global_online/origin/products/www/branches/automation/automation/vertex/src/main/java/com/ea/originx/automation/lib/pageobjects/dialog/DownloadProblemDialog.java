package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * A page object representing the 'Download Problem' dialog.
 *
 * @author lscholte
 * @author palui
 */
public class DownloadProblemDialog extends Dialog {

    protected static final By DIALOG_LOCATOR = By.cssSelector("origin-dialog-content-prompt[header*='encountered a problem']");
    protected static final String EXPECTED_TITLE = "We've encountered a problem";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public DownloadProblemDialog(WebDriver driver) {
        super(driver, DIALOG_LOCATOR);
    }

    /**
     * Override isOpen to also check for matching title.
     *
     * @return true if dialog is open with matching title, false otherwise
     */
    @Override
    public boolean isOpen() {
        return super.isOpen() && verifyTitleEqualsIgnoreCase(EXPECTED_TITLE);
    }
}