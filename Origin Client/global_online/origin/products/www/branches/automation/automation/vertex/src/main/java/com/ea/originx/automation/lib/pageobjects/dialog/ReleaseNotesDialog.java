package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.WebDriver;

/**
 * Component object class for the 'Release Notes' dialog.
 *
 * @author palui
 */
public final class ReleaseNotesDialog extends Dialog {

    protected static final String RELEASE_NOTES_DIALOG_TITLE = "Release Notes";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ReleaseNotesDialog(WebDriver driver) {
        super(driver);
    }
}