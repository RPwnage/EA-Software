package com.ea.originx.automation.scripts.checkout;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.OriginAccessSettingsPage;
import com.ea.originx.automation.lib.pageobjects.dialog.EditPaymentDialog;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.utils.Waits;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests editing the payment method for a subscription
 *
 * @author micwang
 */
public class OAEditPaymentMethod extends EAXVxTestTemplate {

    @TestRail(caseId = 11910)
    @Test(groups = {"full_regression", "checkout"})
    public void testEditPaymentMethod(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();
        WebDriver driver = startClientObject(context, client);

        logFlowPoint("Log into 'Origin' with a newly registered user"); //1
        logFlowPoint("Purchase 'Origin Access'"); //2
        logFlowPoint("Navigate to 'EA Account and Billing' and Navigate to 'Origin Access' tab"); //3
        logFlowPoint("Verify on clicking the 'Edit' Link, the 'Edit Payment' dialog appears"); //4
        logFlowPoint("Verify that only subscription supported payment methods appear as options on the dialog"); //5
        logFlowPoint("Verify that below the payment method selection, there is copy informing the user " +
                "that the subscription is recurring, and of the next billing date"); //6
        logFlowPoint("Verify that there is copy above the 'Save' button that informs the user about the recurring nature of the subscription"); //7
        logFlowPoint("Verify that pressing 'Cancel' does not update the payment method selected for subscription"); //8
        logFlowPoint("Re-enter the edit billing flow by following the steps above, change the billing information used for the account, " +
                "verify that the 'Payment Method' field updates correctly to the new account being used after saving"); //9

        //1
        String username = userAccount.getUsername();
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged in with a newly registered user: " + username);
        } else {
            logFailExit("Failed to login with user: " + username);
        }

        //2
        if (MacroOriginAccess.purchaseOriginAccess(driver)) {
            logPass("Successfully purchased 'Origin Access'.");
        } else {
            logFailExit("Failed to purchase 'Origin Access'.");
        }

        //3
        AccountSettingsPage accountSettingsPage;
        if (ContextHelper.isOriginClientTesing(context)) {
            driver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
            accountSettingsPage = new AccountSettingsPage(driver);
            accountSettingsPage.navigateToIndexPage();
            accountSettingsPage.login(userAccount);
        } else {
            MiniProfile miniProfile = new MiniProfile(driver);
            miniProfile.waitForJQueryAJAXComplete();
            miniProfile.selectAccountBilling();
            // wait and switch to the new tab opened
            Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.ACCOUNT_SETTINGS_PAGE_URL_REGEX, 30);
            Waits.sleep(3000);
            accountSettingsPage = new AccountSettingsPage(driver);
        }
        Waits.pollingWait(accountSettingsPage::verifyAccountSettingsPageReached);
        accountSettingsPage.navigateToIndexPage(AccountSettingsPage.NavLinkType.ORIGIN_ACCESS);
        OriginAccessSettingsPage originAccessSettingsPage = new OriginAccessSettingsPage(driver);
        if (Waits.pollingWait(originAccessSettingsPage::verifyOriginAccessSectionReached)) {
            logPass("Successfully navigated to 'Origin Access' tab");
        } else {
            logFailExit("Failed to navigate to 'Origin Access' tab");
        }

        //4
        MacroOriginAccess.openAndSwitchToEditPaymentDialog(driver);
        EditPaymentDialog editPaymentDialog = new EditPaymentDialog(driver);
        if (Waits.pollingWait(editPaymentDialog::verifyEditPaymentDialogReached)) {
            logPass("Verified on clicking the 'Edit' Link, the 'Edit Payment' dialog appears");
        } else {
            logFailExit("Failed: on clicking the 'Edit' Link, the 'Edit Payment' dialog did not appear");
        }

        //5
        if (editPaymentDialog.verifyPaymentMethods()) {
            logPass("Verified that only subscription supported payment methods appear as options on the dialog");
        } else {
            logFailExit("Failed: subscription supported payment methods do not appear as options on the dialog");
        }

        //6
        if (editPaymentDialog.verifySubInfo()) {
            logPass("Verified that below the payment method selection, there is copy informing the user that the subscription is recurring, and of the next billing date");
        } else {
            logFailExit("Failed: below the payment method selection, there is no copy informing the user that the subscription is recurring, and of the next billing date");
        }

        //7
        if (editPaymentDialog.verifyDescriptionCorrect()) {
            logPass("Verified that there is copy above the Save button that informs the user about the recurring nature of the subscription");
        } else {
            logFailExit("Failed: there is no copy above the Save button that informs the user about the recurring nature of the subscription");
        }

        //8
        OriginAccessSettingsPage originAccessSettingsPage2 = new OriginAccessSettingsPage(driver);
        editPaymentDialog.clickCancel();
        // close dialog to return to payment method section
        boolean isOriginAccessSectionReached = Waits.pollingWait(originAccessSettingsPage2::verifyOriginAccessSectionReached);
        boolean isCreditCardNotUpdated = originAccessSettingsPage2.verifyCreditCardNumber(OriginClientData.CREDIT_CARD_NUMBER);
        if (isOriginAccessSectionReached && isCreditCardNotUpdated) {
            logPass("Verified that pressing 'Cancel' does not update the payment method selected for subscription");
        } else {
            logFailExit("Failed: closing 'Edit Payment' dialog ended with error or pressing 'Cancel' updated the payment method selected for subscription");
        }

        //9
        MacroOriginAccess.openAndSwitchToEditPaymentDialog(driver);
        EditPaymentDialog editPaymentDialog1 = new EditPaymentDialog(driver);
        editPaymentDialog1.clickAddNewPaymentInfo();
        editPaymentDialog1.enterCreditCardDetailsDefaultValues(OriginClientData.ALTERNATE_CREDIT_CARD_NUMBER);
        OriginAccessSettingsPage originAccessSettingsPage3 = new OriginAccessSettingsPage(driver);
        boolean isPaymentMethodUpdated = Waits.pollingWait(originAccessSettingsPage3::verifyPaymentMethodUpdateSucessMsg);
        boolean isCreditCardUpdated = originAccessSettingsPage3.verifyCreditCardNumber(OriginClientData.ALTERNATE_CREDIT_CARD_NUMBER);
        if (isPaymentMethodUpdated && isCreditCardUpdated) {
            logPass("Verified that the 'Payment Method' field updates correctly to the new account being used after saving");
        } else {
            logFailExit("Failed: error occurred updating payment method or the 'Payment Method' field did not update correctly to the new account being used after saving");
        }

        softAssertAll();
    }
}
