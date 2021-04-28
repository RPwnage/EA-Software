package com.ea.originx.automation.lib.pageobjects.login;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.template.OAQtSiteTemplate;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.util.Arrays;
import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.Keys;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.interactions.Actions;

/**
 * A page object that represents the 'Login' page.
 *
 * @author tharan
 */
public class LoginPage extends EAXVxSiteTemplate {

    protected static final String SITE_STRIPE_MESSAGE_CSS = ".otknotice-stripe .otknotice-stripe-message .otkc strong";
    protected static final By SITE_STRIPE_MESSAGE_LOCATOR = By.cssSelector(SITE_STRIPE_MESSAGE_CSS);
    protected static final String[] SITE_STRIPE_OFFLINE_MESSAGE_KEYWORDS = {"Online", "login", "unavailable"};
    protected static final By LOGIN_PAGE_TITLE_LOCATOR = By.cssSelector("#loginBase h1");
    protected static final String[] LOGIN_PAGE_TITLE_KEYWORDS = {"sign", "in", "Account"};

    protected static final String LOGIN_FORM_CSS = "#login-form";
    protected static final By EMAIL_INPUT_LOCATOR = By.cssSelector(LOGIN_FORM_CSS + " #email");
    protected static final By PASSWORD_INPUT_LOCATOR = By.cssSelector(LOGIN_FORM_CSS + " #password");
    protected static final By PASSWORD_ERROR_LOCATOR = By.cssSelector(LOGIN_FORM_CSS + " .otkinput-iserror");
    protected static final By INVALID_CREDENTIALS_LOCATOR = By.cssSelector(LOGIN_FORM_CSS + " #online-general-error .otkinput-errormsg");
    protected static final By OFFLINE_ERROR_LOCATOR = By.cssSelector(LOGIN_FORM_CSS + " #offline-general-error .otkinput-errormsg");
    protected static final By LOGIN_BTN_LOCATOR = By.cssSelector(LOGIN_FORM_CSS + " #logInBtn");
    protected static final By LOGIN_BTN_DISABLED_LOCATOR = By.cssSelector(LOGIN_FORM_CSS + " .panel-action-area a.disabled");
    protected static final By BROWSER_LOGIN_BTN_LOCATOR = By.cssSelector(".origin-navigation .origin-nav origin-cta-login .otkbtn");
    protected static final By REMEMBER_ME_LOCATOR = By.cssSelector(LOGIN_FORM_CSS + " .panel-action-area #rememberMe");
    protected static final By LOGIN_INVISIBLE_LOCATOR = By.cssSelector(LOGIN_FORM_CSS + " .panel-action-area #loginInvisible");
    protected static final By REGISTRATION_LINK_LOCATOR = By.cssSelector("#createNav");
    protected static final By SIGN_IN_LINK_LOCATOR = By.cssSelector("#loginNav");
    protected static final By PARENT_EMAIL_NEXT_BTN_LOCATOR = By.cssSelector("#submit");
    protected static final By TITLE_LOCATOR = By.cssSelector(".otktitle.otktitle-2");
    protected static final By FORGOT_YOUR_PASSWORD_LOCATOR = By.cssSelector("#forgot-password-section .otka");

    // Two Factor Login Page - Client
    protected static final String TWO_FACTOR_LOGIN_VERIFICATION_GATE_CSS = "#loginForm #tfa-login #verificationGate";
    protected static final By TWO_FACTOR_LOGIN_FORM_TITLE_LOCATOR = By.cssSelector(TWO_FACTOR_LOGIN_VERIFICATION_GATE_CSS + " h1.otktitle");
    protected static final String TWO_FACTOR_LOGIN_FORM_EXPECTED_TITLE = "Login Verification";
    protected static final By TWO_FACTOR_LOGIN_FORM_SEND_VERIFICATION_CODE_LOCATOR = By.cssSelector(TWO_FACTOR_LOGIN_VERIFICATION_GATE_CSS + " .otkbtn#btnSendCode");
    protected static final By TWO_FACTOR_LOGIN_FORM_CODE_INPUT_LOCATOR = By.cssSelector(TWO_FACTOR_LOGIN_VERIFICATION_GATE_CSS + " .otkinput #twoFactorCode");
    protected static final By TWO_FACTOR_LOGIN_FORM_SIGN_IN_BUTTON_LOCATOR = By.cssSelector(TWO_FACTOR_LOGIN_VERIFICATION_GATE_CSS + " .otkbtn#btnSubmit");

