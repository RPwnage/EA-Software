package com.ea.originx.automation.lib.macroaction;

import com.ea.originx.automation.lib.helpers.EmailFetcher;
import com.ea.originx.automation.lib.pageobjects.account.AboutMePage;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.MyAccountHeader;
import com.ea.originx.automation.lib.pageobjects.account.OrderHistoryLine;
import com.ea.originx.automation.lib.pageobjects.account.OrderHistoryPage;
import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage.PrivacySetting;
import com.ea.originx.automation.lib.pageobjects.account.SecurityPage;
import com.ea.originx.automation.lib.pageobjects.account.TwoFactorBackupCodesDialog;
import com.ea.originx.automation.lib.pageobjects.account.TwoFactorTurnOnDialog;
import com.ea.originx.automation.lib.pageobjects.account.TwoFactorVerifyCodeDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.SecurityQuestionDialog;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.LanguageInfo.LanguageEnum;
import com.ea.vx.originclient.utils.Waits;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import java.awt.AWTException;
import java.lang.invoke.MethodHandles;
import java.util.List;
import java.util.stream.Collectors;
import org.openqa.selenium.TimeoutException;

/**
 * Macro class containing static methods for multi-step actions related to the
 * 'Account and Billing' page.
 *
 * @author palui
 */
public final class MacroAccount {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     */
    private MacroAccount() {
    }

    /**
     * Turn on 'Two-Factor Authentication'. Assume the 'Security' section of the
     * 'Account and Billing' page has already been opened.
     *
     * @param browserDriver Selenium WebDriver
     * @param username      The username for verifying a user's email address in the
     *                      recipient list
     * @return True if successfully turned on login verification, false
     * otherwise
     */
    public static boolean turnOnTwoFactorAuthentication(WebDriver browserDriver, String username) {
        _log.info("Turning on 'Two Factor Login Verification'");
        SecurityPage securityPage = new SecurityPage(browserDriver);
        securityPage.clickTwoFactorAuthenticationOnButton();
        securityPage.answerSecurityQuestion();

        TwoFactorTurnOnDialog turnOnDialog = new TwoFactorTurnOnDialog(browserDriver);
        if (!turnOnDialog.waitIsVisible()) {
            _log.error("'Turn On Login Verification' dialog failed to open");
            return false;
        }
        turnOnDialog.clickEmailRadioButton();
        turnOnDialog.clickContinueButton();

        TwoFactorVerifyCodeDialog verifyCodeDialog = new TwoFactorVerifyCodeDialog(browserDriver);
        if (!verifyCodeDialog.waitIsVisible()) {
            _log.error("'Verify Code' dialog failed to open");
            return false;
        }

        Waits.sleep(30000); // wait for Security Code email to arrive
        String securityCode = new EmailFetcher(username).getLatestSecurityCode();
        verifyCodeDialog.enterSecurityCode(securityCode);
        verifyCodeDialog.clickTurnOnButton();

        TwoFactorBackupCodesDialog twoFactorBackupCodeDialog = new TwoFactorBackupCodesDialog(browserDriver);
        if (!twoFactorBackupCodeDialog.waitIsVisible()) {
            _log.error("'Backup Codes' dialog failed to open");
            return false;
        }
        twoFactorBackupCodeDialog.clickCloseButton();
        return true;
    }

    /**
     * Sets privacy for a user by logging into Origin in a browser and going to
     * the 'My Account: Privacy Settings' page. Logout when done.
     *
     * @param browserDriver Selenium WebDriver
     * @param user User account to set
     * @param setting What to set
     * @return True if settings were set successfully, false otherwise
     */
    public static boolean setPrivacyThroughBrowserLogin(WebDriver browserDriver, UserAccount user, PrivacySetting setting) {
        try {
            if (!loginToAccountPage(browserDriver, user)) {
                return false;
            } 
        } catch (TimeoutException te){
            if (!loginToAccountPage(browserDriver, user)) {
                return false;
            } 
        }
        AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.gotoPrivacySettingsSection();
        PrivacySettingsPage privacyPage = new PrivacySettingsPage(browserDriver);
        privacyPage.setPrivacySetting(setting);
        if (privacyPage.verifyPrivacySettingMatches(setting)) {
            accountSettingsPage.logout();
            return true;
        } else {
            _log.error(String.format("Cannot set Privacy to '%s', current setting is '%s'", setting.toString(), privacyPage.getPrivacyLabel()));
            accountSettingsPage.logout();
            return false;
        }
    }

