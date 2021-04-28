package com.ea.originx.automation.lib.pageobjects.template;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.utils.Waits;
import java.lang.invoke.MethodHandles;
import org.apache.log4j.LogManager;
import org.apache.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.interactions.Actions;

/**
 * Represents the base (abstract) class for Origin pop-up dialogs.
 *
 * @author palui
 */
public abstract class Dialog extends EAXVxSiteTemplate {

    private final String TITLE_CSS = "origin-dialog-content-prompt .otkmodal-header .otkmodal-title";
    private final String CLOSE_CIRCLE_CSS = "origin-dialog .otkmodal-close .otkicon.otkicon-closecircle";
    private final String MODAL_LOCATOR = "div[class*='otkmodal-backdrop']";

    private final By dialogLocator;
    private final By titleLocator;
    private final By closeLocator;

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    ////////////////////////////////////////////////////////////////////////////
    // Constructors - use default locators or pass in to override
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     * @param dialogLocator Locator for the dialog for visibility check (if
     * null, default to standard dialog tile)
     * @param titleLocator Locator for the dialog title (if null, default to
     * standard dialog title)
     * @param closeLocator Locator for the dialog close button (if null, default
     * to close circle button)
     */
    public Dialog(WebDriver driver, By dialogLocator, By titleLocator, By closeLocator) {
        super(driver);
        this.dialogLocator = (dialogLocator == null) ? By.cssSelector(TITLE_CSS) : dialogLocator;
        this.titleLocator = (titleLocator == null) ? By.cssSelector(TITLE_CSS) : titleLocator;
        this.closeLocator = (closeLocator == null) ? By.cssSelector(CLOSE_CIRCLE_CSS) : closeLocator;
    }

    /**
     * Constructor - using default locators
     *
     * @param driver Selenium WebDriver
     */
    public Dialog(WebDriver driver) {
        this(driver, null, null, null);
    }

    /**
     * Constructor - override default dialog locator
     *
     * @param driver Selenium WebDriver
     * @param dialogLocator Overrides the dialog locator for visibility check
     */
    public Dialog(WebDriver driver, By dialogLocator) {
        this(driver, dialogLocator, null, null);
    }