    // Two Factor Login Page - Browser
    protected static final String TWO_FACTOR_LOGIN_WEB = ".origin-com .tfa-container #challengeForm";
    protected static final By TWO_FACTOR_LOGIN_WEB_TITLE_LOCATOR = By.cssSelector(TWO_FACTOR_LOGIN_WEB + " h1.twoStepHeader");
    protected static final By TWO_FACTOR_LOGIN_WEB_SEND_VERIFICATION_CODE_LOCATOR = By.cssSelector(TWO_FACTOR_LOGIN_WEB + " #btnSendCode");
    protected static final By TWO_FACTOR_LOGIN_WEB_CODE_INPUT_LOCATOR = By.cssSelector(TWO_FACTOR_LOGIN_WEB + " #origin-tfa-container #twofactorCode");
    protected static final By TWO_FACTOR_LOGIN_WEB_SIGN_IN_BUTTON_LOCATOR = By.cssSelector(TWO_FACTOR_LOGIN_WEB + " .tfa-form-container #btnTFAVerify");

    // Two Factor Login - App Authenticator Creation page - Browser only
    protected static final String TWO_FACTOR_LOGIN_CREATE = ".origin-com .tfa-create-container #challengeForm";
    protected static final By TWO_FACTOR_LOGIN_CRETAE_UPDATE_BUTTON_LOCATOR = By.cssSelector(TWO_FACTOR_LOGIN_CREATE + " .tfa-create-form-container #btnTFAUpdate");
    protected static final By TWO_FACTOR_LOGIN_CRETAE_NOT_NOW_BUTTON_LOCATOR = By.cssSelector(TWO_FACTOR_LOGIN_CREATE + " .tfa-create-form-container #btnCancel");

    // Underage parent
    protected static final String UNDERAGE_VERIFICATION_LOGIN_FORM_CSS = "#emailCheckView";
    protected static final By UNDERAGE_VERIFICATION_LOGIN_FORM_TITLE_LOCATOR = By.cssSelector(UNDERAGE_VERIFICATION_LOGIN_FORM_CSS + " h1.otktitle");
    protected static final String[] UNDERAGE_VERIFICATION_LOGIN_FORM_EXPECTED_TITLE_KEYWORDS = {"Parent", "guardian", "email", "verification"};
    protected static final By UNDERAGE_VERIFICATION_LOGIN_FORM_CONTINUE_BUTTON_LOCATOR = By.cssSelector(UNDERAGE_VERIFICATION_LOGIN_FORM_CSS + " #submit");
    protected static final By UNDERAGE_EMAIL_VERIFICATION_ERROR_MESSAGE_LOCATOR = By.cssSelector(UNDERAGE_VERIFICATION_LOGIN_FORM_CSS + " #loginUnderageVerification .otkinput-errormsg");
    protected static final String[] UNDERAGE_EMAIL_VERIFICATION_ERROR_MESSAGE_EXPECTED_TITLE_KEYWORDS = {"email", "not", "verified"};

    protected static final String[] INVALID_CREDENTIALS_KEYWORDS = {"credentials", "incorrect"};
    protected static final String[] INVALID_CREDENTIALS_ALTERNATE_KEYWORDS = {"Email", "ID", "Password", "incorrect"};
    protected static final String[] INVALID_EMAIL_KEYWORDS = {"Email", "invalid"};

    protected static final By CAPTCHA_TITLE_LOCATOR = By.cssSelector(LOGIN_FORM_CSS + " #captcha-container .otktitle-3");
    protected static final By CAPTCHA_INPUT_LOCATOR = By.cssSelector(LOGIN_FORM_CSS + " #captcha-container #recaptcha_response_field");

    protected static final String CAPTCHA_TITLE = "Are you a human";

