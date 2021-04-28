package com.ea.originx.automation.lib.pageobjects.template;

import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.net.helpers.RestfulHelper;
import com.ea.vx.originclient.templates.OASiteTemplate;
import com.ea.vx.originclient.utils.Waits;
import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import java.util.Random;
import java.util.Set;
import javax.annotation.Nonnull;
import org.openqa.selenium.By;
import org.openqa.selenium.Dimension;
import org.openqa.selenium.NoSuchElementException;
import org.openqa.selenium.NoSuchFrameException;
import org.openqa.selenium.StaleElementReferenceException;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebDriverException;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.support.ui.ExpectedConditions;
import org.openqa.selenium.support.ui.WebDriverWait;

/**
 * Class for the EAX Vx Site Template.
 *
 * @author glivingstone
 */
public abstract class EAXVxSiteTemplate extends OASiteTemplate {

    private static final By DOWNLOAD_GAME_WINDOW_LOCATOR = By.xpath("//Origin--UIToolkit--OriginWindow[@name='Origin' and @id='OriginWindow_MsgBox']"); // Same as 'Remove Game'
    private static final By OIG_OFFLINE_MODE_MSGBOX_LOCATOR = By.xpath("//Origin--UIToolkit--OriginWindow[@name='OFFLINE MODE' and @id='OriginWindow_MsgBox']");
    private static final By OIG_DOWNLOAD_MANAGER_WINDOW_LOCATOR = By.xpath("//Origin--UIToolkit--OriginWebView");
    private static final By NET_PROMOTER_TAG_LOCATOR = By.xpath("//Origin--Client--NetPromoterWidget");
    private static final By ORIGIN_TAG_LOCATOR = By.xpath("//Origin--UIToolkit--OriginWebView");
    private static final String OIG_WINDOW_URL = "qtwidget://Origin::Engine::IGOQWidget";
    private static final By REDEEM_COMPLETE_DIALOG_LOCATOR = By.xpath("//Origin--UIToolkit--OriginWindow[@id='RedeemCodeWindow']");
    private static final By OIG_HELP_WINDOW_LOCATOR = By.xpath("//CustomerSupportDialog");
    private static final String OIG_OFFLINE_MODE_INDICATOR_WINDOW_URL = "qtwidget://Origin::Client::IGOOfflineIndicator";
    private static final By ADD_GAME_WINDOW_LOCATOR = By.xpath("//Origin--UIToolkit--OriginWindow[@name='Add Game' and @id='OriginWindow_MsgBox']");
    private static final By OIG_BROWSER_LOCATOR = By.xpath("//Origin--Client--IGOWebBrowser");
    private static final By PROMO_TAG_LOCATOR = By.xpath("//Origin--Client--PromoView");
    private static final String OIG_NAVIGATION_WINDOW_URL_REGEX = "widget://oig";
    private static final By REDEEM_TAG_LOCATOR = By.xpath("//RedeemBrowser");
    private static final By PROBLEM_LAUNCHING_GAME_WINDOW_LOCATOR = By.xpath("//Origin--UIToolkit--OriginWindow[@name='Problem Launching Game' and @id='OriginWindow_MsgBox']");
    private static final By ORIGIN_BODY_LOCATOR = By.cssSelector("body.otk");

    /**
     * This widget URL is the same for most Origin windows such as the main
     * Origin window, the code redemption window, and the promo manager window
     */
    private static final String ORIGIN_WINDOW_URL = "qtwidget://Origin::UIToolkit::OriginWindow";
    private static final By LOGIN_TAG_LOCATOR = By.xpath("//Origin--UIToolkit--OriginWindow[@id='OriginWindow' and @className='Origin::UIToolkit::OriginWindow']");
    private static final By OIG_OFFLINE_MODE_INDICATOR_TAG_LOCATOR = By.xpath("//Origin--Client--IGOOfflineIndicator");
    private static final By REMOVE_GAME_WINDOW_LOCATOR = By.xpath("//Origin--UIToolkit--OriginWindow[@name='Origin' and @id='OriginWindow_MsgBox']");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public EAXVxSiteTemplate(WebDriver driver) {
        super(driver);
    }

