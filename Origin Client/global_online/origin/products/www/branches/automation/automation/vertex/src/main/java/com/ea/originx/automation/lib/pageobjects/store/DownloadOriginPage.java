package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object that represents the 'Download Origin' page.
 *
 * @author mkalaivanan
 */
public class DownloadOriginPage extends EAXVxSiteTemplate {

    public static final By DOWNLOAD_ORIGIN_TILES_LOCATOR = By.cssSelector("#storecontent origin-store-about-tiles .origin-store-about-tile");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public DownloadOriginPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'Download Origin' tiles are visible.
     *
     * @return true if all size of the tiles elements is > 0, false otherwise
     */
    public boolean verifyDownloadOriginTileVisible() {
        List<WebElement> downloadOriginTiles = waitForAllElementsVisible(DOWNLOAD_ORIGIN_TILES_LOCATOR);
        return downloadOriginTiles.size() > 0;
    }
}