    protected static final By WINDOW_TITLE_LOCATOR = By.xpath("//*[@id='lblWindowTitle']");
    protected static final By MINIMIZE_WINDOW_BUTTON_LOCATOR = By.xpath("//*[@id='btnMinimize']");
    protected static final By RESTORE_WINDOW_BUTTON_LOCATOR = By.xpath("//*[@id='btnRestore']");
    protected static final By CLOSE_WINDOW_BUTTON_LOCATOR = By.xpath("//*[@id='btnClose']");

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Qt inner class representing the Menu Bar at the top of the 'Login' page.
     */
    private class QtLoginWindowMenuBar extends OAQtSiteTemplate {

        /**
         * Constructor
         *
         * @param chromeDriver Selenium WebDriver (chrome)
         */
        private QtLoginWindowMenuBar(WebDriver chromeDriver) {
            super(chromeDriver);
        }

        /**
         * Get Windows title of the 'Login' window.
         *
         * @return Title String
         */
        private String getTitle() {
            EAXVxSiteTemplate.switchToLoginWindow(driver);
            return waitForElementVisible(WINDOW_TITLE_LOCATOR).getText();
        }

        /**
         * Click the 'Minimize' button at the top right of the menu to minimize the
         * 'Login' window. 'Origin Login' remains running in the background.
         */
        private void minimize() {
            EAXVxSiteTemplate.switchToLoginWindow(driver);
            waitForElementClickable(MINIMIZE_WINDOW_BUTTON_LOCATOR).click();
        }

        /**
         * Click the 'X' button at the top right of the menu to exit the 'Login'
         * window and Origin.
         */
        private void close() {
            EAXVxSiteTemplate.switchToLoginWindow(driver);
            waitForElementClickable(CLOSE_WINDOW_BUTTON_LOCATOR).click();
        }
    }

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public LoginPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Get the Windows title of the 'Login' window.
     *
     * @return Title as a String
     */
    public String getWindowTitle() {
        return new QtLoginWindowMenuBar(driver).getTitle();
    }

    /**
     * Click the 'Minimize' button at the top right of the menu to minimize the
     * 'Login' window. 'Origin Login' remains running in the background.
     */
    public void minimize() {
        new QtLoginWindowMenuBar(driver).minimize();
    }

    /**
     * Click the 'X' button at the top right of the menu to exit the 'Login 'window
     * and Origin.
     */
    public void close() {
        try {
            // Count the number of Origin processes and wait for them to decrease
            // since it's possible to have multiple Origin clients open at once
            final int numOriginProcs = ProcessUtil.getNumberOfProcessInstances(client, "Origin*");
            new QtLoginWindowMenuBar(driver).close();
            driver.quit();
            Waits.pollingWaitEx(() -> ProcessUtil.getNumberOfProcessInstances(client, "Origin*") < numOriginProcs);
        } catch (InterruptedException | IOException e) {
            // Wrap and throw
            throw new RuntimeException(e);
        }
    }

    /**
     * Check if the 'Site Stripe' message is visible.
     *
     * @return true if the 'Site Stripe' message is visible, false otherwise
     */
    public boolean isSiteStripeMessageVisible() {
        waitForPageThatMatches(OriginClientData.LOGIN_PAGE_URL_REGEX, 180);
        return isElementVisible(SITE_STRIPE_MESSAGE_LOCATOR);
    }

    /**
     * Get the 'Site Stripe' message from the 'Login' window.
     *
     * @return 'Site Stripe' message
     */
    public String getSiteStripeMessage() {
        waitForPageThatMatches(OriginClientData.LOGIN_PAGE_URL_REGEX, 180);
        return waitForElementVisible(SITE_STRIPE_MESSAGE_LOCATOR).getText();
    }

    /**
     * Verify the 'Site Stripe' message contains the network offline message.
     *
     * @return true if message contains the offline message keywords, false
     * otherwise
     */
    public boolean verifySiteStripeOfflineMessage() {
        if (Waits.pollingWait(this::isSiteStripeMessageVisible)) {
            return StringHelper.containsIgnoreCase(getSiteStripeMessage(), SITE_STRIPE_OFFLINE_MESSAGE_KEYWORDS);
        } else {
            return false;
        }
    }

