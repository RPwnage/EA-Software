package com.ea.originx.automation.lib.pageobjects.gamelibrary;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.common.ContextMenu;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Page object representing an extra content game card.
 *
 * @author palui
 */
public class ExtraContentGameCard extends EAXVxSiteTemplate {

    private final By gameCardLocator;
    private final By titleLocator;
    private final By primaryButtonLocator;
    private final By primaryButtonTextLocator;
    private final By violatorTextLocator;
    private final By buyAsGiftButtonLocator;
    private final By buyButtonDropdownArrowLocator;
    private final ContextMenu buyDropdownMenu;
    private final By wishlistHeartIconLocator;
    private final By onWishlistTextLocator;
    private final By viewWishlistLinkLocator;

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     * @param offerId Offer ID string
     */
    public ExtraContentGameCard(WebDriver driver, String offerId) {
        super(driver);

        final String gameCardCss = ".origin-gamecard-item > [offer-id='" + offerId + "']";
        gameCardLocator = By.cssSelector(gameCardCss);
        titleLocator = By.cssSelector(gameCardCss + " .origin-gamecard .origin-gamecard-maintitle");
        primaryButtonLocator = By.cssSelector(gameCardCss + " .origin-gamecard-cta-container .otkbtn-primary");
        primaryButtonTextLocator = By.cssSelector(gameCardCss + " .origin-gamecard-cta-container .otkbtn-primary > .otkbtn-primary-text");
        violatorTextLocator = By.cssSelector(gameCardCss + " > div > section > div > div.origin-gamecard-violator-text");
        buyAsGiftButtonLocator = By.cssSelector(gameCardCss + " .origin-gamecard-gift-cta-container .origin-gifting-cta > .otkbtn");
        buyButtonDropdownArrowLocator = By.cssSelector(gameCardCss + " origin-store-contextmenu .otkbtn.otkbtn-dropdown-btn.otkbtn-primary > .otkicon-downarrow");
        buyDropdownMenu = new ContextMenu(driver, buyButtonDropdownArrowLocator, true); // left click
        wishlistHeartIconLocator = By.cssSelector(gameCardCss + " .origin-wishlist-on-wishlist .otkicon-heart");
        onWishlistTextLocator = By.cssSelector(gameCardCss + " .origin-wishlist-on-wishlist .origin-wishlist-on-wishlist-message > .origin-wishlist-on-wishlist-text");
        viewWishlistLinkLocator = By.cssSelector(gameCardCss + " .origin-wishlist-on-wishlist .origin-wishlist-on-wishlist-message > a");
    }

    /**
     * Verify if the game card is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean isGameCardVisible() {
        return isElementVisible(gameCardLocator);
    }

    /**
     * Verify (with waits) if game card is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyGameCardVisible() {
        return Waits.pollingWait(this::isGameCardVisible);
    }

    /**
     * Scroll to game card.
     */
    public void scrollToGameCard() {
        scrollToElement(waitForElementPresent(gameCardLocator));
        waitForAnimationComplete(gameCardLocator);
    }

    /**
     * Get the title of the game card.
     *
     * @return The game card's title
     */
    public String getTitle() {
        return waitForElementVisible(titleLocator).getText();
    }

    /**
     * Verify text of the CTA primary button matches the buttonLabel.
     *
     * @param buttonLabel Text to match
     *
     * @return true if matches, false otherwise
     */
    private boolean verifyPrimaryButtonVisible(String buttonLabel) {
        try {
            return waitIsElementVisible(primaryButtonLocator)
                    && waitForElementVisible(primaryButtonLocator).getText().equalsIgnoreCase(buttonLabel);
        }
        catch (TimeoutException ex){
            return false;
        }
    }

    /**
     * Verify if the 'Download' button is visible.
     *
     * @return true if button visible, false otherwise
     */
    public boolean verifyDownloadButtonVisible() {
        return verifyPrimaryButtonVisible("Download");
    }

    /**
     * Verify if the 'Details' button is visible.
     *
     * @return true if button visible, false otherwise
     */
    public boolean verifyDetailsButtonVisible() {
        return verifyPrimaryButtonVisible("Details");
    }

    /**
     * Verify if the 'Install Origin & Play' button is visible.
     *
     * @return true if specified button visible, false otherwise
     */
    public boolean verifyInstallOriginAndPlayButtonVisible() {
        return verifyPrimaryButtonVisible("Install Origin & Play");
    }

    /**
     * Verify if the 'Play' button is visible.
     *
     * @return true if specified button visible, false otherwise
     */
    public boolean verifyPlayButtonVisible() {
        return verifyPrimaryButtonVisible("Play");
    }

