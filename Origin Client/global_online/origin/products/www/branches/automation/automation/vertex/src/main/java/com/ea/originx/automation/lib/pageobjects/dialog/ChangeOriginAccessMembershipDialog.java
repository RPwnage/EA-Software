package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Change your Origin Access membership' dialog
 *
 * @author mdobre
 */
public class ChangeOriginAccessMembershipDialog extends Dialog {

    private static final String CHANGE_ORIGIN_ACCESS_MEMBERSHIP_CSS = "origin-dialog-content-access-change-plan";
    private static final By CHANGE_ORIGIN_ACCESS_MEMBERSHIP_LOCATOR = By.cssSelector(CHANGE_ORIGIN_ACCESS_MEMBERSHIP_CSS);
    private static final By CLOSE_LOCATOR = By.cssSelector(".otkmodal-close");
    private static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(CHANGE_ORIGIN_ACCESS_MEMBERSHIP_CSS + " .otkbtn-primary");
    private static final By CHANGE_MEMBERSHIP_YEARLY_BILLING_INFO_LOCATOR = By.cssSelector(CHANGE_ORIGIN_ACCESS_MEMBERSHIP_CSS + " .origin-oa-scheduled-info .otktitle-4");
    private static final By CHANGE_MEMBERSHIP_BUTTON = By.xpath("//*[contains(text(), 'Change Membership')]");
    private static final By CHANGE_BILLING_BUTTON = By.xpath("//*[contains(text(), 'Change Billing')]");
    private static final String[] YEARLY_BILLING_INFO_KEYWORDS = {"billing", "change", "effect"};

    /**
     * Constructor
     *
     * @param driver WebDriver
     */
    public ChangeOriginAccessMembershipDialog(WebDriver driver) {
        super(driver, CHANGE_ORIGIN_ACCESS_MEMBERSHIP_LOCATOR, CLOSE_LOCATOR);
    }

    /**
     * Gets the billing info inside the yearly membership plan
     *
     * @return the billing info
     */
    public String getYearlyMembershipPlanBillingInfo() {
        return waitForElementVisible(CHANGE_MEMBERSHIP_YEARLY_BILLING_INFO_LOCATOR).getText();
    }

    public boolean verifyYearlyMembershipPlanBillingInfo() {
        return StringHelper.containsIgnoreCase(getYearlyMembershipPlanBillingInfo(), YEARLY_BILLING_INFO_KEYWORDS);
    }

    /**
     * Clicks the 'Close' CTA
     */
    public void clickCloseCTA() {
        waitForElementClickable(waitForElementVisible(CLOSE_BUTTON_LOCATOR)).click();
    }

    /**
     * Clicks the 'Change Membership' CTA
     */
    public void clickChangeMembershipCTA() {
        waitForElementClickable(waitForElementVisible(CHANGE_MEMBERSHIP_BUTTON)).click();
    }

    /**
     * Clicks the 'Change Billing' CTA
     */
    public void clickChangeBillingCTA() {
        waitForElementClickable(waitForElementVisible(CHANGE_BILLING_BUTTON)).click();
    }
}
