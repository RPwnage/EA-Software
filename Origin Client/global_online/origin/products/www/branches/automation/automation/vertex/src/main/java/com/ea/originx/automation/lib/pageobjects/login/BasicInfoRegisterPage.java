package com.ea.originx.automation.lib.pageobjects.login;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.resources.CountryInfo;
import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.Dimension;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

import java.lang.invoke.MethodHandles;

/**
 * Page object that represents the 'Basic Information Registration' page that
 * has Origin ID, first name, last name, email ID, password, and captcha.
 *
 * @author mkalaivanan
 */
public class BasicInfoRegisterPage extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    protected static final By ORIGINID_LOCATOR = By.cssSelector("#reg-basic-info-panel #origin-id-container #originId");
    protected static final By FIRST_NAME_LOCATOR = By.cssSelector("#reg-basic-info-panel #first-name-container #firstName");
    protected static final By LAST_NAME_LOCATOR = By.cssSelector("#reg-basic-info-panel #last-name-container #lastName");
    protected static final By PARENT_EMAIL_LOCATOR = By.cssSelector("#reg-basic-info-panel #parent-email-container #parentEmail");
    protected static final By EMAIL_LOCATOR = By.cssSelector("#reg-basic-info-panel #email-container #email");
    protected static final By PASSWORD_LOCATOR = By.cssSelector("#reg-basic-info-panel #password-container #password");
    protected static final By CAPTCHA_LOCATOR = By.cssSelector("#captcha-container #recaptcha_response_field");
    protected static final By READ_ACCEPT_LOCATOR = By.cssSelector("#readAccept");
    protected static final By BASIC_INFO_CONTINUE_BUTTON_LOCATOR = By.cssSelector("#basicInfoNextBtn");

    protected static final By EMAIL_INVALID_MESSAGE_LOCATOR = By.cssSelector("#form-error-invalid-email");
    protected static final By EMAIL_DUPLICATE_MESSAGE_LOCATOR = By.cssSelector("#form-error-duplicate-email");
    protected static final By PUBLIC_ID_ALREADY_IN_USE_MESSAGE_LOCATOR = By.cssSelector("#form-error-origin-id-duplicate");
    protected static final By PUBLIC_ID_INVALID_MESSAGE_LOCATOR = By.cssSelector("#form-error-origin-id-invalid");
    protected static final By PASSWORD_INVALID_CHARACTER_MESSAGE_LOCATOR = By.cssSelector("#form-error-password-unsupported");
    protected static final By PASSWORD_INVALID_TOOLTIP_LOCATOR = By.cssSelector("div.password-tooltip.otktooltip-isvisible");
    private static final By BACK_LOCATOR = By.cssSelector("#back");

    protected static final String EMAIL_INVALID_MESSAGE = "Email address is invalid";
    private static final String PUBLIC_ID_ALREADY_IN_USE_MESSAGE = "The Public ID you have entered is already in use";
    private static final String PUBLIC_ID_INVALID = "ID contains a prohibited word or character";
    private static final String PASSWORD_INVALID_CHARACTER_MESSAGE = "Password contains an unsupported character";
    private static final String[] EMAIL_DUPLICATE_ADDRESS_MESSAGE = {"already", "registered", "email"};

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public BasicInfoRegisterPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Click on the 'Next' button on the 'User Detail' section.
     */
    public void clickOnBasicInfoNextButton() {
        _log.debug("clicking basic info next");
        forceClickElement(waitForElementClickable(BASIC_INFO_CONTINUE_BUTTON_LOCATOR));
    }

    /**
     * Complete registering the user with account specific information - email,
     * username, and password.
     *
     * @param userAccount The user account to input registration details for
     * @param isUnderAge  Whether the user is underage or not
     * @throws java.lang.InterruptedException
     */
    public void inputUserAccountRegistrationDetails(UserAccount userAccount, boolean isUnderAge, boolean isCountryRussia) throws InterruptedException {
        inputUserInfo(userAccount.getUsername(), userAccount.getEmail(), userAccount.getPassword(), userAccount.getFirstName(), userAccount.getLastName(), OriginClientData.CAPTCHA_AUTOMATION, isUnderAge, isCountryRussia);
    }

    /**
     * Enter user related information with retry mechanism ( this is needed as
     * sometimes registration fails as information is not properly filled).
     *
     * @param userAccount The user account to use
     * @throws InterruptedException
     */
    public void inputUserAccountRegistrationDetailsWithRetry(UserAccount userAccount) throws InterruptedException {
        boolean isCountryRussia;
        for (int i = 1; i <= 3; i++) {
            isCountryRussia = userAccount.getCountry().equals(CountryInfo.CountryEnum.RUSSIA.getCountry());
            inputUserAccountRegistrationDetails(userAccount, isUnderAge(), isCountryRussia);
            if (isBasicInfoButtonEnabled()) {
                break;
            }
            _log.debug("User Account details not entered properly and so retrying again:" + i);
        }
        clickOnBasicInfoNextButton();
    }

    /**
     * Enter user related information while registering as a new user.
     *
     * @param username      The username to use
     * @param userEmail     The user email ID to use
     * @param userPassword  The user password to use
     * @param userFirstName The user first name to use
     * @param userLastName  The user last name to use
     * @param captcha       The captcha to enter
     * @param isUnderAge    Whether the user is underage or not
     */
    public void inputUserInfo(String username, String userEmail, String userPassword, String userFirstName, String userLastName, String captcha, boolean isUnderAge, boolean isCountryRussia) {

        if (isUnderAge) {
            _log.debug("entering parent email");
            inputUserDetail(PARENT_EMAIL_LOCATOR, userEmail);
        } else {
            _log.debug("entering email" + userEmail);
            inputEmailId(userEmail);
            sleep(2000); // adding this sleep to reduce the number of ajax requests at time in a page
        }
        _log.debug("entering password");
        inputPassword(userPassword);
        _log.debug("entering username" + username);
        inputPublicId(username);
        sleep(2000); // adding this sleep to reduce the number of ajax requests at time in a page

        if (!isUnderAge && !isCountryRussia) {
            _log.debug("entering first name");
            inputFirstName(userFirstName);
            _log.debug("entering last name");
            inputLastName(userLastName);
        }

        sleep(2000); // adding this sleep to reduce the number of ajax requests at time in a page
        _log.debug("entering captcha");
        inputCaptcha(captcha);

    }

    /**
     * Enter email ID.
     *
     * @param email The email ID of the user
     */
    public void inputEmailId(String email) {
        if (isUnderAge()) {
            inputUserDetail(PARENT_EMAIL_LOCATOR, email);
        } else {
            inputUserDetail(EMAIL_LOCATOR, email);
        }
    }

    /**
     * Enter password.
     *
     * @param password The password of the user
     */
    public void inputPassword(String password) {
        inputUserDetail(PASSWORD_LOCATOR, password);
    }

    /**
     * Enter public ID
     *
     * @param publicId The public ID of the user
     */
    public void inputPublicId(String publicId) {
        inputUserDetail(ORIGINID_LOCATOR, publicId);
    }

    /**
     * Enter the first name of the user.
     *
     * @param firstName The first name of the user
     */
    public void inputFirstName(String firstName) {
        inputUserDetail(FIRST_NAME_LOCATOR, firstName);
    }

    /**
     * Enter last name.
     *
     * @param lastName The last name of the user
     */
    public void inputLastName(String lastName) {
        inputUserDetail(LAST_NAME_LOCATOR, lastName);
    }

    /**
     * Input the captcha to register as a new user.
     *
     * @param captcha The captcha to use
     */
    public void inputCaptcha(String captcha) {
        inputUserDetail(CAPTCHA_LOCATOR, captcha);
    }

    /**
     * Input 'User Detail' into the 'Input' WebElement field with the passed CSS
     * locator value.
     *
     * @param cssLocator Locator for the input field
     * @param inputValue Value for the input field
     */
    public void inputUserDetail(By cssLocator, String inputValue) {
        WebElement userField = waitForElementClickable(cssLocator);
        userField.clear();
        userField.sendKeys(inputValue);
    }

    /**
     * Verify 'Basic Information Registration' page is reached
     *
     * @return true if 'Basic Information Registration' page reached, false
     * otherwise
     */
    public boolean verifyBasicInfoPageReached() {
        return isElementVisible(EMAIL_LOCATOR);
    }

    /**
     * Determine if the registration is for an underage user.
     *
     * @return true if the registration if for an underage user, false otherwise
     */
    public boolean isUnderAge() {
        final WebElement emailElement = waitForElementPresent(EMAIL_LOCATOR);
        final Dimension dimension = emailElement.getSize();
        _log.debug(dimension.width + "x" + dimension.height);
        final boolean isUnderAge = !(emailElement.isEnabled() && dimension.height > 0 && dimension.width > 0);
        _log.debug("underage? " + isUnderAge);
        return isUnderAge;
    }

    /**
     * Wait for the 'Basic Info' page to load.
     */
    public void waitForBasicInfoPage() {
        waitForPageToLoad();
        waitForJQueryAJAXComplete();
        waitForElementVisible(BACK_LOCATOR); // make sure page is displayed
    }

    /**
     * Press the 'Back' button.
     *
     * @return A CountryDoBRegisterPage
     */
    public CountryDoBRegisterPage goBack() {
        waitForElementClickable(BACK_LOCATOR).click();
        CountryDoBRegisterPage countryDoBRegisterPage = new CountryDoBRegisterPage(driver);
        countryDoBRegisterPage.verifyCountryInputVisible();
        return countryDoBRegisterPage;
    }

    /**
     * Verify the 'Basic Info' button is enabled.
     *
     * @return true if the button is enabled, false otherwise
     */
    public boolean isBasicInfoButtonEnabled() {
        return !waitForElementVisible(BASIC_INFO_CONTINUE_BUTTON_LOCATOR).getAttribute("class").contains("disabled");
    }

    /**
     * Verify that the invalid email message is displayed.
     *
     * @return true if the invalid email message is displayed, false otherwise
     */
    public boolean verifyEmailInvalidMessageVisible() {
        try {
            String message = waitForElementVisible(EMAIL_INVALID_MESSAGE_LOCATOR).getText();
            return StringUtils.containsIgnoreCase(message, EMAIL_INVALID_MESSAGE);
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify that the duplicate email message is displayed.
     *
     * @return true if the duplicate email message is displayed, false otherwise
     */
    public boolean verifyDuplicateEmailMessageVisible() {
        try {
            String message = waitForElementVisible(EMAIL_DUPLICATE_MESSAGE_LOCATOR).getText();
            return StringHelper.containsIgnoreCase(message, EMAIL_DUPLICATE_ADDRESS_MESSAGE);
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify 'Public ID already in use' message is displayed.
     *
     * @return true if the message is displayed, false otherwise
     */
    public boolean verifyPublicIdAlreadyInUseMessageVisible() {
        try {
            String message = waitForElementVisible(PUBLIC_ID_ALREADY_IN_USE_MESSAGE_LOCATOR).getText();
            return StringUtils.containsIgnoreCase(message, PUBLIC_ID_ALREADY_IN_USE_MESSAGE);
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify all the passwords that are passed as parameter are invalid
     *
     * @param passwords the values that will be checked
     * @return true if the passwords are invalid, false otherwise
     */
    public boolean verifyAllPasswordsAreInvalid(String... passwords) {
        for (String password : passwords) {
            inputPassword(password);
            if (!verifyInvalidPasswordTooltipVisible())
                return false;
        }
        return true;
    }

    /**
     * Verify 'ID contains a prohibited word or character' message is displayed.
     *
     * @return true if the message is displayed, false otherwise
     */
    public boolean verifyPublicIdInvalidMessageVisible() {
        try {
            String message = waitForElementVisible(PUBLIC_ID_INVALID_MESSAGE_LOCATOR).getText();
            return StringUtils.containsIgnoreCase(message, PUBLIC_ID_INVALID);
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify that the 'Password contains an invalid character' message is
     * displayed
     *
     * @return true if the message is displayed, false otherwise
     */
    public boolean verifyPasswordInvalidCharacterMessageVisible() {
        try {
            String message = waitForElementVisible(PASSWORD_INVALID_CHARACTER_MESSAGE_LOCATOR).getText();
            return StringUtils.containsIgnoreCase(message, PASSWORD_INVALID_CHARACTER_MESSAGE);
        } catch (TimeoutException e) {
            return false;
        }
    }

    /**
     * Verify that the 'Invalid password message' is displayed.
     *
     * @return true if the message is displayed, false otherwise
     */
    public boolean verifyInvalidPasswordTooltipVisible() {
        return waitIsElementPresent(PASSWORD_INVALID_TOOLTIP_LOCATOR);
    }

    /**
     * Verify there is a field for entering a 'Parent Email Address'.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyParentEmailAddressVisible() {
        return waitIsElementVisible(PARENT_EMAIL_LOCATOR);
    }

    /**
     * Verify the 'Password' field is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyPasswordVisible() {
        return waitIsElementVisible(PASSWORD_LOCATOR);
    }

    /**
     * Verify there is a field for entering a Public ID.
     *
     * @return true if a field for entering a public ID is visible, false
     * otherwise
     */
    public boolean verifyPublicIdVisible() {
        return waitIsElementVisible(ORIGINID_LOCATOR);
    }

    /**
     * Verify 'Email' field is visible.
     *
     * @return true if the 'Email' field is visible, false otherwise
     */
    public boolean verifyEmailVisible() {
        return waitIsElementVisible(EMAIL_LOCATOR);
    }

    /**
     * Verify 'First Name' field is visible.
     *
     * @return true if a field for entering a first name is visible, false
     * otherwise
     */
    public boolean verifyFirstNameVisible() {
        return waitIsElementVisible(FIRST_NAME_LOCATOR);
    }

    /**
     * Verify 'Last Name' field is visible.
     *
     * @return true if a field for entering a last name is visible, false
     * otherwise
     */
    public boolean verifyLastNameVisible() {
        return waitIsElementVisible(LAST_NAME_LOCATOR);
    }

    /**
     * Verify 'Captcha' field is visible.
     *
     * @return true if a field for entering a captcha is visible, false
     * otherwise
     */
    public boolean verifyCaptchaVisible() {
        return waitIsElementVisible(CAPTCHA_LOCATOR);
    }
}