    /**
     * Open the lockbox account page for the given user.
     *
     * @param browserDriver Selenium WebDriver
     * @param user The user to login to
     * @return True if the account settings page opens, false otherwise
     */
    public static boolean loginToAccountPage(WebDriver browserDriver, UserAccount user) {
        AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.navigateToIndexPage();
        //Check if the user is logged in already, if yes, skip login process and return true.
        if (accountSettingsPage.verifyAccountSettingsPageReached()) {
            return true;
        } else {
            try {
                accountSettingsPage.login(user);
            } catch (TimeoutException e) {
                _log.error("Failed to login to 'Account Settings' page");
                return false;
            }

            boolean settingsPageReached = accountSettingsPage.verifyAccountSettingsPageReached();
            if (!settingsPageReached) {
                _log.error("Cannot reach the 'Account Settings' page");
                return false;
            }
        }
        return true;
    }

    /**
     * Logs out of account currently signed into My Account
     *
     * @param browserDriver Selenium WebDriver
     * @return True if the account page is not shown (logged out), false
     * otherwise
     */
    public static boolean logoutOfMyAccount(WebDriver browserDriver) {
        new MyAccountHeader(browserDriver).clickLogout();
        AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);
        if (Waits.pollingWait(() -> !accountSettingsPage.verifyAccountSettingsPageReached())) {
            return true;
        }

        _log.error("Unable to sign out of My Account");
        return false;
    }

    /**
     * Registers new throwaway user accounts
     *
     * @param driver Selenium WebDriver
     * @param users An array of user accounts to register
     * @return True if registrations were successful, false otherwise
     *
     * @throws Exception
     */
    public static boolean registerUsers(WebDriver driver, UserAccount[] users) throws Exception {
        boolean registerUsersOK = true;
        for (UserAccount user : users) {
            registerUsersOK = registerUsersOK && MacroLogin.startLogin(driver, user);
            new MiniProfile(driver).selectSignOut();
        }
        return registerUsersOK;
    }

    /**
     * Verify all the 'Order Descriptions' provided exist in the 'Order History
     * Page'
     *
     * @param driver            Selenium WebDriver
     * @param orderDescriptions the list of order descriptions to be verified
     * @return true if the list contains in 'Order History Lines'
     */
    public static boolean verifyAllOrderDescriptionExists(WebDriver driver, List<String> orderDescriptions) {
        List<OrderHistoryLine> orderHistoryLines = new OrderHistoryPage(driver).getOrderHistoryLines();
        if (!orderHistoryLines.isEmpty()) {
            List<String> orderHistoryLinesDescription = orderHistoryLines.stream().map(orderHistoryLine -> orderHistoryLine.getDescription()).collect(Collectors.toList());
            return orderHistoryLinesDescription.containsAll(orderDescriptions);
        } else {
            return false;
        }
    }

    /**
     * Create a new account in Korea. This is a workaround to create an account
     * in Korea as it requires I-Pin/Mobile Information for registration This
     * function creates an account in Canada and changes the country in 'EA
     * Account and Billing' Page
     *
     * @param driver Selenium WebDriver
     * @param client Origin Client
     * @return the user account after changing to country Korea
     * @throws AWTException
     * @throws InterruptedException
     */
    public static UserAccount createNewThrowAwayAccountInKorea(WebDriver driver, OriginClient client) throws AWTException, InterruptedException {
        Waits.sleep(1); // Guarantee we get a unique timestamp
        UserAccount userAccount = new UserAccount.AccountBuilder().country(CountryInfo.CountryEnum.CANADA.getCountry()).exists(false).build();
        MacroLogin.startLogin(driver, userAccount);
        WebDriver settingsDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        MacroAccount.loginToAccountPage(settingsDriver, userAccount);
        new AccountSettingsPage(settingsDriver).gotoAboutMeSection();
        AboutMePage aboutMePage = new AboutMePage(settingsDriver);
        aboutMePage.changeCountryName(CountryInfo.CountryEnum.SOUTH_KOREA);
        aboutMePage.changeLanguageName(LanguageEnum.KOREAN);
        settingsDriver.close();
        return userAccount;
    }

    /**
     * Enter the answer to the security question assuming the dialog is already open in the A&B page
     *
     * @param driver
     * @aparam answerText the answer to the question
     */
    public static void answerSecurityQuestion(WebDriver driver, String answerText) {
        SecurityQuestionDialog securityQuestionDialog = new SecurityQuestionDialog(driver);
        securityQuestionDialog.clearQuestionAnswer();
        securityQuestionDialog.enterSecurityQuestionAnswer(answerText);
        securityQuestionDialog.clickContinue();
    }

    /**
     * Get the invoice number from the accounts settings page's order history
     *
     * @param driver the driver
     * @param index index of the invoice
     * @return String invoice number
     */
    public static String getInvoiceNumber(WebDriver driver, int index) {
        AccountSettingsPage accountSettingsPage = new AccountSettingsPage(driver);
        accountSettingsPage.gotoOrderHistorySection();
        OrderHistoryPage orderHistoryPage = new OrderHistoryPage(driver);
        orderHistoryPage.verifyOrderHistorySectionReached();
        return orderHistoryPage.getOrderHistoryLine(index).getOrderNumber();
    }
}
