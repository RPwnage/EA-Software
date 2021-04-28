package com.ea.originx.automation.lib.pageobjects.account;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.originx.automation.lib.pageobjects.dialog.SecurityQuestionDialog;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.vx.originclient.resources.OSInfo;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represent the 'Account Settings' page (i.e. 'EA Account and Privacy' page)
 * which opens in a browser. Each page has a unique URL (common to all users)
 * depending on the page section selected. Each page section can be selected
 * using the navigation links (to the left of the page content)
 *
 * @author jmittertreiner
 * @author palui
 * @author micwang
 */
public class AccountSettingsPage extends EAXVxSiteTemplate {

    // Navigation Side Bar
    protected static final String NAV_LINK_CSS_TMPL = "#nav_%s > div > div";

    // Index Page URL templates
    protected static final String INDEX_PAGE_URL_TMPL = "https://myaccount.preprod.ea.com/cp-ui/%s/index?env=%s";
    protected static final String PRODUCTION_INDEX_PAGE_URL_TMPL = "https://myaccount.ea.com/cp-ui/%s/index";

    // Page title (of the form "My Account: " + linkLabel)
    protected static final By PAGE_TITLE_LOCATOR = By.cssSelector("h1.page_title");
    protected static final String PAGE_TITLE_PREFIX = "My Account:";

    // Section context locator templates - note 'Redeem Product Code' section does not use .context but #redeemproduct
    protected static final String SECTION_CONTEXT_CSS_TMPL = ".context #NavLink .select #nav_%s";
    protected static final String REDEEM_CONTEXT_CSS_TMPL = "#RightContainer #%s.%s";

    // Login Page
    private static final By EMAIL_FIELD_LOCATOR = By.id("email");
    private static final By PASSWORD_FIELD_LOCATOR = By.id("password");
    private static final By LOGIN_BUTTON_LOCATOR = By.id("btnLogin");
    private static final By LOGOUT_BUTTON_LOCATOR = By.id("logout");

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * NavLinkType enum (also represents the section type) for the navigation
     * side bar and the different Strings in each section
     */
    public enum NavLinkType {
        ABOUT_ME("About Me", "aboutme", "aboutme", "aboutme"),
        SECURITY("Security", "security", "security", "security"),
        PRIVACY_SETTINGS("Privacy Settings", "privacy", "privacy", "privacy"),
        ORIGIN_ACCESS("Origin Access", "subscription", "subscription", "subscription"),
        ORDER_HISTORY("Order History", "orderhistory", "orderhistory", "orderhistory"),
        PAYMENT_AND_SHIPPING("Payment Methods", "paymentandshipping", "paymentandshipping", "paymentandshipping"),
        REDEEM_PRODUCT_CODE("Redeem Product Code", "redeemproduct", "redemption", "redeemproduct"),
        GAME_TESTER_PROGRAM("Game Tester Program", "gamebetas", "gamebetas", "gamebetas"),
        MY_GIFTS("My Gifts", "mygifts", "mygifts", "mygifts");

        private final String linkLabel;
        private final String linkIdSuffix;
        private final String UrlId;
        private final String contextId;

        /**
         * Constructor
         *
         * @param linkLabel navigation link label. Also used as (trimmed) page
         * title suffix for each section
         * @param linkIdSuffix suffix of the navigation link's id attribute
         * @param UrlId URL id for the index page of each section
         * @param contextId top-level id to locate the top-level element
         * containing the section content
         */
        NavLinkType(String linkLabel, String linkIdSuffix, String UrlId, String contextId) {
            this.linkLabel = linkLabel;
            this.linkIdSuffix = linkIdSuffix;
            this.UrlId = UrlId;
            this.contextId = contextId;
        }

        /**
         * Get navigation link locator (on the navigation sidebar) for this link
         * type
         *
         * @return navigation link locator
         */
        private By getLinkLocator() {
            return By.cssSelector(String.format(NAV_LINK_CSS_TMPL, linkIdSuffix));
        }

        /**
         * Get link label for this link type
         *
         * @return link label string
         */
        private String getLinkLabel() {
            return linkLabel;
        }

        /**
         * Get title suffix for this link type / section
         *
         * @return title suffix (same as link label) string
         */
        private String getTitleSuffix() {
            return getLinkLabel();
        }

        /**
         * Get locator for the context div for this link type / section
         *
         * @return context locator
         */
        private By getContextLocator() {
            // Redeem section uses #redeemptcode instead of .context
            if (this == NavLinkType.REDEEM_PRODUCT_CODE) {
                return By.cssSelector(String.format(REDEEM_CONTEXT_CSS_TMPL, contextId, contextId));
            } else {
                return By.cssSelector(String.format(SECTION_CONTEXT_CSS_TMPL, contextId));
            }
        }

        /**
         * Get URL to the index page for this link type / section
         *
         * @return link locator
         */
        private String getIndexURL() {
            // Append environment to the end to navigate back to the correct one
            if (OSInfo.isProduction()) {
                return String.format(PRODUCTION_INDEX_PAGE_URL_TMPL, UrlId);
            } else {
                String environment = OSInfo.getEnvironment().toLowerCase();
                if (environment.contains("sb")) {
                    environment = "dev";
                }
                return String.format(INDEX_PAGE_URL_TMPL, UrlId, environment);
            }
        }
    }

    /**
     * Constructor
     *
     * @param webDriver Selenium WebDriver
     */
    public AccountSettingsPage(WebDriver webDriver) {
        super(webDriver);
    }

    /**
     * Navigate to the default section (about me) index page
     */
    public void navigateToIndexPage() {
        navigateToIndexPage(NavLinkType.ABOUT_ME);
    }

