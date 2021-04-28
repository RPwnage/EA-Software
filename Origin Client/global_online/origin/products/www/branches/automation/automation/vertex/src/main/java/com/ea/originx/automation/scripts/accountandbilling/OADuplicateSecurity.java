package com.ea.originx.automation.scripts.accountandbilling;

import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.SecurityPage;
import com.ea.originx.automation.lib.pageobjects.dialog.AccountSecurityDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.SecurityQuestionDialog;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import java.util.List;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test adding a duplicate question in the account settings
 *
 * @author mdobre
 */
public class OADuplicateSecurity extends EAXVxTestTemplate {

    @TestRail(caseId = 10104)
    @Test(groups = {"accountbilling", "browser_only"})
    public void testDuplicateSecurity(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getRandomAccount();
        final String wrongAnswer = "wrong";
        final String rightAnswer = OriginClientData.ACCOUNT_PRIVACY_SECURITY_ANSWER;
        
        logFlowPoint("Navigate to Origin webpage in browser, log in and then go to the 'Account Settings' page."); //1
        logFlowPoint("Navigate to the 'Security' tab."); //2
        logFlowPoint("Enter a wrong answer to the security question and verify a warning appeared."); //3
        logFlowPoint("Answer the security question correctly and verify the 'Account Security' page is reached."); //4
        logFlowPoint("Go to the 'Security Question' tab"); //5
        logFlowPoint("Click 'Add Another Question, choose the same question as the first one and verify a warning appears.'"); //6
        logFlowPoint("Enter an answer to the second question, click save and verify the answer is not saved."); //7
        
        // 1
        WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, userAccount);
        AccountSettingsPage accountSettingsPage = new AccountSettingsPage(driver);
        accountSettingsPage.navigateToIndexPage();
        if (accountSettingsPage.verifyAccountSettingsPageReached()) {
            logPass("Successfully verified the user logged in and navigated to the 'Account Settings' page.");
        } else {
            logFailExit("Failed: Could not verify the user logged in and navigated to the 'Account Settings' page.");
        }
        
        //2
        accountSettingsPage.gotoSecuritySection();
        SecurityPage securityPage = new SecurityPage(driver);
        if (securityPage.verifySecuritySectionReached()) {
            logPass("Successfully navigated to the 'Security' tab.");
        } else {
            logFailExit("Failed to navigate to the 'Security' tab.");
        }
        
        //3
        securityPage.clickEditButton();
        SecurityQuestionDialog securityQuestionDialog = new SecurityQuestionDialog(driver);
        MacroAccount.answerSecurityQuestion(driver, wrongAnswer);
        if (securityQuestionDialog.verifyWrongAnswerWarningDialog()) {
            logPass("Successfully verified a warning appeared after entering a wrong answer to the Security Question.");
        } else {
            logFailExit("Failed to verify a warning appeared after entering a wrong answer to the Security Question.");
        }
        
        //4
        MacroAccount.answerSecurityQuestion(driver, rightAnswer);
        AccountSecurityDialog accountSecurityDialog = new AccountSecurityDialog(driver);
        if (accountSecurityDialog.verifyAccountSecurityDialogReached()) {
            logPass("Successfully answered the Security Question.");
        } else {
            logFailExit("Failed to answer the Security Question.");
        }
        
        //5
        accountSecurityDialog.navigateToSecurityQuestionTab();
        if (accountSecurityDialog.verifySecurityQuestionTabReached()) {
            logPass("Successfully navigated to the 'Security Questions' tab.");
        } else {
            logFailExit("Failed to navigate to the 'Security Questions' tab.");
        }
        
        //6
        accountSecurityDialog.addAnotherQuestion();
        int numberOfSecurityQuestions = accountSecurityDialog.getNumberOfSecurityQuestions();
        if (accountSecurityDialog.verifyDuplicatedQuestionWarningVisible(numberOfSecurityQuestions -1)) {
            logPass("Successfully entered the same security question and verified a warning appears.");
        } else {
            logFailExit("Failed to enter the same security question and verified a warning appears.");
        }
        
        //7
        if (accountSecurityDialog.verifyAnswerUnsaved(numberOfSecurityQuestions)) {
            logPass("Successfully entered the answers to the security questions and verified they are not saved.");
        } else {
            logFailExit("Failed to enter the answers to the security questions and verify they are not saved.");
        }
        softAssertAll();
    }
}