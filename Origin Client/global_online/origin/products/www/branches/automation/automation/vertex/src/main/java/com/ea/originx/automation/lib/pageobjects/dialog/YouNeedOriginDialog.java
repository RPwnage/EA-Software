package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Dialog displayed when clicking 'Get Origin' from PDP page.
 *
 * @author glivingstone
 */
public class YouNeedOriginDialog extends Dialog {

    private static final By YOU_NEED_ORIGIN_LOCATOR = By.cssSelector("origin-dialog-content-playonorigin");
    private static final By DIALOG_BODY_TEXT_LOCATOR = By.cssSelector(".otkmodal-title");
    private static final By DOWNLOAD_ORIGIN_BUTTON = By.cssSelector("#need-origin-download > div.otkmodal-footer.origin-you-need-origin-steps-footer > a");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public YouNeedOriginDialog(WebDriver driver) {
        super(driver, YOU_NEED_ORIGIN_LOCATOR, DIALOG_BODY_TEXT_LOCATOR);
    }
    
    /**
     * Clicks 'Download' button in the 'You need Origin' dialog.
     * 
     */
    public void clickDownloadOriginButton() {
        waitForElementClickable(DOWNLOAD_ORIGIN_BUTTON).click();
    }
}
