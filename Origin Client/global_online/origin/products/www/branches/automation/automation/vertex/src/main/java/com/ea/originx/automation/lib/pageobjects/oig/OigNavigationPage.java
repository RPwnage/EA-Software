package com.ea.originx.automation.lib.pageobjects.oig;

import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.helpers.RobotKeyboard;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.awt.AWTException;
import java.awt.event.KeyEvent;
import org.apache.logging.log4j.LogManager;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'OIG Navigation' page
 *
 * @author mkalaivanan
 */
public class OigNavigationPage extends EAXVxSiteTemplate {

    private static final org.apache.logging.log4j.Logger _log = LogManager.getLogger(OigNavigationPage.class);

    protected static final By WEB_BROWSER_LOCATOR = By.cssSelector("#oig-sidenav-browser");
    protected static final By BROADCAST_LOCATOR = By.cssSelector("#oig-sidenav-broadcast");
    protected static final By FRIENDS_LIST_LOCATOR = By.cssSelector(".oig-sidenav #oig-sidenav-friends");
    protected static final By ACHIEVEMENTS_LOCATOR = By.cssSelector("#oig-sidenav-achievements");
    protected static final By DOWNLOAD_MANAGER_LOCATOR = By.cssSelector("#oig-sidenav-download");
    protected static final By SETTINGS_LOCATOR = By.cssSelector("#oig-sidenav-settings");
    protected static final By HELP_LOCATOR = By.cssSelector("#oig-sidenav-help");

    private final WebDriver qtDriver;

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OigNavigationPage(WebDriver driver) {
        super(driver);
        qtDriver = OriginClient.getInstance(driver).getQtWebDriver();
    }

    /**
     * Switch to this dialog. This is required since it's wrapped in a Qt
     * window.
     */
    public void switchToOIG() {
        EAXVxSiteTemplate.switchToOigNavigationWindow(driver);
        driver.switchTo().defaultContent();
    }

    /**
     * Click on 'Web Browser'
     *
     * @throws java.lang.Exception
     */
    public void clickWebBrowser() throws Exception {
        clickOIGNavigationItem(WEB_BROWSER_LOCATOR);
        EAXVxSiteTemplate.switchToOigWebBrowserWindow(qtDriver);
    }

    /**
     * Click on 'Broadcast Gameplay'.
     */
    public void clickBroadcastGameplay() throws Exception {
        clickOIGNavigationItem(BROADCAST_LOCATOR);
    }

    /**
     * Click on 'Friends List'.
     *
     * @throws java.lang.Exception
     */
    public void clickFriendsList() throws Exception {
        clickOIGNavigationItem(FRIENDS_LIST_LOCATOR);
        Waits.waitForPageThatMatches(driver, ".*social-hub.*", 60);
    }

    /**
     * Click on 'Achievements List'.
     *
     * @throws java.awt.AWTException
     */
    public void clickAchievements() throws Exception {
        clickOIGNavigationItem(ACHIEVEMENTS_LOCATOR);
        Waits.waitForPageThatMatches(driver, ".*achievements.*", 60);
    }

    /**
     * Click on 'Download Manager'.
     *
     * @throws AWTException
     */
    public void clickDownloadManager() throws Exception {
        clickOIGNavigationItem(DOWNLOAD_MANAGER_LOCATOR);
        Waits.waitForPageThatMatches(driver, ".*oig-download-manager.*", 60);
    }

    /**
     * Click on 'Settings'.
     *
     * @throws java.lang.Exception
     */
    public void clickSettings() throws Exception {
        clickOIGNavigationItem(SETTINGS_LOCATOR);
        Waits.waitForPageThatMatches(driver, ".*settings.*", 60);
    }

    /**
     * Click on 'Help'.
     *
     * @throws java.lang.Exception
     */
    public void clickHelp() throws Exception {
        clickOIGNavigationItem(HELP_LOCATOR);
        EAXVxSiteTemplate.switchToOigHelpWindow(qtDriver);
    }

    /**
     * Click on a specific WebElement on the 'Navigation' page. The mouse needs to be
     * over the game window for some windows to be opened in the OIG instead of
     * the client. Therefore the Robot class is used to move the mouse to an
     * acceptable location.
     *
     * @param cssLocator The CSS locator on the 'Navigation' page
     */
    private void clickOIGNavigationItem(By cssLocator) throws Exception {
        if (!verifyOigNavigationVisible()) { // Only Press if it's not open
            new RobotKeyboard().pressAndReleaseKeys(client, KeyEvent.VK_SHIFT, KeyEvent.VK_F1);
        }
        waitForElementClickable(cssLocator, 10).click();
    }

    /**
     * Input the default keyboard command (F1+SHIFT) to open or close the OIG.
     * For some reason it has to be input two times to get it to open or close.
     *
     * @throws java.awt.AWTException
     */
    public void openCloseOIG() throws Exception {
        RobotKeyboard robotHandler = new RobotKeyboard();
        if (verifyOigNavigationVisible()) { // If it's already open, close it
            new RobotKeyboard().pressAndReleaseKeys(client, KeyEvent.VK_SHIFT, KeyEvent.VK_F1);
        }
        robotHandler.pressAndReleaseKeys(client, KeyEvent.VK_SHIFT, KeyEvent.VK_F1);
        sleep(2000);
        robotHandler.pressAndReleaseKeys(client, KeyEvent.VK_SHIFT, KeyEvent.VK_F1);
        _log.debug("OIG open/closed");
    }

    /**
     * Verify 'Oig Navigation' links are visible.
     *
     * @return true if web browser, broadcast, friends list, achievements, settings
     * and help links are all visible
     */
    public boolean verifyOigNavigationVisible() {
        return isElementVisible(WEB_BROWSER_LOCATOR)
                && isElementVisible(BROADCAST_LOCATOR)
                && isElementVisible(FRIENDS_LIST_LOCATOR)
                && isElementVisible(ACHIEVEMENTS_LOCATOR)
                && isElementVisible(SETTINGS_LOCATOR)
                && isElementVisible(HELP_LOCATOR);
    }
}