package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import java.lang.invoke.MethodHandles;
import java.util.List;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.apache.logging.log4j.LogManager;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Dialog for the 'The Vault' in the Origin Access NUX.
 *
 * @author glivingstone
 */
public class AccessIntroVault extends Dialog {

    private static final org.apache.logging.log4j.Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    protected static final By THE_VAULT_LOCATOR = By.cssSelector(".otkmodal-content .origin-store-access-nux-vault");
    protected static final By THE_VAULT_TITLE_LOCATOR = By.cssSelector(".otkmodal-content .origin-store-access-nux-vault > h2");
    protected static final By INTRO_NEXT_BUTTON_LOCATOR_ = By.cssSelector(".otkmodal-content .origin-store-access-nux-vault-cta");
    protected static final By TRIAL_NEXT_BUTTON_LOCATOR = By.cssSelector(".otkmodal-content .origin-store-access-nux-trials-cta");
    protected static final By DISCOUNT_CLOSE_BUTTON_LOCATOR = By.cssSelector(".otkmodal-content .origin-store-access-nux-discount-cta");

    // Game tiles to add to the 'Game Library'
    protected static final By GAME_TILE_LOCATOR = By.cssSelector(".otkmodal-content .origin-store-carousel-carousel-slidearea > li");
    protected static final By GAME_TILE_TITLE_LOCATOR = By.cssSelector(".origin-storegametile-details .origin-storegametile-title span");
    protected static final By GAME_TILE_OFFER_ID_LOCATOR = By.cssSelector(".origin-storegametile origin-cta-directacquisition");
    protected static final By GET_IT_NOW_LOCATOR = By.cssSelector(".origin-storegametile .origin-storecta-btn button.otkbtn-primary");

    protected static final By TRIAL_TITLE_LOCATOR = By.cssSelector(".origin-store-access-nux-trials > h2");
    protected static final By TRIAL_IMAGE_BANNER_LOCATOR = By.cssSelector(".otkmodal-content .origin-store-access-nux-trials-banner-image");
    protected static final By ORIGIN_ACCESS_DISCOUNT_TITLE = By.cssSelector(".otkmodal-content .origin-store-access-nux-discount h2 b");

    protected static final String[] origin_Access_Discount = {"%", "save"};
    protected static final By ORIGIN_DISCOUNT_BANNER_IMAGE = By.cssSelector(".otkmodal-content .origin-store-access-nux-discount-banner-image");

    // titles for dialog
    protected static String[] theVaultDialogTitle = {"The", "Vault"};
    protected static String[] playFirstTrialsDialogTitle = {"Play", "First", "Trials"};

    /**
     * Constructor
     *
     * @param driver the Selenium WebDriver for the client
     */
    public AccessIntroVault(WebDriver driver) {
        super(driver, THE_VAULT_LOCATOR, THE_VAULT_TITLE_LOCATOR);
    }

    /**
     * Select 'Get It Now' on any entitlement that appears on the page. Will try
     * each tile, until one succeeds.
     *
     * @return String array containing the offer ID and the title of the
     * product, which can be used to drive the entitlement, or null if it could
     * not click on any entitlement
     */
    public String[] addAnyEntitlementToVault() {
        List<WebElement> allEntitlements = driver.findElements(GAME_TILE_LOCATOR);
        for (WebElement entitlement : allEntitlements) {
            try {
                String offerID = entitlement.findElement(GAME_TILE_OFFER_ID_LOCATOR).getAttribute("offer-id");
                String title = entitlement.findElement(GAME_TILE_TITLE_LOCATOR).getText();
                waitForChildElementPresent(entitlement, GET_IT_NOW_LOCATOR).click();
                return new String[]{offerID, title};
            } catch (Exception e) {
                _log.error("Failed to Click on a Title, trying the next.");
            }
        }
        return null;
    }

    /**
     * Check if title for this dialog exists.
     *
     * @return true if exists, false otherwise
     */
    public boolean verifyTitleForDialog() {
        return StringHelper.containsAnyIgnoreCase(waitForElementVisible(THE_VAULT_TITLE_LOCATOR).getText(), theVaultDialogTitle);
    }

    /**
     * Check if first five visible vault titles exist.
     *
     * @return true if exist, false otherwise
     */
    public boolean verifyVaultTitles() {
        return driver.findElements(GAME_TILE_TITLE_LOCATOR)
                .stream().limit(5).allMatch(e->waitIsElementVisible(e) && !e.getText().isEmpty());
    }

    /**
     * Click 'Next' button from intro screen.
     */
    public void clickIntroNextButton() {
        waitForElementClickable(INTRO_NEXT_BUTTON_LOCATOR_).click();
    }

    /**
     * Click 'Next' button from intro trial screen.
     */
    public void clickTrialNextButton() {
        waitForElementClickable(TRIAL_NEXT_BUTTON_LOCATOR).click();
    }

    /**
     * Click 'Close' button from discount screen.
     */
    public void clickDiscountCloseButton() {
        waitForElementClickable(DISCOUNT_CLOSE_BUTTON_LOCATOR).click();
    }

    /**
     * Check if title for trial exists.
     *
     * @return true if exists, false otherwise
     */
    public boolean verifyTrialTitle() {
        return StringHelper.containsAnyIgnoreCase(waitForElementVisible(TRIAL_TITLE_LOCATOR).getText(), playFirstTrialsDialogTitle);
    }

    /**
     * Check if trial banner image exists.
     *
     * @return true if exist, false otherwise
     */
    public boolean verifyTrialBannerImage() {
        return StringHelper.containsAnyIgnoreCase(waitForElementVisible(TRIAL_IMAGE_BANNER_LOCATOR).getAttribute("src"), "http");
    }

    /**
     * Check if discount banner image exists.
     *
     * @return true if exist, false otherwise
     */
    public boolean verifyDiscountBannerImage() {
        return StringHelper.containsAnyIgnoreCase(waitForElementVisible(ORIGIN_DISCOUNT_BANNER_IMAGE).getAttribute("src"), "http");
    }

    /**
     * Check if discount exists from title.
     *
     * @return true if exist, false otherwise
     */
    public boolean verifyDiscountFromTitle() {
        return StringHelper.containsAnyIgnoreCase(waitForElementVisible(ORIGIN_ACCESS_DISCOUNT_TITLE).getText(), origin_Access_Discount);
    }
}