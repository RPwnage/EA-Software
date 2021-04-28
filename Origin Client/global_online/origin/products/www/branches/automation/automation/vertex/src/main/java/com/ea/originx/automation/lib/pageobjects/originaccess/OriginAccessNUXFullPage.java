package com.ea.originx.automation.lib.pageobjects.originaccess;

import com.ea.originx.automation.lib.helpers.DateHelper;
import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;


/**
 * Represents the 'Origin Access NUX Full' page which loads once a user purchases origin access
 * and loads after the 'Origin Access Intro Dialog' is closed
 *
 * @author nvarthakavi
 */
public class OriginAccessNUXFullPage extends EAXVxSiteTemplate {

    protected static final By ORIGIN_ACCESS_NUX_FULL_PAGE_LOCATOR = By.cssSelector(".origin-store-access-nux-fullpage-wrapper-content");
    protected static final By SKIP_NUX_GO_TO_VAULT_LINK_LOCATOR = By.cssSelector(".origin-store-access-nux-fullpage-gameintro-skip");
    protected static final By SKIP_TO_ORIGIN_ACCESS_COLLECTION = By.cssSelector(".origin-store-access-nux-fullpage-gameintro a");
    protected static final By PREMIER_SKIP_NUX_GO_TO_VAULT_LINK_LOCATOR = By.cssSelector(".origin-store-access-nux-fullpage-gameintro-skip");
    protected static final By PREMIER_SKIP_TO_ORIGIN_ACCESS_COLLECTION = By.cssSelector(".origin-store-access-nux-fullpage-gameintro-game .otkbtn-nux");
    protected static final By MEMBERSHIP_INFORMATION_BILLING_CYCLE = By.cssSelector("origin-store-access-nux-fullpage-gameintro li[ng-bind-html*='billingDate']");
    protected static final String BILLING_CYCLE_YEARLY_KEYWORD = "annually";
    protected static final String BILLING_CYCLE_MONTHLY_KEYWORD = "monthly";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OriginAccessNUXFullPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'Origin Access Full NUX' page is displayed.
     *
     * @return true if page displayed, false otherwise
     */
    public boolean verifyPageReached() {
        return waitIsElementVisible(ORIGIN_ACCESS_NUX_FULL_PAGE_LOCATOR);
    }

    /**
     * Click on the 'Go to the Vault Instead' link to navigate to the 'Vault Page'
     */
    public void clickSkipNuxGoToVaultLink() {
        waitForElementClickable(SKIP_NUX_GO_TO_VAULT_LINK_LOCATOR).click();
    }

    /**
     * Click on the 'Check out Origin Access game collection' link
     *
     */
    public void clickSkipToOriginAccessCollection() {
        waitForElementClickable(SKIP_TO_ORIGIN_ACCESS_COLLECTION).click();
    }
    
    
    /**
     * Click on the 'Go to the Vault Instead' link to navigate to the 'Vault Page'
     */
    public void clickSkipNuxGoToVaultLinkPremierSubscriber() {
        waitForAnimationComplete(PREMIER_SKIP_NUX_GO_TO_VAULT_LINK_LOCATOR);
        waitForElementClickable(PREMIER_SKIP_NUX_GO_TO_VAULT_LINK_LOCATOR).click();
    }

    /**
     * Click on the 'Check out Origin Access game collection' link
     *
     */
    public void clickSkipToOriginAccessCollectionPremierSubscriber() {
        waitForElementClickable(PREMIER_SKIP_TO_ORIGIN_ACCESS_COLLECTION).click();
    }

    /**
     * Verifies the 'Billing' section mentions plan type
     *
     * @return true if the keyword is found, false otherwise
     */
    public boolean verifyBillingMonthly() {
        return StringHelper.containsIgnoreCase(getMembershipInformationBillingText(), BILLING_CYCLE_MONTHLY_KEYWORD);
    }

    /**
     * Verifies the 'Billing' section mentions plan type
     *
     * @return true if the keyword is found, false otherwise
     */
    public boolean verifyBillingYearly() {
        return StringHelper.containsIgnoreCase(getMembershipInformationBillingText(), BILLING_CYCLE_YEARLY_KEYWORD);
    }

    /**
     * Gets the 'Billing' section text
     *
     * @return 'Billing' section text
     */
    public String getMembershipInformationBillingText() {
        return waitForElementVisible(MEMBERSHIP_INFORMATION_BILLING_CYCLE).getText();
    }

    /**
     * Verifies the next monthly billing date in 'Billing' section
     *
     * @return true if the next month is correct, false otherwise
     */
    public boolean verifyNextBillingMonthlyDate() {
        return StringHelper.containsIgnoreCase(getMembershipInformationBillingText(), DateHelper.getNextMonthNameGMT());
    }

    /**
     * Verifies the next yearly billing date in 'Billing' section
     *
     * @return true if the next billing date is correct, false otherwise
     */
    public boolean verifyNextBillingYearlyDate() {
        return StringHelper.containsIgnoreCase(getMembershipInformationBillingText(), DateHelper.getDatePlusOneYearGMT(), DateHelper.getMonthNameGMT());
    }
}
