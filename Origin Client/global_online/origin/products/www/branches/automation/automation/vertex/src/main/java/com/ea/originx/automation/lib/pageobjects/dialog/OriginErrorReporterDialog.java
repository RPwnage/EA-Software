package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.QtDialog;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.core.helpers.Logger;
import java.io.IOException;
import org.openqa.selenium.WebDriver;

/**
 * Component object class for the 'Origin Error Reporter' dialog.
 *
 * @author palui
 */
public class OriginErrorReporterDialog extends QtDialog {

    protected static final String ORIGIN_ERROR_REPORTER_DIALOG_TITLE = "Origin Error Reporter";
    protected static final String ORIGIN_ERROR_REPORTER_PROCESS_NAME = "OriginER.exe";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OriginErrorReporterDialog(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify if 'Origin Error Reporter' process is running.
     *
     * @return true if process is running, false otherwise
     * @throws java.io.IOException
     * @throws java.lang.InterruptedException
     */
    public boolean verifyOriginErrorReporterRunning() throws IOException, InterruptedException {
        boolean oer_running;
        try {
            oer_running = ProcessUtil.isProcessRunning(client, ORIGIN_ERROR_REPORTER_PROCESS_NAME);
        } catch (IOException | InterruptedException e) {
            Logger.console(String.format("Exception: Cannot check if OER running - %s", e.getMessage()), Logger.Types.WARN);
            throw e;
        }
        return oer_running;
    }

    /**
     * Kill any 'Origin Error Reporter' process that is running.
     */
    public void killOriginErrorReporterProcess() {
        ProcessUtil.killProcess(client, ORIGIN_ERROR_REPORTER_PROCESS_NAME);
    }
}