    /**
     * Enter the username and password into the appropriate boxes on the Origin
     * login page and attempt to login.
     *
     * @param username The username to use
     * @param password The password to use
     * @param options {@link LoginOptions}<br>
     * - rememberMe Boolean flag for remember me option: true for check, false
     * for un-check, or null for no change<br>
     * - setInvisible Boolean flag for set invisible option: true for check,
     * false for un-check, or null for no change<br>
     * - handleUnderAgeFirstLogin boolean flag to handle parent email flow for
     * an underage user<br>
     * - submitUsingEnter if true press enter to login instead of clicking login
     * the button<br>
     * - noWaitForDiscoverPage if true return a DiscoverPage instance without
     * waiting for it to load
     * @return The {@link DiscoverPage} after a successful login
     * @throws RuntimeException If the username or password are null
     * @throws InterruptedException If the Thread is interrupted
     * @throws TimeoutException If any of the elements on the login page are not
     * found or visible
     */
    public DiscoverPage quickLogin(
            String username,
            String password,
            LoginOptions options) throws RuntimeException, InterruptedException, TimeoutException {

        if (StringHelper.nullOrEmpty(username)) {
            throw new RuntimeException("Username/email must be specified!");
        } else if (StringHelper.nullOrEmpty(password)) {
            throw new RuntimeException("User password must be specified!");
        }
        _log.debug("waiting login page to load");
        waitForPageToLoad();
        _log.debug("login page loaded");

        // Open the sidebar if on mobile
        new NavigationSidebar(driver).openSidebar();

        if (!isClient) {
            _log.debug("click browser login button");
            clickBrowserLoginBtn();
        }
        _log.debug("entering user name");
        inputUsername(username);
        _log.debug("entering password");
        inputPassword(password);
        if (options.invisible != null) {
            _log.debug("setting invisible");
            setInvisible(options.invisible);
        }
        if (options.rememberMe != null) {
            _log.debug("setting rememberMe");
            setRememberMe(options.rememberMe);
        }
        if (options.useEnter) {
            _log.debug("pressing enter");
            final WebElement passwordInput = waitForElementVisible(PASSWORD_INPUT_LOCATOR);
            passwordInput.sendKeys(Keys.RETURN);
        } else {
            _log.debug("clicking login button");
            clickLoginBtn();
        }

        if (options.handleUnderAgeFirstLogin) {
            _log.debug("entering handleUnderAgeFirstLogin");
            waitForPageThatMatches(OriginClientData.LOGIN_PAGE_URL_REGEX, 180);
            waitForPageToLoad();
            forceClickNextButton();
        }

        // Re open the sidebar if on mobile
        new NavigationSidebar(driver).openSidebar();

        if (options.noWaitForDiscoverPage) {
            _log.debug("Skip waiting for the myhome page");
            return new DiscoverPage(driver);
        }
        return waitForDiscoverPage();
    }

    /**
     * Waits for the 'My Home' page to load and switches to it.
     *
     * @return true if the 'My Home' page that was found, false otherwise
     */
    public DiscoverPage waitForDiscoverPage() {
        Waits.waitForWindowHandlesToStabilize(driver, 30);
        waitForPageThatMatches(OriginClientData.MAIN_SPA_PAGE_URL, 180);
        DiscoverPage discoverPage = nextPage(DiscoverPage.class);
        discoverPage.waitForPageToLoad(30);
        discoverPage.waitForAngularHttpComplete(30);
        return discoverPage;
    }

    /**
     * Enter the username and password into the appropriate boxes on the Origin
     * login page and attempt to login. Does not change "remember me" or "login
     * as invisible" options.
     *
     * @param username The username to use
     * @param password The password to use
     * @return The {@link DiscoverPage} after a successful login
     * @throws RuntimeException If the username or password are null
     * @throws InterruptedException If the Thread is interrupted
     * @throws TimeoutException If any of the elements on the login page are not
     * found or visible
     */
    public DiscoverPage quickLogin(String username, String password) throws RuntimeException, InterruptedException, TimeoutException {
        return quickLogin(username, password, new LoginOptions());
    }

