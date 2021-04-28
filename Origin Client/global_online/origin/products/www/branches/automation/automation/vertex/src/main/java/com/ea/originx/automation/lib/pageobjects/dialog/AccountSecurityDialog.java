package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.vx.originclient.account.UserAccount;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the dialog in the 'Account Settings' page that allows changing of
 * the user's password and security question.
 *
 * @author jmittertreiner
 * @author mdobre
 */
public class AccountSecurityDialog extends Dialog {

    private static final By ORIGINAL_PASSWORD_FIELD = By.id("originalPassword");
    private static final By NEW_PASSWORD_FIELD = By.id("newPassword");
    private static final By CONFIRM_PASSWORD_FIELD = By.id("newPasswordR");
    private static final By SAVE_PASSWORD_BUTTON = By.id("savebtn_cpass");
    private static final By SECURITY_EDIT_BUTTON = By.id("editSecuritySection2");
    private static final By DIALOG_LOCATOR = By.id("security_section2");
    private static final By CANCEL_BUTTON = By.id("cancelbtn_securitySection_for_password");
    private static final By TITLE_LOCATOR = By.id("security_section_header");
    private static final By SECURITY_QUESTION_LOCATOR = By.id("security_question");
    private static final By SECURITY_QUESTIONS_TAB_LOCATOR = By.id("security_questions");
    private static final By ADD_ANOTHER_QUESTION_LOCATOR = By.id("addMoreLink");
    private static final By SECURITY_QUESTIONS_CSS = By.cssSelector("#questionContainer .origin-ux-drop-down-selection");
    private static final String SECURITY_QUESTIONS_OPTIONS = "#questionBlock%s #drop-down-container%s #drop-down%s .drop-down-options a";
    private static final By SECURITY_ANSWERS_LOCATOR = By.cssSelector("#questionContainer .origin-ux-textbox-control input");
    private static final By DUPLICATED_QUESTION_WARNING_LOCATOR = By.xpath("//*[contains(text(),'Duplicated Question')]");

    private static final String SECURITY_QUESTIONS_ANSWER = "QAt3st3r";
    private static final String DUPLICATED_QUESTION_WARNING_TEXT = "Duplicated Question";

    public AccountSecurityDialog(WebDriver driver) {
        super(driver, DIALOG_LOCATOR, TITLE_LOCATOR, CANCEL_BUTTON);
    }

    /**
     * Verify the account security dialog appeared
     *
     * @return true if the dialog is visible, false otherwise
     */
    public boolean verifyAccountSecurityDialogReached() {
        return waitIsElementPresent(ORIGINAL_PASSWORD_FIELD);
    }

    /**
     * Enters the password of the user account.
     *
     * @param account The account to change the password of
     */
    public void enterCurrentPassword(UserAccount account) {
        waitForElementVisible(ORIGINAL_PASSWORD_FIELD).sendKeys(account.getPassword());
    }

    /**
     * Enters the new password in both the new password box and the confirm new
     * password box.
     *
     * @param newPassword The new password to enter
     */
    public void enterAndConfirmNewPassword(String newPassword) {
        waitForElementVisible(NEW_PASSWORD_FIELD).sendKeys(newPassword);
        waitForElementVisible(CONFIRM_PASSWORD_FIELD).sendKeys(newPassword);
    }

    /**
     * Clicks the 'Save' button, which returns you to the 'Account Settings'
     * page
     */
    public void clickSave() {
        waitForElementClickable(SAVE_PASSWORD_BUTTON).click();
        waitForElementClickable(SECURITY_EDIT_BUTTON);
    }

    /**
     * Navigates to the Security Question Tab
     */
    public void navigateToSecurityQuestionTab() {
        waitForElementClickable(SECURITY_QUESTION_LOCATOR).click();
    }

    /**
     * Verifies the 'Security Question' tab is reached
     *
     * @return true is the tab was reached, false otherwise
     */
    public boolean verifySecurityQuestionTabReached() {
        return waitIsElementVisible(SECURITY_QUESTIONS_TAB_LOCATOR);
    }

    /**
     * Click on 'Add Another Question' link
     */
    public void addAnotherQuestion() {
        waitForElementClickable(ADD_ANOTHER_QUESTION_LOCATOR).click();
    }

    /**
     * Gets the number of security questions the user has
     *
     * @return an int, which represents the number of questions
     */
    public int getNumberOfSecurityQuestions() {
        return driver.findElements(SECURITY_QUESTIONS_CSS).size();
    }

    /**
     * Verifies the 'Duplicated Question' warning is visible
     *
     * @return true if the warning is displayed, false otherwise
     */
    public boolean verifyDuplicatedQuestionWarningVisible(int index) {
        return driver.findElements(DUPLICATED_QUESTION_WARNING_LOCATOR).get(index).isDisplayed();
    }

    /**
     * Enters an answer to all security question
     *
     * @param securityQuestionAnswer the answer which will be assigned to the
     * questions
     */
    public void enterAnswerToAllSecurityQuestion(String securityQuestionAnswer) {
        driver.findElements(SECURITY_ANSWERS_LOCATOR).stream().forEach((item) -> {
            item.sendKeys(securityQuestionAnswer);
        });
    }

    /**
     * Verify the answers to duplicated questions are not saved
     *
     * @param lastSecurityQuestionIndex the question which needs answer
     *
     * @return true if after entering 2 answers, the 'Duplicated Question'
     * message is still displayed, false otherwise
     */
    public boolean verifyAnswerUnsaved(int lastSecurityQuestionIndex) {
        enterAnswerToAllSecurityQuestion(SECURITY_QUESTIONS_ANSWER);
        clickSave();
        return verifyDuplicatedQuestionWarningVisible(lastSecurityQuestionIndex);
    }
}
