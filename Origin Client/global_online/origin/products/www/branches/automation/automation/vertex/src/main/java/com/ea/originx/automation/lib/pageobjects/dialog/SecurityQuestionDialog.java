package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.originx.automation.lib.resources.OriginClientData;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the security question dialog that appears on the account settings
 * page when managing your account.
 *
 * @author jmittertreiner
 */
public class SecurityQuestionDialog extends Dialog {

    // Depending on the page, the id may or may not have "_for_basic_info" appended to it.
    // We use XPATH to find either one.
    private static final By DIALOG_LOCATOR = By.xpath("//div[contains(@style, 'display: block')]//input[contains(@id, 'challenge_securityanswer')]");
    private static final By SAVE_BUTTON_LOCATOR = By.xpath("//div[contains(@style, 'display: block')]//a[contains(@id, 'savebtn_security_question_challenge')]");
    private static final By WARNING_LOCATOR = By.xpath("//*[contains(@class, 'origin-ux-textbox-status-message origin-ux-status-message')]");
    
    private static final String WARNING_MESSAGE_TEXT = "Invalid Security Answer.";
            
    public SecurityQuestionDialog(WebDriver webDriver) {
        super(webDriver, DIALOG_LOCATOR, null, null);
    }

    /**
     * Clicks the 'Continue' button.
     */
    public void clickContinue() {
        waitForElementVisible(SAVE_BUTTON_LOCATOR).click();
    }
    
    /**
     * Clears the answer of the question
     */
    public void clearQuestionAnswer() {
        waitForElementVisible(DIALOG_LOCATOR).clear();
    }

    /**
     * Enter the answer to the security question.
     */
    public void enterSecurityQuestionAnswer() {
        enterSecurityQuestionAnswer(OriginClientData.ACCOUNT_PRIVACY_SECURITY_ANSWER);
    }
    
    /**
     * Enter an answer to the security question.
     * @param answerText the answer sent by the user
     */
    public void enterSecurityQuestionAnswer(String answerText) {
        waitForElementVisible(DIALOG_LOCATOR).sendKeys(answerText);
    }
    
    /**
     * Verify the warning message stating the answer is incorrect appeared
     * @return true if the warning is visible, false otherwise
     */
    public boolean verifyWrongAnswerWarningDialog() {
        return waitForElementVisible(WARNING_LOCATOR).getText().equals(WARNING_MESSAGE_TEXT);
    }
}