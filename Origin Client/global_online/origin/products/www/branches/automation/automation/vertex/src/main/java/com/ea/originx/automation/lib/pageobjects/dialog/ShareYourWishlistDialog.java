package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import java.util.Optional;

/**
 * Represents the 'Share Your Wishlist' dialog that pops up when a user clicks the 'Share Your Wishlist'
 * button in the wishlist tab.
 *
 * @author nvarthakavi
 */
public class ShareYourWishlistDialog extends Dialog {

    private static final By DIALOG_LOCATOR = By.cssSelector("origin-dialog-content-sharewishlist");
    private static final By TITLE_LOCATOR = By.cssSelector("otkmodal-header .origin-dialog-header");
    private static final By CLOSE_LOCATOR = By.cssSelector(".otkmodal-close");
    private static final String[] TITLE_KEYWORDS = {"share", "wishlist"};

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ShareYourWishlistDialog(WebDriver driver) {
        super(driver, DIALOG_LOCATOR, TITLE_LOCATOR, CLOSE_LOCATOR);
    }

    /**
     * Get the user's wishlist link.
     *
     * @return The link to the share the user's wishlist.
     */
    public Optional<String> getShareWishlistLink() {
        final By wishlistSelector = By.id("wishlistLink");
        // wait up to 3 seconds for wishlist link element to be present
        return waitIsElementPresent(wishlistSelector, 3)
                ? Optional.of(waitForElementVisible(wishlistSelector, 3).getText())
                : Optional.empty();
    }
}