package com.ea.originx.automation.lib.pageobjects.setup;

import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * A class for setup.
 */
public class ThinSetup extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(ThinSetup.class);

    protected static final int SETUP_WAIT_TIMEOUT = 300000; // 5 minutes
    protected static final int SETUP_WAIT_INTERVAL = 1000;

    protected static final By INSTALL_ORIGIN_LOCATOR = By.cssSelector("#page-splash .content .otkbtn.otkbtn-primary");
    protected static final By AUTO_UPDATE_CHECKBOX_LOCATOR = By.cssSelector("#installoption-autoupdate");
    protected static final By AUTO_UPDATE_LABEL_LOCATOR = By.cssSelector("label[for='installoption-autoupdate']");
    protected static final By ACCEPT_EULA_CHECKBOX_LOCATOR = By.cssSelector("#installoption-accept-eula");
    protected static final By ACCEPT_EULA_LABEL_LOCATOR = By.cssSelector("label[for='installoption-accept-eula']");
    protected static final By CONTINUE_LOCATOR = By.cssSelector("#installoption-action-install");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ThinSetup(WebDriver driver) {
        super(driver);
    }

    /**
     * Click on the 'Install Origin' button.
     */
    public void clickInstallOrigin() {
        _log.debug("clicking \"Install Origin\" button");
        waitForElementClickable(INSTALL_ORIGIN_LOCATOR).click();
    }

    /**
     * Toggle the 'Automatically keep Origin and my games up to date'
     * checkbox.
     *
     * @param checked expected checkbox 'checked' state
     */
    public void toggleAutoUpdateCheckbox(boolean checked) {
        _log.debug("toggling \"AutoUpdate\" checkbox");
        if (waitForElementPresent(AUTO_UPDATE_CHECKBOX_LOCATOR).isSelected() != checked) {
            waitForElementVisible(AUTO_UPDATE_LABEL_LOCATOR).click();
        }
    }

    /**
     * Toggle "I have read and accepted the Origin End user license
     * agreement" checkbox.
     *
     * @param checked expected checkbox 'checked' state
     */
    public void toggleAcceptEULACheckbox(boolean checked) {
        _log.debug("toggling \"EULA\" checkbox");
        if (waitForElementPresent(ACCEPT_EULA_CHECKBOX_LOCATOR).isSelected() != checked) {
            waitForElementVisible(ACCEPT_EULA_LABEL_LOCATOR).click();
        }
    }

    /**
     * Click on the 'Continue' button.
     */
    public void clickContinue() {
        _log.debug("clicking \"Continue\" button");
        waitForElementClickable(CONTINUE_LOCATOR).click();
    }

    /**
     * Wait until Origin.exe is running and OriginThinSetupInternal.exe
     * is terminated.
     *
     * @return true if conditions are satisfied, false otherwise
     */
    public boolean waitInstallationSucceeded() {
        _log.debug("waiting for OriginThinSetupInternal.exe to terminate");
        final boolean isSetupNotRunning = Waits.pollingWaitEx(() -> !ProcessUtil.isProcessRunning(client, "OriginThinSetupInternal.exe"), SETUP_WAIT_TIMEOUT, SETUP_WAIT_INTERVAL, 0);
        if (!isSetupNotRunning) {
            _log.info("OriginThinSetupInternal.exe has not terminated");
        }
        _log.debug("waiting for Origin.exe to run");
        final boolean isOriginRunning = Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, "Origin.exe"), SETUP_WAIT_TIMEOUT, SETUP_WAIT_INTERVAL, 0);
        if (!isOriginRunning) {
            _log.info("Origin.exe is not running");
        }
        return isOriginRunning & isSetupNotRunning;
    }
}