    /**
     * Navigate to the My Gifts section (about me) index page
     */
    public void navigateToMyGiftsPage() {
        navigateToIndexPage(NavLinkType.MY_GIFTS);
    }

    /**
     * Navigate to a specific section's index page
     *
     * @param section the {@link NavLinkType} representing the section
     */
    public void navigateToIndexPage(NavLinkType section) {
        driver.get(section.getIndexURL());
    }

    /**
     * Verify the 'Account Settings' page is open (any Section is fine)
     *
     * @return true if the driver is on the accounts page, else false
     */
    public boolean verifyAccountSettingsPageReached() {
        return waitIsElementVisible(PAGE_TITLE_LOCATOR)
                && StringHelper.containsIgnoreCase(getPageTitle(), PAGE_TITLE_PREFIX);
    }

    /**
     * Verify 'Account Settings' page is open at the specified section
     *
     * @param section the {@link NavLinkType} representing the section
     * @return true if the specified section of 'Account Settings' page is open,
     * false otherwise
     */
    public boolean verifySectionReached(NavLinkType section) {
        return verifyPageTitle(section) && verifySectionContext(section);
    }

    /**
     * Get page title
     *
     * @return page title which should start with "My Account:"
     */
    public String getPageTitle() {
        return waitForElementVisible(PAGE_TITLE_LOCATOR).getText();
    }

    /**
     * Verify page title matches the section
     *
     * @param section the {@link NavLinkType} representing the section
     * @return true if page title matches, false otherwise
     */
    public boolean verifyPageTitle(NavLinkType section) {
        String pageTitle = getPageTitle().trim();
        return pageTitle.equals(PAGE_TITLE_PREFIX + " " + section.getTitleSuffix());
    }

    /**
     * Verify section context (top level WebElement that contains the section)
     * is visible
     *
     * @param section the {@link NavLinkType} representing the section
     * @return true if section context is visible, false otherwise
     */
    public boolean verifySectionContext(NavLinkType section) {
        return waitIsElementVisible(section.getContextLocator());
    }

    ///////////////////////////////////////////
    // Navigation links for opening section
    ///////////////////////////////////////////
    /**
     * Navigate to the section specified by navLink
     *
     * @param navLink section to navigate to
     */
    private void gotoSection(NavLinkType navLink) {
        waitForElementVisible(navLink.getLinkLocator()).click();
        try {
            Thread.sleep(2000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    /**
     * Navigate to the 'About Me' section
     */
    public void gotoAboutMeSection() {
        gotoSection(NavLinkType.ABOUT_ME);
    }

    /**
     * Navigate to the 'Security' section
     */
    public void gotoSecuritySection() {
        gotoSection(NavLinkType.SECURITY);
    }

    /**
     * Navigate to the 'Privacy Settings' section
     */
    public void gotoPrivacySettingsSection() {
        gotoSection(NavLinkType.PRIVACY_SETTINGS);
    }

    /**
     * Navigate to the 'Origin Access' (settings) section
     */
    public void gotoOriginAccessSection() {
        gotoSection(NavLinkType.ORIGIN_ACCESS);
    }

    /**
     * Navigate to the 'Order History' section
     */
    public void gotoOrderHistorySection() {
        gotoSection(NavLinkType.ORDER_HISTORY);
    }

    /**
     * Navigate to the 'Payment & shipping' section
     */
    public void gotoPaymentAndShippingSection() {
        gotoSection(NavLinkType.PAYMENT_AND_SHIPPING);
    }

    /**
     * Navigate to the 'Redeem Product Code' section
     */
    public void gotoRedeemProductCodeSection() {
        gotoSection(NavLinkType.REDEEM_PRODUCT_CODE);
    }

    /**
     * Navigate to the 'Game Tester Program' section
     */
    public void gotoGameTesterProgramSection() {
        gotoSection(NavLinkType.GAME_TESTER_PROGRAM);
    }

    /**
     * Navigate to the 'My Gifts' section
     */
    public void gotoMyGiftsSection() {
        gotoSection(NavLinkType.MY_GIFTS);
    }

    //////////////////////////////////////////////////
    // Login to 'Account Settings' Page
    //////////////////////////////////////////////////
    /**
     * Login to 'Account Settings' page. Default is to start at the 'About Me'
     * section. If navigating from Origin on the web, this shouldn't be needed
     * since your cookies should allow you to get in without logging in.
     *
     * @param account user account to login to
     */
    public void login(UserAccount account) {
        // Login to the page
        waitForElementVisible(EMAIL_FIELD_LOCATOR).clear();
        waitForElementVisible(EMAIL_FIELD_LOCATOR).sendKeys(account.getEmail());
        waitForElementVisible(PASSWORD_FIELD_LOCATOR).sendKeys(account.getPassword());
        waitForElementClickable(LOGIN_BUTTON_LOCATOR).click();
        waitForElementVisible(PAGE_TITLE_LOCATOR);
    }

    /**
     * Helper method to handle the security dialog pop up
     */
    public void answerSecurityQuestion() {
        final SecurityQuestionDialog securityQuestionDialog = new SecurityQuestionDialog(driver);
        securityQuestionDialog.waitForVisible();
        securityQuestionDialog.enterSecurityQuestionAnswer();
        securityQuestionDialog.clickContinue();
    }

    //////////////////////////////////////////////////
    // Logout from 'Account Settings' Page
    //////////////////////////////////////////////////
    /**
     * Logout from the 'Account Settngs' page
     */
    public void logout() {
        scrollToElement(waitForElementVisible(LOGOUT_BUTTON_LOCATOR)); // scroll necessary after clicking update bottom at the bottom of the screen
        waitForElementClickable(LOGOUT_BUTTON_LOCATOR).click();
    }
}