    /**
     * Enters the user credentials stored in the {@link UserAccount} object and
     * attempts to login.
     *
     * @param userAccount The {@link UserAccount} object where the user info is
     * stored
     * @return The {@link DiscoverPage} after a successful login
     * @throws RuntimeException If the {@link UserAccount} object is null
     * @throws InterruptedException If the Thread is interrupted
     * @throws TimeoutException If any of the elements on the login page are not
     * found or visible
     */
    public DiscoverPage quickLogin(UserAccount userAccount) throws RuntimeException, InterruptedException, TimeoutException {
        if (userAccount == null) {
            throw new RuntimeException("User account cannot be null!");
        }
        return quickLogin(
                isClient ? userAccount.getUsername() : userAccount.getEmail(),
                userAccount.getPassword(),
                new LoginOptions().setRememberMe(userAccount.getRememberMe()).setInvisible(userAccount.getInvisible()));
    }

    /**
     * Clear the login form.
     */
    public void clearLoginForm() {
        WebElement emailInput = waitForElementClickable(EMAIL_INPUT_LOCATOR);
        WebElement passwordInput = waitForElementVisible(PASSWORD_INPUT_LOCATOR);
        emailInput.clear();
        passwordInput.clear();
        setRememberMe(false, true);
        // There is no Set Invisible on browser tests
        if (isElementVisible(LOGIN_INVISIBLE_LOCATOR)) {
            setInvisible(false, true);
        }
    }

    /**
     * Enter the username into the 'Username/Email' box on the Origin login window.
     *
     * @param username The username or email to enter into the box
     */
    public void inputUsername(String username) {
        inputStringByChar(EMAIL_INPUT_LOCATOR, username);
    }

    /**
     * Enter the password into the 'Password' box on the Origin login window.
     *
     * @param password The password to enter
     */
    public void inputPassword(String password) {
        inputStringByChar(PASSWORD_INPUT_LOCATOR, password);
    }

    /**
     * Input the default automation captcha string into the captcha box on the
     * Origin login window (using WebElement sendKeys command)<br>
     * NOTE: Current captcha hack is via white-boxing, so any string (including
     * null string) can be used as captcha.
     *
     * @throws TimeoutException if the captcha box cannot be found or is not
     * visible
     */
    public void inputCaptcha() throws TimeoutException {
        WebElement captchaInput = waitForElementVisible(CAPTCHA_INPUT_LOCATOR);
        captchaInput.clear();
        captchaInput.sendKeys(OriginClientData.CAPTCHA_AUTOMATION);
    }

    /**
     * Check or un-check the 'Remember Me' checkbox on the 'Login' page.
     *
     * @param remember true for checked, false for unchecked
     */
    public void setRememberMe(boolean remember) {
        setRememberMe(remember, false);
    }

    /**
     * Check or un-check the 'Remember Me' checkbox on the 'Login' page. Also
     * have the option to perform the operation forcefully using JavaScript,
     * which may be necessary for legacy case.
     *
     * @param remember {@code true} for checked, {@code false} for unchecked
     * @param force To specify if the operation should be performed forcefully
     * using JavaScript or not
     */
    public void setRememberMe(boolean remember, boolean force) {
        final boolean checked = isRememberMeChecked();
        WebElement rememberMe = waitForElementPresent(REMEMBER_ME_LOCATOR);
        final WebElement checkbox = waitForChildElementPresent(rememberMe, By.xpath(".."));
        waitForElementClickable(checkbox);
        if (checked != remember) {
            if (force) {
                forceClickElement(checkbox);
            } else {
                checkbox.click();
            }
        }
    }

    /**
     * Get the current state of the 'Remember Me' checkbox.
     *
     * @return {@code true} if checked, {@code false} if unchecked
     */
    public boolean isRememberMeChecked() {
        WebElement rememberMe = waitForElementPresent(REMEMBER_ME_LOCATOR);
        return Boolean.valueOf(rememberMe.getAttribute("checked"));
    }

    /**
     * Check or un-check the 'Login as Invisible' checkbox on the 'Login' page.
     *
     * @param invisible {@code true} for checked, {@code false} for unchecked
     */
    public void setInvisible(boolean invisible) {
        setInvisible(invisible, false);
    }

