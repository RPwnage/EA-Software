package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.QtDialog;
import com.ea.vx.core.helpers.Logger;
import org.openqa.selenium.WebDriver;

/**
 * Component object class for the 9.x client 'PromoManager' dialog.
 *
 * @author palui
 */
public final class PromoManagerDialog extends QtDialog {

    protected static final String PROMO_MANAGER_TITLE_BAR_TITLE = "Featured Today";
    protected static final String PROMO_MANAGER_DIALOG_TITLE = "Promo Manager";  // variable naming may be a problem
    protected static final String PROMO_MANAGER_DIALOG_URL_REGEX = "https://*promotionManager*";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public PromoManagerDialog(WebDriver driver) {
        super(driver);
    }

    /**
     * Wait for 'Promo Manager' Dialog and switch to it.
     *
     * @return true if the 'Promo Manager' dialog is visible and successfully
     * switched to it, false otherwise
     */
    @Override
    public boolean waitIsVisible() {
        sleep(2000);
        waitForPageThatMatches(PROMO_MANAGER_DIALOG_URL_REGEX, 120);
        return true;
    }

    /**
     * Close 'Promo Manager' dialog and switch to Origin window (Clicking X may
     * cause lost of view executor).
     */
    @Override
    public void close() {
        switchToOriginWindow(driver);
        String originWin = driver.getWindowHandle();
        try {
            switchToPromoWindow(driver);
            clickXButton();
        } catch (Exception e) {
            Logger.console(String.format("Exception: No 'Promo Manager' dialog to close - %s", e.getMessage()), Logger.Types.WARN);
        }
        driver.switchTo().window(originWin);
    }
}