package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object that represents browse game tiles in 'Browse Games'
 *
 * @author tdhillon
 */
public class StoreHomeTile extends EAXVxSiteTemplate {

    private By rootElement;
    private static final String HOME_TILE_LOCATOR = "otkex_hometile[tile-header='%s']";
    protected By tileTitleLocator = By.cssSelector(".home-tile-header");
    protected By tileImageLocator = By.cssSelector("img");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public StoreHomeTile(WebDriver driver, String tileTitle) {
        super(driver);
        this.rootElement = By.cssSelector(String.format(HOME_TILE_LOCATOR, tileTitle));
    }

    private WebElement getRootElement(){
        return driver.findElement(rootElement);
    }

    private WebElement getTileTitleElement() {
        return driver.findElement(tileTitleLocator);
    }

    public boolean verifyTileTitleVisible() {
        return isElementVisible(getTileTitleElement());
    }

    public String getTileTitle() {
        return getTileTitleElement().getText();
    }

    private WebElement getTileImageElement() {
        return driver.findElement(tileImageLocator);
    }

    public boolean verifyTileImageVisible() {
        return isElementVisible(getTileImageElement());
    }

    public void clickOnTile() {
        waitForElementClickable(getTileImageElement()).click();
    }

}