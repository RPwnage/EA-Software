package com.ea.originx.automation.lib.macroaction;

import com.ea.originx.automation.lib.helpers.EmailFetcher;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.SystemMessage;
import com.ea.originx.automation.lib.pageobjects.login.BasicInfoRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.CountryDoBRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.EmailVerificationWebPage;
import com.ea.originx.automation.lib.pageobjects.login.GetStartedPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginOptions;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.login.SecurityQuestionAnswerPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.utils.Waits;
import java.awt.AWTException;
import java.io.IOException;
import java.lang.invoke.MethodHandles;
import javax.mail.MessagingException;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;

/**
 * Macro class containing static methods for multi-step actions related to the login or registration page.
 *
 * @author mkalaivanan
 */
public class MacroLogin {

    public final static int NUMBER_OF_INVALID_LOGINS_TO_TRIGGER_CAPTCHA = 4;

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    private MacroLogin() {
    }

    /**
     * Login or register as the specified user account.
     *
     * @param driver             Selenium WebDriver
     * @param userAccount        User account to register or login to
     * @param options            {@link LoginOptions}<br>
     *                           - rememberMe Boolean flag for remember me option<br>
     *                           - setInvisible Boolean flag for set invisible option<br>
     *                           - handleUnderAgeFirstLogin boolean flag to handle parent email flow for
     *                           an underage user<br>
     * @param newUsername        New username to be used for login
     * @param newUserEmail       User email address to be used for login
     * @param newPassword        New password to be used for login
     * @param uncheckFindByEmail Boolean flag to handle unchecking of 'Find By
       *                           Email Address' visibility checkbox
     * @return true if 'Mini Profile' has the right username, otherwise false
     * @throws java.lang.InterruptedException
     * @throws java.awt.AWTException
     */
    public static boolean startLogin(WebDriver driver,
                                     UserAccount userAccount,
                                     LoginOptions options,
                                     String newUsername,
                                     String newUserEmail,
                                     String newPassword,
            boolean uncheckFindByEmail, String privacyVisibilitySetting)
            throws InterruptedException, AWTException {
        boolean isClient = OriginClient.getInstance(driver).isQtClient(driver);
        LoginPage loginPage = new LoginPage(driver);

        if (isClient) {
            //Switch the driver to the login page just in case we aren't
            //already on it, which may be the case if we log out of Origin
            //and are trying to log back in
            Waits.waitForPageThatMatches(driver, OriginClientData.LOGIN_PAGE_URL_REGEX, 30);
        }

        // Open the sidebar on mobile if it is not open
        // When 'Login Page' is open the Sidebar element cannot be interacted with
        if (!loginPage.isOnLoginPage()) {
            new NavigationSidebar(driver).openSidebar();
        }

        if (!userAccount.getExists()) {
            loginPage.clickRegistrationLink();
            quickRegister(userAccount, driver, uncheckFindByEmail, privacyVisibilitySetting);
        } else {
            final String email = newUserEmail.isEmpty() ? userAccount.getEmail() : newUserEmail;
            final String password = newPassword.isEmpty() ? userAccount.getPassword() : newPassword;
            final String username = newUsername.isEmpty() ? userAccount.getUsername() : newUsername;
            final String loginName = isClient ? username : email;
            loginPage.quickLogin(loginName, password, options);
        }

        // Verify login successful
        if (!options.skipVerifyingMiniProfileUsername && !verifyMiniProfileUsername(driver, userAccount.getUsername())) {
            return false;
        }

        // Close all open SiteStripes, attempt a max of 10 times.
        SystemMessage siteStripe = new SystemMessage(driver);
        int i = 0;
        while (siteStripe.isSiteStripeVisible() && i <= 10) {
            siteStripe.tryClosingSiteStripe();
            i++;
        }
        return true;
    }

    /**
     * Verify expected username against 'Mini Profile' username to confirm user has logged in.
     *
     * @param driver   Selenium WebDriver
     * @param username Expected username
     * @return true if usernames match, false otherwise
     */
    public static boolean verifyMiniProfileUsername(WebDriver driver, String username) {
        MiniProfile miniProfile = new MiniProfile(driver);
        boolean isClient = OriginClient.getInstance(driver).isQtClient(driver);
        _log.debug("waiting mini profile page to load");
        miniProfile.waitForPageToLoad();
        _log.debug("mini profile page loaded");
        if (isClient) {
            miniProfile.waitForAngularHttpComplete();
        }
        _log.debug("mini profile page Angular has completed, verifying user: " + username);
        return miniProfile.verifyUser(username);
    }

