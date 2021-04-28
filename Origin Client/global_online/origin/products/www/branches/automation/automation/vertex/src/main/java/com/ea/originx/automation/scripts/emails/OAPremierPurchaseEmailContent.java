package com.ea.originx.automation.scripts.emails;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.EmailFetcher;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;
import javax.mail.Message;
import java.util.Map;

/**
 * Test new OA membership email content
 *
 * @author dchalasani
 */
public class OAPremierPurchaseEmailContent extends EAXVxTestTemplate {

    @TestRail(caseId = 3001045)
    @Test(groups = {"gifting", "emails", "full_regression", "int_only"})
    public void testOAPremierPurchaseEmailContent(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();
        final String userAccountName = userAccount.getUsername();

        logFlowPoint("Login as a newly registered user"); //1
        logFlowPoint("Purchase 'Origin Access' monthly subscription"); //2
        logFlowPoint("Navigate to the email that is associated with the new subscription and verify that an email was received"); //3
        logFlowPoint("Verify that you can retrieve the email fields in the new OA subscription email"); //4
        logFlowPoint("Verify that the Purchase email has a subject line"); //5
        logFlowPoint("Verify that the EA id username is displayed in the purchase email"); //6
        logFlowPoint("Verify that the payment method is listed in the purchase email"); //7
        logFlowPoint("Verify that in the body, there is a URL to log into Origin"); //8
        logFlowPoint("Verify that in the body, there is a URL to download the Origin client"); //9
        logFlowPoint("Verify that in the body, there is a URL to log into the account portal"); //10
        logFlowPoint("Verify that the help link directs the user to the main EA help site"); //11
        logFlowPoint("Verify that the Terms of Sale text with hyperlink is displayed"); //12
        logFlowPoint("Verify that the Origin Access Terms and Conditions text is displayed and hyperlinked in the footer"); //13
        logFlowPoint("Verify that the Seller text is displayed in the footer"); //14
        logFlowPoint("Verify that the Privacy policy link is displayed in the footer email"); //15
        logFlowPoint("Verify that the User Agreement link is displayed in the footer email"); //16
        logFlowPoint("Verify that the Legal link is displayed in the footer email"); //17

        WebDriver driver = startClientObject(context, client);

        //1
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        logPassFail(MacroOriginAccess.purchaseOriginPremierAccess(driver), true);

        //3
        EmailFetcher emailFetcher = new EmailFetcher(userAccountName);
        Message message = emailFetcher.getLatestNewOAMembershipEmail();
        logPassFail(message != null,true);

        emailFetcher.closeConnection();

        //4
        EmailFetcher userEmailFetcher = new EmailFetcher(userAccountName);
        Map<String, String> newMembershipEmailFields = userEmailFetcher.getLatestOANewMembershipEmailFields();
        logPassFail(newMembershipEmailFields.size() > 0, false);

        userEmailFetcher.closeConnection();

        String subjectLine = newMembershipEmailFields.get("Subject Line");
        String EAIdUsername = newMembershipEmailFields.get("EA Id Username");
        String paymentMethod = newMembershipEmailFields.get("Payment method");
        String signInOriginURL = newMembershipEmailFields.get("Sign in URL");
        String downloadOriginURL = newMembershipEmailFields.get("Download it here");
        String EAAccountURL = newMembershipEmailFields.get("EA Account");
        String helpLink = newMembershipEmailFields.get("help");
        String termOfSaleLink = newMembershipEmailFields.get("Terms of Sale");
        String termsAndConditionsLink = newMembershipEmailFields.get("Terms and Conditions");
        String sellerAddress = newMembershipEmailFields.get("Seller Address");
        String privacyPolicyLink = newMembershipEmailFields.get("Privacy Policy");
        String userAgreementLink = newMembershipEmailFields.get("User Agreement");
        String legalLink = newMembershipEmailFields.get("Legal");

        //5
        logPassFail(subjectLine != null, false);

        //6
        logPassFail(EAIdUsername != null, false);

        //7
        logPassFail(paymentMethod != null, false);

        //8
        logPassFail(signInOriginURL != null, false);

        //9
        logPassFail(downloadOriginURL != null, false);

        //10
        logPassFail(EAAccountURL != null, false);

        // 11
        String httpResponse = TestScriptHelper.getHttpResponse(helpLink);
        logPassFail(!httpResponse.equals("404") && !httpResponse.equals("Server DNS not found"), false);

        //12
        httpResponse = TestScriptHelper.getHttpResponse(termOfSaleLink);
        logPassFail(!httpResponse.equals("404") && !httpResponse.equals("Server DNS not found"), false);

        //13
        httpResponse = TestScriptHelper.getHttpResponse(termsAndConditionsLink);
        logPassFail(!httpResponse.equals("404") && !httpResponse.equals("Server DNS not found"), false);

        //14
        logPassFail(sellerAddress != null, false);

        //15
        httpResponse = TestScriptHelper.getHttpResponse(privacyPolicyLink);
        logPassFail(!httpResponse.equals("404") && !httpResponse.equals("Server DNS not found"), false);


        //16
        httpResponse = TestScriptHelper.getHttpResponse(userAgreementLink);
        logPassFail(!httpResponse.equals("404") && !httpResponse.equals("Server DNS not found"), false);

        //17
        httpResponse = TestScriptHelper.getHttpResponse(legalLink);
        logPassFail(!httpResponse.equals("404") && !httpResponse.equals("Server DNS not found"), false);

        softAssertAll();
    }
}