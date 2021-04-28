package com.ea.originx.automation.lib.pageobjects.account;

import com.ea.vx.originclient.account.UserAccount;
import com.ea.originx.automation.lib.pageobjects.dialog.AccountSecurityDialog;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represent the 'Security' page ('Account Settings' page with the 'Security'
 * section open)
 *
 * @author palui
 */
public class SecurityPage extends AccountSettingsPage {

    private final By SECURITY_EDIT_BUTTON = By.id("editSecuritySection2");
    private final By TWO_FACTOR_AUTHENTICATION_ON_BUTTON = By.id("twofactoronbtn");
    private final By TWO_FACTOR_AUTHENTICATION_OFF_BUTTON = By.id("twofactoroffbtn");

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public SecurityPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'Security' section of the 'Account Settings' page is open
     *
     * @return true if open, false otherwise
     */
    public boolean verifySecuritySectionReached() {
        return verifySectionReached(NavLinkType.SECURITY);
    }

    /**
     * Change user account password
     *
     * @param account user account to change
     * @param newPassword password to change to
     */
    public void changePassword(UserAccount account, String newPassword) {
        waitForElementClickable(SECURITY_EDIT_BUTTON).click();
        answerSecurityQuestion();
        final AccountSecurityDialog accountSecurityDialog = new AccountSecurityDialog(driver);
        accountSecurityDialog.waitForVisible();
        accountSecurityDialog.enterCurrentPassword(account);
        accountSecurityDialog.enterAndConfirmNewPassword(newPassword);
        accountSecurityDialog.clickSave();
    }

    /**
     * Click the "Turn On" button for two-factor authentication
     */
    public void clickTwoFactorAuthenticationOnButton() {
        _log.info("Turning on 2-factor authentication");
        waitForElementClickable(TWO_FACTOR_AUTHENTICATION_ON_BUTTON).click();
    }

    /**
     * Click the "Turn Off" button for two-factor authentication
     */
    public void clickTwoFactorAuthenticationOffButton() {
        _log.info("Turning off 2-factor authentication");
        waitForElementClickable(TWO_FACTOR_AUTHENTICATION_OFF_BUTTON).click();
    }

    /**
     * Click the 'Edit' button
     */
    public void clickEditButton() {
        waitForElementClickable(SECURITY_EDIT_BUTTON).click();
    }
}