    /**
     * Check or un-check the 'Login as Invisible' checkbox on the 'Login' page.
     * Also have the option to perform the operation forcefully using
     * JavaScript, which may be necessary for legacy case.
     *
     * @param invisible {@code true} for checked, {@code false} for unchecked
     * @param force To specify if the operation should be performed forcefully
     * using JavaScript or not
     */
    public void setInvisible(boolean invisible, boolean force) {
        boolean checked = isLoginInvisibleChecked();
        WebElement invisibleId = waitForElementPresent(LOGIN_INVISIBLE_LOCATOR);
        final WebElement checkbox = waitForChildElementPresent(invisibleId, By.xpath(".."));
        waitForElementClickable(checkbox);
        if (checked != invisible) {
            if (force) {
                forceClickElement(checkbox);
            } else {
                checkbox.click();
            }
        }
    }

    /**
     * Get the current state of the 'Login as Invisible' checkbox.
     *
     * @return {@code true} if checked, {@code false} if unchecked
     */
    public boolean isLoginInvisibleChecked() {
        WebElement loginInvisible = waitForElementPresent(LOGIN_INVISIBLE_LOCATOR);
        return Boolean.valueOf(loginInvisible.getAttribute("checked"));
    }

    /**
     * Checks to see if the 'Login' button is disabled.
     *
     * @return {@code true} if disabled, {@code false} if enabled
     */
    public boolean isLoginButtonDisabled() {
        return isElementVisible(LOGIN_BTN_DISABLED_LOCATOR);
    }

    /**
     * Waits for the 'Login' button to be clickable and then clicks the 'Login'
     * button. The 'Login' button will only be clickable after the user and pass
     * have been entered.
     *
     * @throws TimeoutException If the login button is not found or if it is not
     * clickable
     */
    public void clickLoginBtn() throws TimeoutException {
        waitForElementClickable(LOGIN_BTN_LOCATOR).click();
    }

    /**
     * Click the 'Login' button. Should only be used in the browser where a user
     * can view the SPA anonymously
     */
    public void clickBrowserLoginBtn() {
        waitForElementClickable(BROWSER_LOGIN_BTN_LOCATOR).click();
        waitForPageThatMatches(OriginClientData.LOGIN_PAGE_URL_REGEX);
    }

    /**
     * Checks for the 'Login' button visibility in the browser
     *
     * @return true if it is visible, false otherwise
     */
    public boolean isLoginBtnVisible() {
        return isElementVisible(BROWSER_LOGIN_BTN_LOCATOR);
    }

    /**
     * Clicks the 'Login' button using JavaScript, if it is possible.
     */
    public void forceClickLoginBtn() {
        WebElement loginBtn = waitForElementPresent(LOGIN_BTN_LOCATOR);
        forceClickElement(loginBtn);
    }

    /**
     * Clicks the 'Next' button in the 'Verify Parent Email' dialog after underage
     * user login.
     */
    public void forceClickNextButton() {
        WebElement nextBtn = waitForElementClickable(PARENT_EMAIL_NEXT_BTN_LOCATOR);
        new Actions(driver).moveToElement(nextBtn).click().perform();
    }

    /**
     * Clicks on 'Create you EA Account' link for a new user.
     *
     * @return The Country DoB Register page
     */
    public CountryDoBRegisterPage clickRegistrationLink() {
        if (!isClient && !isOnLoginPage()) {
            clickBrowserLoginBtn();
        }

        WebElement registrationLink;
        registrationLink = waitForElementClickable(REGISTRATION_LINK_LOCATOR);
        forceClickElement(registrationLink);
        if (isClient) {
            waitForPageThatMatches("^.*origin.*$", 180);
        } else {
            waitForPageThatMatches("^.*create.*$", 180);

        }
        CountryDoBRegisterPage registerPage = nextPage(CountryDoBRegisterPage.class);
        registerPage.waitForPageToLoad();
        return registerPage;
    }

