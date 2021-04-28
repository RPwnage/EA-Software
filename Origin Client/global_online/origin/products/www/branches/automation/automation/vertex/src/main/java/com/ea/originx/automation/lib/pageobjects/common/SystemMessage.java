package com.ea.originx.automation.lib.pageobjects.common;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebDriverException;
import org.openqa.selenium.WebElement;

/**
 * A class to handle system-level messages and dialogs, like the site stripe.
 *
 * @author rmcneil
 */
public class SystemMessage extends EAXVxSiteTemplate {

    protected static final By SITE_STRIPE_LOCATOR = By.cssSelector(".origin-sitestripe .otknotice-stripe-message");
    protected static final By SITE_STRIPE_CLOSE_LOCATOR = By.cssSelector(".origin-sitestripe .otkicon.otkicon-closecircle");
    protected final String[] SITE_STRIPE_MESSAGE_KEYWORDS = {"origin", "access", "expired"};
    
    public SystemMessage(WebDriver driver) {
        super(driver);
    }

    /**
     * Get the 'Renew membership' link as WebElement
     * 
     * @return the link as WebElement
     */
    public WebElement getRenewMembershipLink() {
        return waitForElementVisible(SITE_STRIPE_LOCATOR).findElement(By.cssSelector("a"));
    }
    
    /**
     * Close the site stripe
     */
    public void closeSiteStripe() {
        waitForElementClickable(SITE_STRIPE_CLOSE_LOCATOR).click();
    }
    
    public boolean tryClosingSiteStripe() {
        try {
            waitForElementClickable(SITE_STRIPE_CLOSE_LOCATOR, 2).click();
            return true;
        } catch(WebDriverException e) {
            return false;
        }
    }

    /**
     * Check if the site stripe is visible
     *
     * @return true if the site stripe is visible, false otherwise
     */
    public boolean isSiteStripeVisible() {
        return waitIsElementVisible(SITE_STRIPE_LOCATOR, 2);
    }
    
    /**
     * Verify the banner has a message similar to 'Your Origin Access membership
     * has expired'
     *
     * @return true if the banner is visible and contains keywords, false
     * otherwise
     */
    public boolean verifyBannerMessageVisible() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(SITE_STRIPE_LOCATOR).getText(), SITE_STRIPE_MESSAGE_KEYWORDS);
    }
    
    /**
     * Verify the banner has a 'Renew Membership' link visible
     * 
     * @return true if the link is visible, false otherwise
     */
    public boolean verifyRenewMembershipLinkVisible() {
        return isElementVisible(getRenewMembershipLink());
    }
    
    /**
     * Clicks 'Renew Membership' link
     */
    public void clickRenewMembershipLink() {
        waitForElementClickable(getRenewMembershipLink()).click();
    }
}