    /**
     * Verify the 'Buy' button is visible.
     *
     * @return true if button visible, false otherwise
     */
    public boolean verifyBuyButtonVisible() {
        return waitIsElementVisible(primaryButtonLocator)
                && StringHelper.containsIgnoreCase(waitForElementVisible(primaryButtonTextLocator).getText(), "Buy");
    }

    /**
     * Click the primary button.
     */
    public void clickPrimaryButton() {
        // Force click as the primary button may be behind "Chat (n)"
        forceClickElement(waitForElementPresent(primaryButtonLocator));
    }

    /**
     * Click the "Download" button.
     */
    public void clickDownloadButton() {
        clickPrimaryButton();
    }

    /**
     * Click the "Buy..." button.
     */
    public void clickBuyButton() {
        clickPrimaryButton();
    }

    /**
     * Click the 'Play' button.
     */
    public void clickPlayButton() {
        clickPrimaryButton();
    }

    /**
     * Verify if violator with the given text is visible.
     *
     * @return true if specified violator with matching text is visible, false
     * if not match, or throw TimeoutException if violator not visible
     */
    private boolean verifyViolatorVisible(String text) {
        return waitIsElementVisible(violatorTextLocator)
                && waitForElementVisible(violatorTextLocator).getText().equalsIgnoreCase(text);
    }

    /**
     * Verify the 'Installed' violator is visible.
     *
     * @return true if 'Installed' violator is visible, false if violator text
     * not match, or throw TimeoutException if violator not visible
     */
    public boolean verifyInstalledViolatorVisible() {
        return verifyViolatorVisible("Installed");
    }

    /**
     * Verify the 'Unlocked' violator is visible.
     *
     * @return true if 'Unlocked' violator is visible, false if violator text
     * not match, or throw TimeoutException if violator not visible
     */
    public boolean verifyUnlockedViolatorVisible() {
        return verifyViolatorVisible("Unlocked");
    }

    /**
     * Verify the 'Buy as Gift' button is visible.
     *
     * @return true if button is visible, false otherwise
     */
    public boolean verifyBuyAsGiftButtonVisible() {
        return waitIsElementVisible(buyAsGiftButtonLocator);
    }

    /**
     * Click 'Buy as Gift..." button if it is visible.
     */
    public void clickBuyAsGiftButton() {
        final WebElement e = waitForElementClickable(buyAsGiftButtonLocator);
        scrollElementIntoView(e);
        e.click();
    }

    /**
     * Select the 'Buy' dropdown menu item to add game to the wishlist.
     */
    public void selectBuyDropdownAddToWishlist() {
        buyDropdownMenu.selectItemContainingIgnoreCase("Add to wishlist");
    }

    /**
     * Select the 'Buy' dropdown menu item to remove game from the wishlist.
     */
    public void selectBuyDropdownRemoveFromWishlist() {
        buyDropdownMenu.selectItemContainingIgnoreCase("Remove from wishlist");
    }

    /**
     * Select the 'Buy' dropdown menu item to purchase game as a gift.
     */
    public void selectBuyDropdownPurchaseAsGift() {
        buyDropdownMenu.selectItemContainingIgnoreCase("Purchase as a gift");
    }

    /**
     * Select the 'Buy' dropdown menu item to buy game for self.
     */
    public void selectBuyDropdownBuyForSelf() {
        buyDropdownMenu.selectItemContainingIgnoreCase("Buy for myself");
    }

    /**
     * Verify the wishlist heart icon is visible.
     *
     * @return true if wishlist heart icon is visible, false otherwise
     */
    public boolean verifyWishlistHeartIconVisible() {
        return waitIsElementVisible(wishlistHeartIconLocator);
    }

    /**
     * Verify the 'On Wishlist' message is expected.
     *
     * @return true if 'On Wishlist' message text matches, false otherwise
     */
    public boolean verifyOnWishlistText() {
        String[] expectedKeywords = {"wishlist"};
        return StringHelper.containsIgnoreCase(waitForElementVisible(onWishlistTextLocator).getText(), expectedKeywords);
    }

    /**
     * Verify the 'View Wishlist' link text is as expected.
     *
     * @return true if 'View Wishlist' link text matches, false otherwise
     */
    public boolean verifyViewWishlistLinkText() {
        String[] expectedKeywords = {"wishlist"};
        return StringHelper.containsIgnoreCase(waitForElementVisible(viewWishlistLinkLocator).getText(), expectedKeywords);
    }

    /**
     * Click the 'View Wishlist' link.
     */
    public void clickViewWishlistLink() {
        waitForElementClickable(viewWishlistLinkLocator).click();
    }
}