    /**
     * Clicks on the 'Sign In Tab' at the top of the 'Login' page.
     */
    public void clickSignInLink() {
        waitForElementClickable(SIGN_IN_LINK_LOCATOR).click();
        waitForPageToLoad();
    }

    /**
     * Checks if on the 'Login' page.
     *
     * @return true if the 'Login' button is visible, false otherwise
     */
    public boolean isOnLoginPage() {
        return isElementPresent(LOGIN_BTN_LOCATOR);
    }

    /**
     * Verify the banner stating the credentials appears, and that it contains
     * the correct information.
     *
     * @return true if the banner appears and contains the correct text, false
     * otherwise
     */
    public boolean verifyInvalidCredentialsBanner() {
        WebElement invalidCredentials = waitForElementVisible(INVALID_CREDENTIALS_LOCATOR);
        String invalidCredentialsText = invalidCredentials.getText();

        //The banner seems to change text a frequently, so we are checking
        //that it matches a few different options
        return StringHelper.containsIgnoreCase(invalidCredentialsText, INVALID_CREDENTIALS_KEYWORDS)
                || StringHelper.containsIgnoreCase(invalidCredentialsText, INVALID_CREDENTIALS_ALTERNATE_KEYWORDS)
                || StringHelper.containsIgnoreCase(invalidCredentialsText, INVALID_EMAIL_KEYWORDS);
    }

    /**
     * Verify that an error message stating that you must be online to log in
     * for the first time is visible.
     *
     * @return true if the message is visible, false otherwise
     */
    public boolean verifyOfflineErrorVisible() {
        return waitForElementVisible(OFFLINE_ERROR_LOCATOR).isDisplayed();
    }

    /**
     * Verify captcha is visible at the 'Login' window (Captcha appears after
     * multiple times of invalid logins).
     *
     * @return true if captcha is visible containing (case ignored) the expected
     * title, false otherwise
     */
    public boolean verifyCaptchaVisible() {
        WebElement captchaTitle = waitForElementVisible(CAPTCHA_TITLE_LOCATOR);
        return StringUtils.containsIgnoreCase(captchaTitle.getText(), CAPTCHA_TITLE);
    }

    /**
     * Verify 'Invalid Credentials' banner is visible.
     *
     * @return true if 'Invalid Credentials' banner is visible, false otherwise
     */
    public boolean isInvalidCredentailsBannerVisible() {
        return isElementVisible(INVALID_CREDENTIALS_LOCATOR);
    }

    /**
     * Enter the given username and password into the respective fields in the
     * login window, then press the 'Login' button. This does not verify that the
     * login was successful.
     *
     * @param username The string going into the username field
     * @param password The string going into the password field
     */
    public void enterInformation(String username, String password) {
        if (!OriginClient.getInstance(driver).isQtClient(driver) && !new LoginPage(driver).isOnLoginPage()) {
            new NavigationSidebar(driver).openSidebar();
            clickBrowserLoginBtn();
        }
        inputUsername(username);
        inputPassword(password);
        clickLoginBtn();
        sleep(1000); // Wait for page to stabilize.
    }

    /**
     * Verifies the password input indicates there was an error in the previous
     * attempt to log in.
     *
     * @return true if the password box indicates there was an error, false
     * otherwise
     */
    public boolean verifyPasswordError() {
        return isElementVisible(PASSWORD_ERROR_LOCATOR);
    }

    /**
     * Verifies the password input has been cleared
     *
     * @return true if the 'Password' box is cleared of text, false otherwise
     */
    public boolean verifyPasswordCleared() {
        return waitForElementVisible(PASSWORD_INPUT_LOCATOR).getText().equals("");
    }

    /**
     * Verifies that the title message is {@code message}.
     *
     * @param message The message to compare with
     *
     * @return true if the message and title match, false otherwise
     */
    public boolean verifyTitleIs(String message) {
        return waitForElementVisible(TITLE_LOCATOR).getText().equals(message);
    }

    /**
     * Click the 'Forgot Password' link.
     */
    public void clickForgotPassword() {
        waitForElementClickable(FORGOT_YOUR_PASSWORD_LOCATOR).click();
    }

