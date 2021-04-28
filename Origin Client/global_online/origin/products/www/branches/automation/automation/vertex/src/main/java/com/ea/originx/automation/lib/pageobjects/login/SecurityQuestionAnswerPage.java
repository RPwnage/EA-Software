package com.ea.originx.automation.lib.pageobjects.login;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.utils.Waits;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.interactions.Actions;
import org.openqa.selenium.support.ui.Select;

/**
 * Page object that represents a registration page that has a security
 * question and answer details to be entered.
 *
 * @author mkalaivanan
 */
public class SecurityQuestionAnswerPage extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(CountryDoBRegisterPage.class);

    protected static final By SECURITY_QUESTION_LOCATOR = By.cssSelector("#security-question-container .otkselect #securityQuestion");
    protected static final By SECURITY_ANSWER_LOCATOR = By.cssSelector("#security-answer-container #securityAnswer");
    protected static final By CREATE_ACCOUNT_BUTTON_LOCATOR = By.cssSelector("#submitBtn");
    protected static final By CONTACT_ME_CHECKBOX_LOCATOR = By.cssSelector("#contactMe");
    protected static final By CONTACT_ME_LOCATOR = By.cssSelector("#contact-me-container .otkcheckbox");
    protected static final By EMAIL_VISIBILITY_LOCATOR = By.cssSelector("#email-visibility-container .otkcheckbox");
    protected static final By EMAIL_VISIBILITY_CHECKBOX_LOCATOR = By.cssSelector("#emailVisibility");
    protected static final By PROFILE_VISIBILITY_LOCATOR = By.cssSelector("#clientreg_friend_visibility");
    protected static final By PROFILE_VISIBILITY_SELECT_LOCATOR = By.cssSelector("#clientreg_friend_visibility #friend_visibility_selctrl");

    private static final String OPTION_WITH_TEXT_TMPL = "//option[contains(text(), '%s')]";

    private static final String DEFAULT_SECURITY_QUESTION = "What is the name of your favorite pet?";
    private static final String DEFAULT_SECURITY_ANSWER = "Kedi";
    public static final String DEFAULT_PRIVACY_VISIBLITY_SETTING = "Everyone";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public SecurityQuestionAnswerPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Complete the security question and answer.
     */
    public void inputSecurityQuestionAndAnswer() {
        _log.debug("entering security question");
        WebElement securityQuestion = waitForElementPresent(SECURITY_QUESTION_LOCATOR, 30);
        forceClickElement(securityQuestion);
        WebElement secQuestion = waitForChildElementPresent(securityQuestion, By.xpath(String.format(OPTION_WITH_TEXT_TMPL, DEFAULT_SECURITY_QUESTION)));
        waitForElementClickable(secQuestion);
        hoverOnElement(secQuestion);
        secQuestion.click();

        WebElement securityAnswer = waitForElementClickable(SECURITY_ANSWER_LOCATOR);
        _log.debug("entering answer");
        securityAnswer.sendKeys(OriginClientData.ACCOUNT_PRIVACY_SECURITY_ANSWER);
        sleep(2000); // adding this sleep to reduce the number of ajax requests at time in a page 
    }

    /**
     * Click on the 'Create Account' button.
     *
     */
    public void clickCreateAccount() {
        new Actions(driver).moveToElement(waitForElementPresent(CREATE_ACCOUNT_BUTTON_LOCATOR, 30)).click().perform();
    }

    /**
     * Wait for the page to load.
     */
    public void waitForPage() {
        Waits.waitForWindowThatHasElement(driver, SECURITY_QUESTION_LOCATOR, 25);
    }

    /**
     * Verify the 'Create Account' button is enabled.
     *
     * @return true if 'Create Account' button is enabled, false otherwise
     */
    public boolean isCreateAccountEnabled() {
        return !waitForElementVisible(CREATE_ACCOUNT_BUTTON_LOCATOR).getAttribute("class").contains("disabled");
    }

    /**
     * Complete the security question and answer with retry mechanism.
     *
     * @param checkFindByEmail Flag to uncheck the 'Email Visibility' checkbox
     * @param privacyVisibilitySetting the visibility settings to be selected
     * from dropdown
     */
    public void enterSecurityQuestionAnswerWithRetry(boolean checkFindByEmail, String privacyVisibilitySetting) {
        for (int i = 1; i <= 3; i++) {
            inputSecurityQuestionAndAnswer();
            if (!verifyProfileVisiblitySetting(DEFAULT_PRIVACY_VISIBLITY_SETTING)) {
                setProfileVisibilitySetting(privacyVisibilitySetting);
            }
            if (isEmailVisiblityIsSelected() != checkFindByEmail) {
                setFindByEmail(true);
            }
            if (isCreateAccountEnabled()) {
                break;
            }
            _log.debug("Security Question and Answered not filled properly and so retrying again:" + i);
        }
        clickCreateAccount();
    }

    /**
     * Complete the security question and answer with retry mechanism.
     */
    public void enterSecurityQuestionAnswerWithRetry() {
        enterSecurityQuestionAnswerWithRetry(false, DEFAULT_PRIVACY_VISIBLITY_SETTING);
    }

    /**
     * Verifies that the 'Email me about products, events, etc' is selected.
     *
     * @return true if the checkbox is checked
     */
    public boolean verifyEmailMeIsSelected() {
        return waitForElementPresent(CONTACT_ME_CHECKBOX_LOCATOR).isSelected();
    }

    /**
     * Uncheck the 'Email Visibility' checkbox.
     */
    public void unCheckEmailVisiblity() {
        setFindByEmail(false);
    }

    /**
     * Check or uncheck the 'Email Visibility' checkbox.
     *
     * @param setEmailVisiblity Flag to check/uncheck email visibility
     * checkbox
     */
    public void setFindByEmail(boolean setEmailVisiblity) {
        if (isEmailVisiblityIsSelected() != setEmailVisiblity) {
            waitForElementClickable(EMAIL_VISIBILITY_LOCATOR).click();
        }
    }

    /**
     * Selects profile visibility setting from dropdown
     *
     * @param visibilitySetting the visibility setting to be selected from
     * dropdown
     */
    public void setProfileVisibilitySetting(String visibilitySetting) {
        Select select = new Select(waitForElementPresent(PROFILE_VISIBILITY_SELECT_LOCATOR));
        select.selectByValue(visibilitySetting.toUpperCase());
    }

    /**
     * Check if the 'Email Visibility' checkbox is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyEmailVisiblityVisible() {
        return waitIsElementVisible(EMAIL_VISIBILITY_LOCATOR);
    }

    /**
     * Check if the security question is present.
     *
     * @return true if present, false otherwise
     */
    public boolean verifySecurityQuestionPresent() {
        return waitIsElementPresent(SECURITY_QUESTION_LOCATOR); // the select dropdown is present but not visible
    }

    /**
     * Check if the security answer is visible
     *
     * @return true if visible, false otherwise
     */
    public boolean verifySecurityAnswerVisible() {
        return waitIsElementVisible(SECURITY_ANSWER_LOCATOR); 
    }

    /**
     * Check if the 'Contact Me' checkbox is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyContactMeVisible() {
        return waitIsElementVisible(CONTACT_ME_LOCATOR);
    }

    /**
     * Verify the 'Profile Visibility' dropdown is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyProfileVisibilityVisible() {
        return waitIsElementVisible(PROFILE_VISIBILITY_LOCATOR);
    }

    /**
     * Get the current state of the 'Email Visibility' checkbox.
     *
     * @return {@code true} if checked, {@code false} if unchecked
     */
    public boolean isEmailVisiblityIsSelected() {
        WebElement emailVisiblityCheckBox = waitForElementPresent(EMAIL_VISIBILITY_CHECKBOX_LOCATOR);
        return emailVisiblityCheckBox.isSelected();
    }

    /**
     * Verify setting for 'Profile Visibility' dropdown.
     *
     * @param expectedProfileVisiblitySetting The expected 'Profile Visibility' setting
     * @return true if expected setting matches value of 'Profile Visibility'
     * checkbox, false otherwise
     */
    public boolean verifyProfileVisiblitySetting(String expectedProfileVisiblitySetting) {
        Select select = new Select(waitForElementPresent(PROFILE_VISIBILITY_SELECT_LOCATOR));
        WebElement option = select.getFirstSelectedOption();
        return option.getText().trim().equalsIgnoreCase(expectedProfileVisiblitySetting);
    }

    /**
     * Click the 'Back' button.
     */
    public BasicInfoRegisterPage goBack() {
        waitForElementClickable(By.cssSelector("#back")).click();
        BasicInfoRegisterPage basicInfoRegisterPage = new BasicInfoRegisterPage(driver);
        basicInfoRegisterPage.waitForBasicInfoPage();
        return basicInfoRegisterPage;
    }
}