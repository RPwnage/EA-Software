package com.ea.originx.automation.lib.pageobjects.template;

import com.ea.vx.originclient.client.OriginClient;
import java.lang.invoke.MethodHandles;
import javax.annotation.Nonnull;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represent the base (abstract) class for Origin Qt dialogs. For a dialog with
 * both web and Qt components (the qt portion usually represents the outer Qt
 * shell), a Dialog subclass with a private QtDialog inner subclass is defined
 * (see RedeemDialog.java for example).
 *
 * @author palui
 */
public abstract class QtDialog extends Dialog {

    public static final By QT_TITLE_LOCATOR = By.id("lblWindowTitle");
    public static final By QT_CLOSE_LOCATOR = By.id("btnClose");

    protected WebDriver webDriver;

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor using titleLocator for visibility check
     *
     * @param webDriver Selenium WebDriver
     * @param titleLocator (non-null) Locator for the dialog title and for
     * checking visibility. Must be specified as the Dialog default is for web
     * dialogs and is not applicable for QtDialog.<br>
     * (Css specification for the titleDialog usually ending with ".title h3").
     * @param closeLocator (non-null) Locator for the close button
     */
    public QtDialog(WebDriver webDriver, @Nonnull By titleLocator, @Nonnull By closeLocator) {
        super(OriginClient.getInstance(webDriver).getQtWebDriver(),
                titleLocator, titleLocator, closeLocator);
        this.webDriver = webDriver; // remember the webDriver
    }

    /**
     * Constructor using the 'X' Button as the default close button.
     *
     * @param webDriver Selenium WebDriver
     * @param titleLocator (non-null) Locator for the dialog title and for
     * checking visibility.
     */
    public QtDialog(WebDriver webDriver, @Nonnull By titleLocator) {
        this(webDriver, titleLocator, QT_CLOSE_LOCATOR);
    }

    /**
     * Constructor using the 'Title Bar' and 'X' Button as default.
     *
     * @param webDriver Selenium WebDriver
     */
    public QtDialog(WebDriver webDriver) {
        this(webDriver, QT_TITLE_LOCATOR, QT_CLOSE_LOCATOR);
    }

    /**
     * For Qt dialogs, cannot click the 'Close' circle to close dialog. Throw an
     * exception instead.
     */
    @Override
    public final void clickCloseCircle() {
        throw new RuntimeException("Qt dialog does not support close circle button. No action taken");
    }

    /**
     * Click the 'X' circle in the upper right corner to close the Qt dialog.
     */
    public final void clickXButton() {
        waitForElementClickable(QT_CLOSE_LOCATOR).click();
        sleep(1000); // wait for any animation complete
    }
}