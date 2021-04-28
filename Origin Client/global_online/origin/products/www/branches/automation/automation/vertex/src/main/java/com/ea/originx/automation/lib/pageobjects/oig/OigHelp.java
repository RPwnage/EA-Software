package com.ea.originx.automation.lib.pageobjects.oig;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.pageobjects.template.OAQtSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents 'Help' dialog inside the OIG Window.
 *
 * @author mkalaivanan
 */
public class OigHelp extends OAQtSiteTemplate {

    protected static final By HELP_TITLE_LOCATOR = By.xpath("//*[@id='lblWindowTitle']");

    protected static final String HELP_DIALOG_TITLE = "Help";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OigHelp(WebDriver driver) {
        super(driver);
    }

    /** Verify the 'Help' window title is 'Help'.
     *
     * @return true if window title is 'Help', false otherwise
     */
    public boolean verifyOigHelpTitle() {
        EAXVxSiteTemplate.switchToOigHelpWindow(driver);
        return waitForElementVisible(HELP_TITLE_LOCATOR, 10).getText().equals(HELP_DIALOG_TITLE);
    }
}