    /**
     * Constructor - override default dialog locator and title locator
     *
     * @param driver Selenium WebDriver
     * @param dialogLocator Overrides the dialog locator for visibility check
     * @param titleLocator Overrides the dialog locator for visibility check and
     * title locator for obtaining title
     */
    public Dialog(WebDriver driver, By dialogLocator, By titleLocator) {
        this(driver, dialogLocator, titleLocator, null);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Overridable Methods - Polymorphic
    // 1) If the method is overriden in a Subclass, calling with an instance of
    //    the subclass will invoke the subclass method
    // 2) If any method M calls these which are overriden in a Subclass, calling
    //    M with an instance of the subclass will invoke the overriden subclass
    //    methods even if M is defined in this class and M is not overriden in the
    //    in the subclass
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Check if dialog is open using the dialogLocator. <br>
     * NOTE: Override this method for more precise visibility check (e.g. match
     * title string) This is necessary if the dialogLocator is shared among
     * multiple dialogs.
     *
     * @return true if dialog is open, false otherwise
     */
    public boolean isOpen() {
        return isElementVisible(dialogLocator);
    }

    /**
     * Wait for dialog to appear. If isOpen method is overridden in subclass,
     * this will polymorphically call subclass's isOpen<br>
     * NOTE: For Qt dialogs, override this class to switch to the Qt window
     * first before checking for visibility.
     *
     * @return true if dialog becomes visible after waiting for it to open,
     * false otherwise
     */
    public boolean waitIsVisible() {
        boolean visible = Waits.pollingWait(this::isOpen);
        if (visible) {
            try {
                waitForAnimationComplete(dialogLocator);
            } catch (TimeoutException e) {
                _log.warn("Warning: timeout waiting for animation to complete");
            }
        }
        return visible;
    }

    /**
     * Click "closeLocator" button to close dialog.<br>
     * NOTE: Examples when this subclass needs to be overridden:<br>
     * 1) In Qt dialog, switch to the Qt window first before we click the close
     * button.<br>
     * 2) If desirable, add wait for the dialog to close before exiting (if the
     * closeAndWait method below is not applicable).<br>
     * 3) If necessary, switch back to the main SPA page after exit.
     */
    public void close() {
        try {
            waitForAnimationComplete(dialogLocator);
        } catch (TimeoutException e) {
            _log.warn("Warning: timeout waiting for animation to complete");
        }
        waitForElementClickable(waitForElementVisible(closeLocator)).click();
        try {
            waitForAnimationComplete(dialogLocator);
        } catch (TimeoutException e) {
            _log.warn("Warning: timeout waiting for animation to complete");
        }
    }
    
    /**
     * Verify 'Close' CTA is visible
     *
     * @return true if CTA is visible, false otherwise
     */
    public boolean verifyCloseButtonVisible(){
        return waitIsElementVisible(closeLocator);
    }
    
    /**
     * Verify 'Close' circle is visible
     *
     * @return true if 'Close' circle is visible, false otherwise
     */
    public boolean verifyCloseCircleVisible(){
        return waitIsElementVisible(By.cssSelector(CLOSE_CIRCLE_CSS));
    }
    
    /**
     * Verify dialog title is visible
     *
     * @return true if title is visible, false otherwise
     */
    public boolean verifyTitleVisible(){
        return waitIsElementVisible(titleLocator);
    }

    /**
     * Click the 'Close' circle in the upper right corner to close dialog.<br>
     * NOTE: Overriden in QtDialog as it is not applicable to Qt dialogs.
     */
    public void clickCloseCircle() {
        waitForElementClickable(waitForElementVisible(By.cssSelector(CLOSE_CIRCLE_CSS))).click();
        try {
            waitForAnimationComplete(dialogLocator);
        } catch (TimeoutException e) {
            _log.warn("Warning: timeout waiting for animation to complete");
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Non-overridable methods that are depending on the overridable ones above.
    // Polymorphism allows these method to call the overriden ones above when
    // invoked from an instance of the subclass.
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Wait for dialog to appear (does not return boolean result).
     */
    public final void waitForVisible() {
        waitIsVisible();
    }

    /**
     * Wait for the dialog to no longer be open.
     *
     * @return true when the dialog is closed, false otherwise
     */
    public final boolean waitIsClose() {
        boolean closed = Waits.pollingWait(() -> !isOpen());
        try {
            waitForAnimationComplete(dialogLocator);
        } catch (TimeoutException e) {
            _log.warn("Warning: timeout waiting for animation to complete");
        }
        return closed;
    }

    /**
     * Wait for dialog to no longer be open (does not return boolean result).
     */
    public final void waitForClose() {
        waitIsClose();
    }

    /**
     * Close dialog and wait for the confirmation that the dialog is closed
     *
     * @return true if the dialog closed, false otherwise
     */
    public final boolean closeAndWait() {
        close();
        return waitIsClose();
    }

    ////////////////////////////////////////////////////////////////////////////
    // Other methods are made non-overridable to prevent unnecessary complexity
    // in defining subclasses
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Get dialog title using the titleLocator.
     *
     * @return Title String
     */
    public final String getTitle() {
        return waitForElementVisible(titleLocator).getText();
    }

    /**
     * Return dialog title with "'s and then trimmed (white spaces removed).
     *
     * @return Title String with "'s removed and then trimmed
     */
    public final String getTrimmedTitle() {
        return getTitle().replace("\"", "").trim();
    }

    /**
     * Verify if title is equal to the expected.<br>
     * NOTE: If language is not English, Skip check and return true for now.
     *
     * @param expectedTitle Expected title to check
     * @return true if equal (or not English), false otherwise
     *
     */
    public final boolean verifyTitleEquals(String expectedTitle) {
        try {
            return !OriginClient.isEnglish(driver) || getTrimmedTitle().equals(expectedTitle);
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify if title is equal to the expected (case ignored).<br>
     * NOTE: If language is not English, Skip check and return true for now.
     *
     * @param expectedTitle Expected title to check (case ignored)
     * @return true if equals ignore case (or not English), false otherwise
     *
     */
    public final boolean verifyTitleEqualsIgnoreCase(String expectedTitle) {
        try {
            return !OriginClient.isEnglish(driver) || getTrimmedTitle().equalsIgnoreCase(expectedTitle);
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify if title contains the expected keyword(s).<br>
     * NOTE: If language is not English, Skip check and return true for now.
     *
     * @param titleKeywords To check for containment (case ignored)
     * @return true if title contains keyword case ignored) (or not English),
     * false otherwise
     *
     */
    public final boolean verifyTitleContainsIgnoreCase(String... titleKeywords) {
        try {
            return !OriginClient.isEnglish(driver) || StringHelper.containsIgnoreCase(getTitle(), titleKeywords);
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     *  Click outside the dialog window
     */
    public final void clickOutsideDialogWindow() {
        WebElement modalLocator = waitForElementClickable(By.cssSelector(MODAL_LOCATOR));
        new Actions(driver).moveToElement(modalLocator, 0, 0).doubleClick().perform();
    }
}