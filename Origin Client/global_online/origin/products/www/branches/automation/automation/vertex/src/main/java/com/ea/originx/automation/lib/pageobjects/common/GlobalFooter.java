package com.ea.originx.automation.lib.pageobjects.common;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.vx.originclient.resources.LanguageInfo;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represent the 'Global Footer' page object that is common to all Origin pages
 * at the browser. This footer also appears at the store and PDP pages at the
 * client
 *
 * @author palui
 */
public class GlobalFooter extends EAXVxSiteTemplate {

    protected static final String GLOBAL_FOOTER_LINKLIST_CSS = "origin-globalfooter-base .origin-globalfooter-linklist";
    protected static final By ABOUT_GREAT_TIME_GUARANTEE_LINK_LOCATOR = By.cssSelector(GLOBAL_FOOTER_LINKLIST_CSS + " > li[link-text*='About Great Game Guarantee'] > a");
    protected static final By GREAT_GAME_GUARANTEE_POLICY_LINK_LOCATOR = By.cssSelector(GLOBAL_FOOTER_LINKLIST_CSS + " > li[link-text*='Great Game Guarantee Terms'] > a");
    protected static final By LEGAL_LINK_LOCATOR = By.cssSelector(GLOBAL_FOOTER_LINKLIST_CSS + " > li[link-text*='Legal'] > a");
    protected static final By TERMS_OF_SALES_LINK_LOCATOR = By.cssSelector(GLOBAL_FOOTER_LINKLIST_CSS + " > li[link-text*='Terms of Sale'] > a");
    protected static final By USER_AGREEMENT_LINK_LOCATOR = By.cssSelector(GLOBAL_FOOTER_LINKLIST_CSS + " > li[link-text*='User Agreement'] > a");
    protected static final By CORPORATE_INFORMATION_LINK_LOCATOR = By.cssSelector(GLOBAL_FOOTER_LINKLIST_CSS + " > li[link-text*='Corporate Information'] > a");
    protected static final By PRIVACY_COOKIE_POLICY_LINK_LOCATOR = By.cssSelector(GLOBAL_FOOTER_LINKLIST_CSS + " > li[link-text*='Privacy and Cookie Policy'] > a");

    protected static final String LANGUAGE_CSS = "origin-globalfooter-base .origin-globalfooter-languageselector";
    protected static final By LANGUAGE_LOCATOR = By.cssSelector(LANGUAGE_CSS);

    protected static final String CURRENT_LANGUAGE_CSS = LANGUAGE_CSS + " .otkselect-label .origin-globalfooter-languagelabel";
    protected static final By CURRENT_LANGUAGE_LOCATOR = By.cssSelector(CURRENT_LANGUAGE_CSS);

    protected static final String LANGUAGE_SELECTOR_CSS = LANGUAGE_CSS + " select";
    protected static final String LANGUAGE_OPTIONS_CSS = LANGUAGE_SELECTOR_CSS + " option";
    protected static final String LANGUAGE_OPTION_CSS_TMPL = LANGUAGE_OPTIONS_CSS + "[label='%s'][value='string:%s']";
    protected static final String SELECTED_LANGUAGE_CSS = LANGUAGE_OPTIONS_CSS + "[selected='selected']";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public GlobalFooter(WebDriver driver) {
        super(driver);
    }

    /**
     * Wait for the Global Footer to load by waiting for the Current Language to
     * be displayed
     */
    public void waitForGlobalFooterToLoad() {
        waitForElementVisible(CURRENT_LANGUAGE_LOCATOR);
    }

    /**
     * @return true if Global Footer visible, false otherwise
     */
    public boolean verifyGlobalFooterVisible() {
        return waitIsElementVisible(CURRENT_LANGUAGE_LOCATOR);
    }

    /**
     * Scroll to About Great game Guarantee link and click on it
     */
    public void scrollToClickAboutGreatGameGuaranteeLink() {
        scrollToElementAndClick(ABOUT_GREAT_TIME_GUARANTEE_LINK_LOCATOR);
    }

    /**
     * Scroll to Great Game Guarantee Policy link and click on it
     */
    public void scrollToClickGreatGameGuaranteePolicyLink() {

        scrollToElementAndClick(GREAT_GAME_GUARANTEE_POLICY_LINK_LOCATOR);
    }

    /**
     * Scroll to Legal Notices link and click on it
     */
    public void scrollToClickLegalNoticesLink() {
        scrollToElementAndClick(LEGAL_LINK_LOCATOR);
    }

    /**
     * Scroll to Terms of Sales link and click on it
     */
    public void scrollToClickTermsOfSalesLink() {

        scrollToElementAndClick(TERMS_OF_SALES_LINK_LOCATOR);
    }

    /**
     * Scroll to Terms of Service link and click on it
     */
    public void scrollToClickUserAgreementLink() {
        scrollToElementAndClick(USER_AGREEMENT_LINK_LOCATOR);
    }

    /**
     * Scroll to Corporate Information link and click on it
     */
    public void scrollToClickCorportateInformationLink() {
        scrollToElementAndClick(CORPORATE_INFORMATION_LINK_LOCATOR);
    }

    /**
     * Scroll to Privacy Cookie Policy link and click on it
     */
    public void scrollToClickPrivacyCookiePolicyLink() {
        scrollToElementAndClick(PRIVACY_COOKIE_POLICY_LINK_LOCATOR);
    }

    /**
     * Scroll to click element
     *
     * @param locator
     */
    private void scrollToElementAndClick(By locator) {
        scrollToElement(waitForElementVisible(locator));
        waitForElementClickable(locator).click();
    }

    /**
     * @return the displayed text for the 'Current Language' at this 'Global
     * Footer'
     */
    public String getCurrentLanguageText() {
        waitForGlobalFooterToLoad();
        return waitForElementVisible(CURRENT_LANGUAGE_LOCATOR).getAttribute("textContent").trim();
    }

    /**
     * @param languageInfo language to match
     * @return true if 'Current Language' at this 'Global Footer' matches the
     * local name of the language info, false otherwise
     */
    public boolean verifyCurrentLanguage(LanguageInfo languageInfo) {
        String localName = languageInfo.getLocalName();
        return localName.equalsIgnoreCase(getCurrentLanguageText());
    }

    /**
     * Scroll to the 'Current Language' displayed at this 'Global Footer'
     */
    public void scrollToCurrentLanguage() {
        scrollToElement(waitForElementVisible(CURRENT_LANGUAGE_LOCATOR));
    }

}
