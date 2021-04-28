package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.OriginAccessSettingsPage;
import com.ea.originx.automation.lib.pageobjects.dialog.EditPaymentDialog;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.utils.Waits;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import java.util.ArrayList;

/**
 * Test users ability to edit 'Credit Card' payment method from the account portal
 *
 * @author ivlim
 */
public class OAEditPaymentMethodCreditCard extends EAXVxTestTemplate{
    
    public void testEditPaymentMethodCreditCard(ITestContext context, boolean premierSubscriber) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount;
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        if (premierSubscriber) {
            userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.PREMIER_SUBSCRIPTION_ACTIVE_NO_ENTITLEMENTS);
        } else {
            userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE_NO_ENTITLEMENTS);
        }
        
        logFlowPoint("Launch Origin and login as newly registered account."); // 1
        logFlowPoint("Open the account settings page, navigate to the 'Origin Access' section"); // 2
        logFlowPoint("Click on the payment method 'Edit' link. "); // 3
        logFlowPoint("Click on the saved payment information drop down and click 'Add New...'"); // 4
        logFlowPoint("Input an incorrect credit card number and verify credit card number field is highlighted red"); // 5
        logFlowPoint("Input a correct credit card number and verify card type is visible on the right side of the credit card field."); // 6
        logFlowPoint("Verify that address fields match the existing store fields."); // 7
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        final WebDriver browserDriver;
        if (isClient) {
            browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
            MacroAccount.loginToAccountPage(browserDriver, userAccount);
        } else {
            browserDriver = driver;
            new MiniProfile(driver).selectProfileDropdownMenuItemByHref("eaAccounts");
            driver.switchTo().window(new ArrayList<>(driver.getWindowHandles()).get(1)); // switching to newly opened tab
        }
        AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.gotoOriginAccessSection();
        OriginAccessSettingsPage originAccessSettingsPage = new OriginAccessSettingsPage(browserDriver);
        logPassFail(originAccessSettingsPage.verifyModifyPaymentLinkVisible(), true);
        
        // 3
        originAccessSettingsPage.clickModifyPaymentLink();
        EditPaymentDialog editPaymentDialog = new EditPaymentDialog(browserDriver);
        logPassFail(editPaymentDialog.verifyEditPaymentDialogReached(), true);
        
        // 4
        editPaymentDialog.waitAndSwitchToEditPaymentDialog();
        editPaymentDialog.clickAddNewPaymentInfo();
        logPassFail(editPaymentDialog.verifyCreditCardNumberEmpty(), true);
        
        // 5
        editPaymentDialog.inputCreditCard(OriginClientData.INVALID_CREDIT_CARD_NUMBER);
        logPassFail(Waits.pollingWait(() -> editPaymentDialog.verifyCreditCardNumberShowsInvalid()), false);
        
        // 6
        editPaymentDialog.inputCreditCard(OriginClientData.CREDIT_CARD_NUMBER);
        logPassFail(Waits.pollingWait(() -> editPaymentDialog.verifyCreditCardTypeVisible()), false);

        // 7
        logPassFail(editPaymentDialog.verifyAddressFieldsShown(), false);
        
        softAssertAll();
    }
}