    /**
     * Login or register as the specified user account.
     *
     * @param driver      Selenium WebDriver
     * @param userAccount User account to register or log in to
     * @param options     {@link LoginOptions}<br>
     * @return true if 'Mini Profile' has the right username, otherwise false
     * @throws java.lang.InterruptedException
     * @throws java.awt.AWTException
     */
    public static boolean startLogin(WebDriver driver, UserAccount userAccount, LoginOptions options)
            throws InterruptedException, AWTException {
        return startLogin(driver, userAccount, options, "", "", "", true, SecurityQuestionAnswerPage.DEFAULT_PRIVACY_VISIBLITY_SETTING);
    }

    /**
     * Login or register as the specified user account.
     *
     * @param driver      Selenium WebDriver
     * @param userAccount User account to register or log in to
     * @return true if 'Mini Profile' has the right username, otherwise false
     * @throws java.lang.InterruptedException
     * @throws java.awt.AWTException
     */
    public static boolean startLogin(WebDriver driver, UserAccount userAccount) {
        try {
            return startLogin(driver, userAccount, new LoginOptions());
        } catch (Exception e) {
            _log.error("Login Failed !!!!");
            _log.error(e);
            return false;
        }
    }

    /**
     * Login when the Origin client is disconnected from the network.
     *
     * @param driver      Selenium WebDriver
     * @param userAccount User account to log in to
     * @return true if successfully logged into the Origin client when disconnected from the network, false otherwise
     */
    public static boolean networkOfflineClientLogin(WebDriver driver, UserAccount userAccount)
            throws InterruptedException, AWTException {
        boolean isClient = OriginClient.getInstance(driver).isQtClient(driver);
        if (!isClient) {
            String errorMessage = "Network Offline login is for testing Origin client only";
            _log.error(errorMessage);
            throw new RuntimeException(errorMessage);
        }
        Waits.waitForPageThatMatches(driver, OriginClientData.LOGIN_PAGE_URL_REGEX, 30);
        final LoginPage loginPage = new LoginPage(driver);
        loginPage.quickLogin(userAccount.getUsername(), userAccount.getPassword(), new LoginOptions().setNoWaitForDiscoverPage(true));
        // Can take couple minutes for main page to show
        if (Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.MAIN_SPA_PAGE_URL, 180)) {
            MiniProfile miniProfile = new MiniProfile(driver);
            // Can take couple minutes more for mini profile to show
            return miniProfile.verifyUser(userAccount.getUsername());
        }
        return false;
    }

    /**
     * Register a user.
     *
     * @param userAccount User Account to register
     * @param driver      Selenium WebDriver
     * @throws InterruptedException
     */
    public static void quickRegister(UserAccount userAccount, WebDriver driver) throws InterruptedException {
        quickRegister(userAccount, driver, false, SecurityQuestionAnswerPage.DEFAULT_PRIVACY_VISIBLITY_SETTING);
    }

    /**
     * Register a user.
     *
     * @param userAccount        User Account to register
     * @param driver             Selenium WebDriver
     * @param uncheckFindByEmail boolean flag to handle unchecking of 'Find By
     *                           Email Address' visibility checkbox
     * @throws InterruptedException
     */
    public static void quickRegister(UserAccount userAccount,
                                     WebDriver driver,
            boolean uncheckFindByEmail, String privacyVisibilitySetting) throws InterruptedException {
        new CountryDoBRegisterPage(driver).enterDoBCountryInformation(userAccount);

        BasicInfoRegisterPage createAccPage = new BasicInfoRegisterPage(driver);
        createAccPage.waitForBasicInfoPage();
        boolean isUnderAge = createAccPage.isUnderAge();
        createAccPage.inputUserAccountRegistrationDetailsWithRetry(userAccount);
        if (!isUnderAge) {
            SecurityQuestionAnswerPage securityQuestionAnswerPage = new SecurityQuestionAnswerPage(driver);
            securityQuestionAnswerPage.waitForPage();
            securityQuestionAnswerPage.enterSecurityQuestionAnswerWithRetry(uncheckFindByEmail, privacyVisibilitySetting);
        }

        GetStartedPage getStartedPage = new GetStartedPage(driver);
        getStartedPage.waitForPage();
        getStartedPage.clickGetStartedButton();

        Waits.waitForWindowHandlesToStabilize(driver, 30);
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 30);
        getStartedPage.waitForPageToLoad();
        userAccount.setExists(true);
    }

    /**
     * Login to fail multiple times using a wrong password.
     *
     * @param driver      Selenium WebDriver
     * @param userAccount Account to login to
     * @param nTimes      The number of times to enter the wrong password
     * @return true if all login attempts fail, false otherwise
     */
    public static boolean loginToFailMultipleTimes(WebDriver driver, UserAccount userAccount, int nTimes) {
        boolean isClient = OriginClient.getInstance(driver).isQtClient(driver);
        LoginPage loginPage = new LoginPage(driver);
        String loginId = isClient ? userAccount.getUsername() : userAccount.getEmail();
        String wrongPassword = "Wrong" + userAccount.getPassword();
        for (int i = 0; i < nTimes; i++) {
            loginPage.enterInformation(loginId, wrongPassword);
            boolean isLoginPage = Waits.pollingWait(() -> loginPage.isPageThatMatchesOpen(OriginClientData.LOGIN_PAGE_URL_REGEX));
            // Origin doesn't like it if you attempt to login in too many times too quickly
            // Give it some time to recover between attempts
            Waits.sleep(1000);
            if (!isLoginPage) {
                return false;
            }
        }
        return true;
    }

    /**
     * Login to fail multiple times using a wrong password to cause Captcha to
     * appear at the Login window.
     *
     * @param driver      Selenium WebDriver
     * @param userAccount The account to log in to
     * @return true if all login attempts fail as expected, false otherwise
     */
    public static boolean loginTriggerCaptcha(WebDriver driver, UserAccount userAccount) {
        return loginToFailMultipleTimes(driver, userAccount, NUMBER_OF_INVALID_LOGINS_TO_TRIGGER_CAPTCHA);
    }

    /**
     * Login with correct user ID and password, and with captcha entered.
     *
     * @param driver      Selenium WebDriver
     * @param userAccount The account to log in to
     * @return true if login successful, false otherwise
     */
    public static boolean loginWithCaptcha(WebDriver driver, UserAccount userAccount) {
        boolean isClient = OriginClient.getInstance(driver).isQtClient(driver);
        String loginId = isClient ? userAccount.getUsername() : userAccount.getEmail();
        LoginPage loginPage = new LoginPage(driver);
        loginPage.inputCaptcha();
        loginPage.enterInformation(loginId, userAccount.getPassword());

        loginPage.waitForDiscoverPage();
        // Verify login successful
        return verifyMiniProfileUsername(driver, userAccount.getUsername());
    }

    /**
     * Verifies the parent email by fetching the verify email link from the
     * email and posts in a new web browser (assumes a browser was already opened
     * before this function)
     *
     * @param browserDriver  Selenium WebDriver
     * @param underageUserID The username of the underAge account
     * @return true if the parental email verification link was verified successfully
     * @throws IOException
     * @throws MessagingException
     */
    public static boolean verifyParentEmail(WebDriver browserDriver, String underageUserID) throws
            IOException, MessagingException {
        EmailFetcher emailFetcher = new EmailFetcher(underageUserID);
        String verifyEmailLink = emailFetcher.getVerifyParentEmailLink();
        emailFetcher.closeConnection();
        browserDriver.get(verifyEmailLink);
        EmailVerificationWebPage verificationWebPage = new EmailVerificationWebPage(browserDriver);
        verificationWebPage.waitForEmailVerificationWebPageToLoad();
        boolean success = verificationWebPage.verifyTitle() && verificationWebPage.verifySuccessfulResult();
        Waits.sleep(2000); // wait a little for email verification to take its course
        return success;
    }

    public static boolean loginAnonymousProfile(WebDriver driver, UserAccount userAccount) {
        LoginPage loginPage = new LoginPage(driver);
        loginPage.waitForPageToLoad();
        loginPage.enterInformation(userAccount.getEmail(), userAccount.getPassword());
        // Can take couple minutes for main page to show
        if (Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.MAIN_SPA_PAGE_URL, 180)) {
            MiniProfile miniProfile = new MiniProfile(driver);
            // Can take couple minutes more for mini profile to show
            return miniProfile.verifyUser(userAccount.getUsername());
        }
        return false;
    }
}