    /**
     * Origin is part of the url of all pages in site.
     *
     * @return 'origin'
     */
    @Override
    public String getExpectedUrl() {
        return "origin";
    }

    /**
     * Return null to check url instead of title
     *
     * @return null
     */
    @Override
    public String getExpectedTitle() {
        return null;
    }

    /**
     * Check if the given frame is active.
     *
     * @param frameNameOrId The frame name or the frame ID to check
     * @return The WebDriver
     */
    public WebDriver checkIfFrameIsActive(WebElement frameNameOrId) {
        try {
            return driver.switchTo().frame(frameNameOrId);
        } catch (NoSuchFrameException e) {
            return null;
        }
    }

    /**
     * Wait for the OIG 'Offline Mode Message Box' window and switch to it.
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToOigOfflineModeMsgBox(WebDriver driver) {
        Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, OIG_WINDOW_URL, OIG_OFFLINE_MODE_MSGBOX_LOCATOR, 0);
    }

    /**
     * Wait for the 'Remove Game' message box and switch the driver to that
     * window.
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToRemoveGameMsgBox(WebDriver driver) {
        Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, ORIGIN_WINDOW_URL, REMOVE_GAME_WINDOW_LOCATOR, 0);
    }

    /**
     * Waits for a window that has the 'OIG Help' element and switches to that
     * window.
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToOigHelpWindow(WebDriver driver) {
        Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, OIG_WINDOW_URL, OIG_HELP_WINDOW_LOCATOR, 40);
    }

    /**
     * Waits for a window that has the 'Download Game' element and switches the
     * driver to that window.
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToDownloadGameMsgBox(WebDriver driver) {
        Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, ORIGIN_WINDOW_URL, DOWNLOAD_GAME_WINDOW_LOCATOR, 0);
    }

    /**
     * Waits for a window that has the 'Promotion Manager' element and switches
     * the driver to that window.
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToPromoWindow(WebDriver driver) {
        Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, ORIGIN_WINDOW_URL, PROMO_TAG_LOCATOR, 5);
    }

    /**
     * Waits for a window that has the 'OIG Browser' element and switches to
     * that window.
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToOigWebBrowserWindow(WebDriver driver) {
        Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, OIG_WINDOW_URL, OIG_BROWSER_LOCATOR, 40);
    }

    /**
     * Waits for a window that has the 'OriginWebView' element and switches to
     * that window.
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToOigDownloadManagerWindow(WebDriver driver) {
        Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, OIG_WINDOW_URL, OIG_DOWNLOAD_MANAGER_WINDOW_LOCATOR, 40);
    }

    /**
     * Waits for the main SPA window and switches to it.
     *
     * @param driver The driver to switch
     */
    public static void switchToSPAWindow(@Nonnull WebDriver driver) {
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60);
    }

    /**
     * Wait for OIG 'Offline Mode Indicator' window (consists of a link button)
     * and switch to it.
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToOigOfflineModeIndicator(WebDriver driver) {
        Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, OIG_OFFLINE_MODE_INDICATOR_WINDOW_URL, OIG_OFFLINE_MODE_INDICATOR_TAG_LOCATOR, 0);
    }

    /**
     * Waits for the 'Login' window and switches the driver to it, to allow access to
     * the Qt outer frame of the 'Login' page.
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToLoginWindow(WebDriver driver) {
        Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, ORIGIN_WINDOW_URL, LOGIN_TAG_LOCATOR, 0);
    }

    /**
     * Wait for the 'Redeem Complete' dialog and switch driver to that window.
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToRedeemCompleteDialog(WebDriver driver) {
        Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, ORIGIN_WINDOW_URL, REDEEM_COMPLETE_DIALOG_LOCATOR, 0);
    }

    /**
     * Switch the driver to the main Origin window that has the menu bar. This
     * must be done before you can interact with the menu items (Origin, Games,
     * Friends, Help).
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToOriginWindow(WebDriver driver) {
        Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, ORIGIN_WINDOW_URL, ORIGIN_TAG_LOCATOR, 0);
    }

    /**
     * Waits for a window with title that starts with 'OIG' and switches the
     * driver to that window.
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToOigNavigationWindow(WebDriver driver) {
        Waits.waitForPageThatStartsWith(driver, OIG_NAVIGATION_WINDOW_URL_REGEX, 60);
    }

    /**
     * Waits for a window that has the 'Redeem Code' element and switches the
     * driver to that window.
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToRedeemCodeWindow(WebDriver driver) {
        Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, ORIGIN_WINDOW_URL, REDEEM_TAG_LOCATOR, 0);
    }

    /**
     * Wait for the 'Add Game' message box and switch the driver to that window.
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToAddGameWindow(WebDriver driver) {
        Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, ORIGIN_WINDOW_URL, ADD_GAME_WINDOW_LOCATOR, 0);
    }

    /**
     * Wait for the 'Problem Launching Game' message box and switch the driver
     * to that window.
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToProblemLaunchingGameMsgBox(WebDriver driver) {
        Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, ORIGIN_WINDOW_URL, PROBLEM_LAUNCHING_GAME_WINDOW_LOCATOR, 0);
    }

    /**
     * Waits for a window that has the 'Net Promoter' element and switches the
     * driver to that window.
     *
     * @param driver Selenium WebDriver (cannot be null)
     */
    public static void switchToNetPromoterWindow(WebDriver driver) {
        Waits.waitForWindowWithURLThatEqualsAndHasElement(driver, ORIGIN_WINDOW_URL, NET_PROMOTER_TAG_LOCATOR, 0);
    }

    /**
     * Wait for a given frame and switches to it.
     *
     * @param frameNameOrId The frame name or the frame ID to switch to
     * @throws TimeoutException
     */
    public void waitForFrameAndSwitchToIt(final WebElement frameNameOrId)
            throws TimeoutException {
        if (!checkIfFrameIsActive(frameNameOrId).equals(null)) {
            return;
        }
        pageWait(0).
                until(ExpectedConditions.frameToBeAvailableAndSwitchToIt(frameNameOrId));
    }

    /**
     * Check if WebElement found using a By locator is visible.
     *
     * @param alternateDriver WebDriver driver
     * @param locator         The locator for searching WebElement
     * @return true if WebElement is found, false otherwise
     */
    protected boolean verifyElementVisible(WebDriver alternateDriver, By locator) {
        try {
            final List<WebElement> elements = alternateDriver.findElements(locator);
            return !elements.isEmpty() && elements.get(0).isDisplayed();
        } catch (StaleElementReferenceException e) {
            return false;
        }
    }

    /**
     * Switch the driver to the main Origin window that has the menu bar. This
     * must be done before you can interact with the menu items (Origin, Games,
     * Friends, Help).
     */
    protected void switchToOriginWindow() {
        _log.debug("switching to Origin window");
        switchToOriginWindow(driver);
        _log.debug("page URL: " + driver.getCurrentUrl());
        _log.debug("page title: " + driver.getTitle());
    }

    /**
    * Switches to the first window with a given url with a given WebElement
    * Throws a runtime exception if the WebElement is not contained in any of the windows
    *
    * @param driver Selenium WebDriver
    * @param url the url of the window
    * @param webElement the By of the WebElement to search for in the new window
    */
    protected void switchToWindowWithUrlThatHasElement(WebDriver driver, String url, By webElement) {
        Set<String> windowHandles = driver.getWindowHandles();
        boolean isElementPresent = false;
        for (String w : windowHandles) {
            try {
                driver.switchTo().window(w);
                if (driver.findElement(webElement) != null) {
                    isElementPresent = true;
                    break;
                }
            } catch (WebDriverException e) {
            }
        }
        if (!isElementPresent) {
            throw new RuntimeException("WebElement not present in any window with url " + url);
        }
    }

    /**
     * Returns a Dimension object specifying the height and width of the main Origin client window
     *
     * @param driver Selenium WebDriver
     * @return Dimension of the Origin client's main window
     */
    public static Dimension getOriginClientWindowSize(WebDriver driver) {
        return driver.findElement(ORIGIN_BODY_LOCATOR).getSize();
    }

    /**
     * Returns the X coordinate of a web element
     * 
     * @param w the web element
     * @return an int which represents the X coordinate
     */
    public static int getXCoordinate(WebElement w) {
         return w.getLocation().getX();
    }
    
    /**
     * Returns the Y coordinate of a web element
     * 
     * @param w the web element
     * @return an int which represents the Y coordinate
     */
    public static int getYCoordinate(WebElement w) {
        return w.getLocation().getY();
    }
    
    /**
     * Returns the color of a web element
     * 
     * @param w the web element
     * @return a String representing the code of the color
     */
    public String getColorOfElement(WebElement w) {
        return getColorOfElement(w, "color");
    }

    /**
     * Returns a random WebElement from a given list
     *
     * @param elements List of WebElements
     * @return random WebElement
     */
    public static WebElement getRandomWebElement(List<WebElement> elements) {
        Random random = new Random();
        int size = elements.size();

        return elements.get(random.nextInt(size));
    }

    /**
     * Returns the color of a web element
     *
     * @param w the web element
     * @param cssValue the cssValue to get the color from
     * @return a String representing the code of the color
     */
    public String getColorOfElement(WebElement w, String cssValue) {
        return convertRGBAtoHexColor(waitForElementVisible(w).getCssValue(cssValue));
    }
    
    /**
     * Scroll down the page manually so the navigation bar show up
     *
     * @param verticalOffsetValue indicate how much the page will be scrolled
     * down. The page will scroll down with verticalOffsetValue
     * multiple with 100 from the window y position (keep in mind that)
     */
    public void scrollDownThePageManually(int verticalOffsetValue) {
        for(int i=0; i<100; i++) {
            scrollByVerticalOffset(verticalOffsetValue);
        }
    }

    /**
     * Verifies that an URL returns the expected response code
     *
     * @param url URL to get the response code
     * @return true if the url code matches the expected code, false otherwise
     */
    public static boolean verifyUrlResponseCode(String url) {
        try {
            return RestfulHelper.getResponseCode(url) == 200;
        } catch (IOException ex) {
            return false;
        }

    }
    
    /**
     * Clicks a button (with retries) and verifies that an element is no longer visible.
     *
     * @param buttonLocator the locator of the button to click
     * @param locatorToClose the element to check that is no longer after a successful click
     */
    protected void clickButtonWithRetries(By buttonLocator, By locatorToClose) {
        for (int i = 0; i < 5; i++) {
            try {
                waitForElementClickable(buttonLocator, 3).click();
            } catch (TimeoutException e) {
                _log.debug("Click generated a TimeoutException on attempt: " + i);
            } catch (NoSuchElementException e) {
                _log.debug("Click generated an ElementNotFoundException on attempt: " + i);
            }
            if (waitForElementNotVisible(locatorToClose, 3)) {
                return;
            }
            Waits.sleep(1000);
        }
        throw new RuntimeException("Function clickButtonWithRetries(...) failed");
    }

    /**
     * Waits for a Web Element to not be visible (can still be present)
     *
     * @param locatorToClose
     */
    protected boolean waitForElementNotVisible(By locatorToClose, int seconds) {
        try {
            return new WebDriverWait(driver, seconds).until(ExpectedConditions.invisibilityOfElementLocated(locatorToClose));
        } catch (TimeoutException e) {
            _log.debug("Locator still visible");
            return false;
        }
    }

    /**
     * Refresh the page and sleep in order to properly load page elements
     *
     * @param driver Selenium Web Driver
     */
    public static void refreshAndSleep(WebDriver driver) {
        driver.get(driver.getCurrentUrl());
        Waits.sleep(5000); // Wait for elements on the page to load
    }

    /**
     * Enter a string into a box character by character to avoid characters shifting or missing.
     * Uses the normal WebElement sendKeys command.
     *
     * @param inputLocator locator of the input tag
     * @param strToInput the string to enter into the box
     * @throws TimeoutException if the input box cannot be found or is not visible
     */
    public void inputStringByChar(By inputLocator, String strToInput) throws TimeoutException  {
        final WebElement inputElement = waitForElementClickable(inputLocator);
        inputElement.clear();
        // to avoid characters shifting or missing
        Arrays.stream(strToInput.split("")).forEach(c -> inputElement.sendKeys(c));
    }
}