package com.ea.originx.automation.lib.pageobjects.store;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object that represents the 'Product Pack' in the 'Store' page.
 * This is most often a 6 or 9 pack with three columns.
 *
 * @author glivingstone
 */
public class ProductPack extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(ProductPack.class);

    protected final String PRODUCT_PACK = "#content #storecontent origin-store-pack[type='%s'] .origin-store-pack";
    protected final String PACK_TITLE = PRODUCT_PACK + " .origin-store-pack-title";
    protected final String PACK_ITEM = PRODUCT_PACK + " .l-origin-store-pack-item";
    protected final String ITEM_PRICE = PACK_ITEM + " .origin-storeprice .otkprice";
    protected final String ITEM_IMAGE = PACK_ITEM + " .origin-storegametile-posterart";
    protected final String ITEM_ORIGIN_ACCESS_PRICE = ".origin-storeprice-orprice";

    private final String storePackType;
    private final By productPackLocator;
    private final By packTitleLocator;
    private final By packItemLocator;
    private final By itemPriceLocator;
    private final By itemImageLocator;

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     * @param storePackType The type of store pack
     */
    public ProductPack(WebDriver driver, String storePackType) {
        super(driver);
        this.storePackType = storePackType;
        this.productPackLocator = By.cssSelector(String.format(PRODUCT_PACK, storePackType));
        this.packTitleLocator = By.cssSelector(String.format(PACK_TITLE, storePackType));
        this.packItemLocator = By.cssSelector(String.format(PACK_ITEM, storePackType));
        this.itemPriceLocator = By.cssSelector(String.format(ITEM_PRICE, storePackType));
        this.itemImageLocator = By.cssSelector(String.format(ITEM_IMAGE, storePackType));
    }

    /**
     * Verify the 'Product Pack' exists on the page.
     *
     * @return true if the 'Product Pack' exists, false otherwise
     */
    public boolean verifyProductPack() {
        return !waitForAllElementsVisible(productPackLocator).isEmpty();
    }

    /**
     * Verify the title on the 'Product Pack' is correct.
     *
     * @param title The title we expect to appear
     * @return true if the title given matches the title on the page, false
     * otherwise
     */
    public boolean verifyTitleExists(String title) {
        WebElement packTitle = waitForElementPresent(packTitleLocator);
        return packTitle.getText().contains(title);
    }

    /**
     * Verify the pack has the correct number of items in it.
     *
     * @param packSize The number of items we expect there to be
     * @return true if the pack size matches the expected size, false
     * otherwise
     */
    public boolean verifyPackSize(int packSize) {
       return waitForAllElementsVisible(packItemLocator).size() == packSize;
    }

    /**
     * Verify the prices and images displayed on all the tiles in the pack.
     *
     * @return true if all the items in the pack have both prices and images,
     * false if any tile is missing any of those items
     */
    public boolean verifyItemsHavePricesAndImages() {
        
        return verifyItemsHavePrices() && verifyItemsHaveImages();
    }

    /**
     * Verify the prices exist on all the tiles in the pack.
     *
     * @return true if all the items in the pack have prices, false if any of
     * the prices are missing
     */
    public boolean verifyItemsHavePrices() {
        List<WebElement> allPrices = waitForAllElementsVisible(itemPriceLocator);
        boolean allHavePrices = true;
        for (WebElement price : allPrices) {
          String productPrice = price.getText();
          if(productPrice.equals("0"))
          {
              WebElement productPack = price.findElement(By.xpath(".."));
              String originAccessPrice = productPack.findElement(By.cssSelector(ITEM_ORIGIN_ACCESS_PRICE)).getText();
              _log.debug("Origin Access Price"+ originAccessPrice);
              productPrice = originAccessPrice;
              
          }
          if (!productPrice.contains("$")) {
                allHavePrices = false;
                break;
           }
        }
        return allHavePrices && !allPrices.isEmpty();
    }

    /**
     * Verify the images exist on all the tiles in the pack.
     *
     * @return true if all the items in the pack have prices, false if any of
     * the prices are missing
     */
    public boolean verifyItemsHaveImages() {
        List<WebElement> allImages = waitForAllElementsVisible(itemImageLocator);
        boolean allHaveImages = true;
      
        for (WebElement image : allImages) {
            if (image.getAttribute("src").equals("") || image.getAttribute("src") == null) {
                allHaveImages = false;
                break;
            }
        }
        return allHaveImages && !allImages.isEmpty();
    }
}