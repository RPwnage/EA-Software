package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.pageobjects.template.QtDialog;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Unable to Broadcast' dialog in OIG.
 *
 * @author mkalaivanan
 * @author jmittertreiner
 */
public class OigUnableToBroadcastDialog extends Dialog {

    private static final By OIG_UNABLE_TO_BROADCAST_LOCATOR = By.xpath("//Origin--UIToolkit--OriginWindowTemplateMsgBox");
    private static final By TITLE_LOCATOR = By.xpath("//*[@id='lblMsgBoxTitle']");
    private static final By CLOSE_BUTTON_LOCATOR = By.xpath("//Origin--UIToolkit--OriginPushButton");
    private static final String OIG_WINDOW_URL = "qtwidget://Origin::Engine::IGOQWidget";
    private static final String EXPECTED_TITLE = "Unable to broadcast this game";
    private QtUnableToBroadcastDialog qtDialog;

    private class QtUnableToBroadcastDialog extends QtDialog {

        /**
         * Constructor
         *
         * @param webDriver Selenium WebDriver
         */
        private QtUnableToBroadcastDialog(WebDriver webDriver) {
            super(webDriver, TITLE_LOCATOR, CLOSE_BUTTON_LOCATOR);
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

        /**
         * Verify the 'Unable to Broadcast' dialog is visible and switch to it if it is.
         *
         * @return true if the 'Unable to Broadcast' dialog exists and visible, false
         * otherwise
         */
        @Override
        public boolean waitIsVisible() {
            switchToUnableToBroadcastWindow();
            return super.waitIsVisible();
        }

        /**
         * Close the dialog.
         */
        @Override
        public void close() {
            switchToUnableToBroadcastWindow();
            super.close();
            EAXVxSiteTemplate.switchToSPAWindow(this.webDriver);
        }

        /**
         * Waits for a window that has the 'Unable to Broadcast' element and
         * switches to that window.
         */
        private void switchToUnableToBroadcastWindow() {
            Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, OIG_WINDOW_URL, OIG_UNABLE_TO_BROADCAST_LOCATOR, 30);
        }
    }

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OigUnableToBroadcastDialog(WebDriver driver) {
        super(driver, null, null);
        qtDialog = new QtUnableToBroadcastDialog(driver);
    }

    /**
     * Override isOpen to also check for matching title.
     *
     * @return true if dialog is open with matching title, false otherwise
     */
    @Override
    public boolean isOpen() {
        return qtDialog.isOpen();
    }

    /**
     * Verify the 'Unable to Broadcast' dialog is visible and switch to it if it is.
     *
     * @return true if the 'Unable to Broadcast' dialog exists and visible, false
     * otherwise
     */
    @Override
    public boolean waitIsVisible() {
        return qtDialog.waitIsVisible();
    }

    /**
     * Close the dialog.
     */
    @Override
    public void close() {
        qtDialog.close();
    }
}