    /**
     * Verify if the 'Two Factor Authentication Login' page is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyTwoFactorLoginPageVisible() {
        if (isClient) {
            return waitForElementVisible(TWO_FACTOR_LOGIN_FORM_TITLE_LOCATOR).getText().equalsIgnoreCase(TWO_FACTOR_LOGIN_FORM_EXPECTED_TITLE);
        } else {
            return waitForElementVisible(TWO_FACTOR_LOGIN_WEB_TITLE_LOCATOR).getText().equalsIgnoreCase(TWO_FACTOR_LOGIN_FORM_EXPECTED_TITLE);
        }
    }

    /**
     * Enter the security code at the 'Two Factor Authentication Login' page.
     *
     * @param securityCode Code to enter
     */
    public void enterTwoFactorLoginSecurityCode(String securityCode) {
        if (isClient) {
            waitForElementVisible(TWO_FACTOR_LOGIN_FORM_CODE_INPUT_LOCATOR).sendKeys(securityCode);
        } else {
            waitForElementVisible(TWO_FACTOR_LOGIN_WEB_CODE_INPUT_LOCATOR).sendKeys(securityCode);
        }
    }

    /**
     * Click the 'Sign In' button at the 'Two Factor Authentication Login' page.
     */
    public void clickTwoFactorSigninButton() {
        if (isClient) {
            waitForElementClickable(TWO_FACTOR_LOGIN_FORM_SIGN_IN_BUTTON_LOCATOR).click();
        } else {
            waitForElementClickable(TWO_FACTOR_LOGIN_WEB_SIGN_IN_BUTTON_LOCATOR).click();
        }
    }

    /**
     * Click the 'Send Verification/Security code' button at the 'Two Factor
     * Authentication Login' page.
     */
    public void clickTwoFactorSendVerificationCodeButton() {
        if (isClient) {
            waitForElementClickable(TWO_FACTOR_LOGIN_FORM_SEND_VERIFICATION_CODE_LOCATOR).click();
        } else {
            waitForElementClickable(TWO_FACTOR_LOGIN_WEB_SEND_VERIFICATION_CODE_LOCATOR).click();
        }
    }

    /**
     * Click the 'Update' button at the 'App Authenticator Setup' page.
     */
    public void clickTwoFactorCreateUpdateButton() {
        waitForElementClickable(TWO_FACTOR_LOGIN_CRETAE_UPDATE_BUTTON_LOCATOR).click();
    }

    /**
     * Click the 'Not Now' button at the 'App Authenticator Setup' page.
     */
    public void clickTwoFactorCreateNotNowButton() {
        waitForElementClickable(TWO_FACTOR_LOGIN_CRETAE_NOT_NOW_BUTTON_LOCATOR).click();
    }

    /**
     * Verify if the 'Login' page is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyLoginPageVisible() {
        String title = waitForElementVisible(LOGIN_PAGE_TITLE_LOCATOR).getText();
        return StringHelper.containsIgnoreCase(title, LOGIN_PAGE_TITLE_KEYWORDS);
    }

    /**
     * Verify the 'Underage Verification Login' page is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyUnderageVerificationLoginPageVisible() {
        String title = waitForElementVisible(UNDERAGE_VERIFICATION_LOGIN_FORM_TITLE_LOCATOR).getText();
        return StringHelper.containsIgnoreCase(title, UNDERAGE_VERIFICATION_LOGIN_FORM_EXPECTED_TITLE_KEYWORDS);
    }

    /**
     * Click the 'Continue' button at the 'Underage Verification Login' page.
     */
    public void clickUnderageVerificationContinueButton() {
        waitForElementClickable(UNDERAGE_VERIFICATION_LOGIN_FORM_CONTINUE_BUTTON_LOCATOR).click();
    }

    /**
     * Verify a 'Email not Verified' error message is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyUnderAgeEmailVerificationErrorMessageVisible() {
        String title = waitForElementVisible(UNDERAGE_EMAIL_VERIFICATION_ERROR_MESSAGE_LOCATOR).getText();
        return StringHelper.containsIgnoreCase(title, UNDERAGE_EMAIL_VERIFICATION_ERROR_MESSAGE_EXPECTED_TITLE_KEYWORDS);
    }
}