package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.OriginAccessSettingsPage;
import com.ea.originx.automation.lib.pageobjects.dialog.EditPaymentDialog;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.webdrivers.BrowserType;
import java.util.ArrayList;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test users ability to edit 'Paypal' payment method from the account portal
 *
 * @author cdeaconu
 */
public class OAEditPaymentMethodPaypal extends EAXVxTestTemplate{
    
    public void testEditPaymentMethodPaypal(ITestContext context, boolean premierSubscriber) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        UserAccount userAccount;
        String accountType;
        if (premierSubscriber) {
            userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.PREMIER_SUBSCRIPTION_ACTIVE_NO_ENTITLEMENTS);
            accountType = "premier";
        } else {
            userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE_NO_ENTITLEMENTS);
            accountType = "basic";
        }
        
        logFlowPoint("Launch Origin and login as an account with a " + accountType + " subscription"); // 1
        logFlowPoint("Open the account settings page, navigate to the 'Origin Access' section"); // 2
        logFlowPoint("Click on the payment method 'Edit' link. "); // 3
        logFlowPoint("Verify that the user can select PayPal as a the method in all countries that have PayPal as a method"); // 4
        logFlowPoint("Click the paypal selector and verify that the modal changes to reflect the payment method"); // 5
        logFlowPoint("Click the 'Set up your paypal account' CTA and verify the PayPal flow mirrors the existing store flow for paypal."); // 6
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        final WebDriver browserDriver;
        ArrayList<String> tabs = null;
        if (isClient) {
            browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
            MacroAccount.loginToAccountPage(browserDriver, userAccount);
        } else {
            browserDriver = driver;
            new MiniProfile(browserDriver).selectAccountBilling();
            tabs = new ArrayList<>(driver.getWindowHandles());
            driver.switchTo().window(tabs.get(1)); // switching to newly opened tab
        }
        AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.gotoOriginAccessSection();
        OriginAccessSettingsPage originAccessSettingsPage = new OriginAccessSettingsPage(browserDriver);
        logPassFail(originAccessSettingsPage.verifyEditPaymentLinkVisible(), true);
        
        // 3
        originAccessSettingsPage.clickEditPaymentLink();
        EditPaymentDialog editPaymentDialog = new EditPaymentDialog(browserDriver);
        logPassFail(editPaymentDialog.verifyEditPaymentDialogReached(), true);
        
        // 4
        editPaymentDialog.waitAndSwitchToEditPaymentDialog();
        logPassFail(editPaymentDialog.verifyPaymentMethods(), true);
        
        // 5
        editPaymentDialog.clickPayPalRadioButton();
        logPassFail(editPaymentDialog.verifySetUpPayPalAccountButtonVisible(), true);
        
        // 6
        editPaymentDialog.clickSave();
        logPassFail(originAccessSettingsPage.verifyPayPalWalletInfoSectionVisible(), true);
        
        softAssertAll();
    }
}