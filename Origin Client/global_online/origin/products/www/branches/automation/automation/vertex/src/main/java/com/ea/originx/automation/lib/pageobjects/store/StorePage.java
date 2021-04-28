package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.util.List;
import java.util.stream.Collectors;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

/**
 * The store page, chosen using the navigation on the left side.
 *
 * @author glivingstone
 * @author palui
 */
public class StorePage extends EAXVxSiteTemplate {

    protected final String STORE_PAGE = "#content #storewrapper";
    protected final By STORE_ALL_IMAGES_LOCATOR = By.cssSelector(STORE_PAGE + " img[src]");
    protected final By STORE_ONLINE_STORECONTENT_LOCATOR = By.cssSelector("origin-store-online #storecontent");
    protected final String STORE_OFFLINE_CTA_CSS = "origin-store-offline .origin-offline-cta";
    protected final By STORE_OFFLINE_TITLE_LOCATOR = By.cssSelector(STORE_OFFLINE_CTA_CSS + " .origin-message-title");
    protected final By STORE_OFFLINE_MESSAGE_LOCATOR = By.cssSelector(STORE_OFFLINE_CTA_CSS + " .origin-message-content");
    protected final By STORE_GO_ONLINE_BUTTON_LOCATOR = By.cssSelector(STORE_OFFLINE_CTA_CSS + " .origin-offline-reconnectbutton-button[ng-click='goOnline()']");

    protected static final String EXPECTED_OFFLINE_TITLE = "You're Offline";
    protected static final String[] EXPECTED_OFFLINE_MESSAGE_KEYWORDS = {"go", "online"};

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    public StorePage(WebDriver driver) {
        super(driver);
    }

    /**
     * Wait for the store page to load.
     */
    public void waitForStorePageToLoad() {
        this.waitForPageToLoad();
        this.waitForAngularHttpComplete();
        waitForElementVisible(By.cssSelector("#footer"));
        waitForAnimationComplete(By.cssSelector("#footer"));
    }

    /**
     * Verify the store page exists.
     *
     * @return true if the store page is active, false otherwise
     */
    public boolean verifyStorePageReached() {
        return waitIsElementVisible(By.cssSelector(STORE_PAGE));
    }

    /**
     * Verify the store page is offline, false otherwise.
     *
     * @return true if the offline message title text is visible and is as
     * expected, false otherwise
     */
    public boolean verifyStorePageOffline() {
        return getOfflineTitleText().equalsIgnoreCase(EXPECTED_OFFLINE_TITLE);
    }

    /**
     * Returns the offline store title text
     *
     * @return offline store title text
     */
    public String getOfflineTitleText() {
        return waitForElementVisible(STORE_OFFLINE_TITLE_LOCATOR, 5).getText();
    }

    /**
     * Verify the store page offline message is as expected, false otherwise.
     *
     * @return true if the offline message content is visible and matches the
     * expected keywords (case ignored), false otherwise
     */
    public boolean verifyStorePageOfflineMessage() {
        String message = waitForElementVisible(STORE_OFFLINE_MESSAGE_LOCATOR, 5).getText();
        return StringHelper.containsIgnoreCase(message, EXPECTED_OFFLINE_MESSAGE_KEYWORDS);
    }

    /**
     * Verify the store page is online.
     *
     * @return true if the store content is visible, false otherwise
     */
    public boolean verifyStorePageOnline() {
        return waitIsElementVisible(STORE_ONLINE_STORECONTENT_LOCATOR, 5);
    }

    /**
     * Click 'Go Online' button at the 'Store' page.
     */
    public void clickGoOnline() {
        waitForElementClickable(STORE_GO_ONLINE_BUTTON_LOCATOR, 5).click();
    }

    /**
     * Check if any of the images do not exist.
     *
     * @return List with image URL if found, empty list otherwise
     */
    public List<String> verifyAllImagesExist() {
        List<String> imageNotFoundList = driver.findElements(STORE_ALL_IMAGES_LOCATOR)
                .stream()
                .filter(element -> {
                    String sourceUrl = element.getAttribute("src");
                    try {
                        return TestScriptHelper.verifyBrokenLink(sourceUrl);
                    } catch (IOException e) {
                        _log.error(e);
                    }
                    return false;
                })
                .map(element -> element.getAttribute("src")).collect(Collectors.toList());
        return imageNotFoundList;
    }
}