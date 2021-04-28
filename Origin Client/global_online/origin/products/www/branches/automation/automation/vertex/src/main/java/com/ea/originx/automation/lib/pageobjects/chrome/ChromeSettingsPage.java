package com.ea.originx.automation.lib.pageobjects.chrome;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.*;

/**
 * Represents Chrome Settings page and Chrome Downloads page objects
 *
 * @author cbalea
 */
public class ChromeSettingsPage extends EAXVxSiteTemplate{
    
    protected static final String originSetupLabel = "OriginThinSetup";
    protected static final String chromeDownloadsPage = "chrome://downloads/";
    
    // Chrome Downloads page
    protected static final By DONWLOADS_MANAGER_SELECTOR = By.tagName("downloads-manager");
    protected static final By DOWNLOAD_ITEMS_SELECTOR = By.cssSelector("downloads-item");
    protected static final By DOWNLOAD_ITEM_TITLE_AREA_SELECTOR = By.cssSelector("#title-area");
    
    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ChromeSettingsPage(WebDriver driver){
        super(driver);
    }
    
    /**
     * Gets the shadow root WebElement
     *
     * @param element
     * @return the expanded shadow root element
     */
    private WebElement expandRootElement(WebElement element) {
        WebElement rootElement = (WebElement) ((JavascriptExecutor) driver)
                .executeScript("return arguments[0].shadowRoot", element);
        return rootElement;
    }
    
    /**
     * Verifies the download of 'Origin Client' setup has begun by going to Chrome 'Downloads' page and checking for the 'OriginThinSetup' file there
     *
     * @return true if the downloaded file name matches
     */
    public boolean verifyOriginSetupDownload() {
        driver.get(chromeDownloadsPage);
        WebElement downloadsManager = waitForElementVisible(DONWLOADS_MANAGER_SELECTOR);
        WebElement expandDownloadManagerShadowRoot = expandRootElement(downloadsManager);
        String downloadItemName = "";
        try {
            WebElement downloadsItem =  waitForChildElementPresent(expandDownloadManagerShadowRoot,DOWNLOAD_ITEMS_SELECTOR);
            WebElement downloadListShadowRootElement = expandRootElement(downloadsItem);
            downloadItemName =  waitForChildElementPresent(downloadListShadowRootElement,DOWNLOAD_ITEM_TITLE_AREA_SELECTOR).getText();
        } catch (NoSuchElementException e) {
            return false;
        }
        return StringHelper.containsIgnoreCase(downloadItemName, originSetupLabel);
    }
}
