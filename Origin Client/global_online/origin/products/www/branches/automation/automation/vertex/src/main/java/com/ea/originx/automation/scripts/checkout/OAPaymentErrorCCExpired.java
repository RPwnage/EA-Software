package com.ea.originx.automation.scripts.checkout;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.OriginAccessSettingsPage;
import com.ea.originx.automation.lib.pageobjects.dialog.EditPaymentDialog;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.webdrivers.BrowserType;
import java.util.ArrayList;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test a subscriber account with an expired credit card attached can update the
 * payment option
 *
 * @author cdeaconu
 */
public class OAPaymentErrorCCExpired extends EAXVxTestTemplate{
    
    @TestRail(caseId = 1376232)
    @Test(groups = {"checkout", "full_regression"})
    public void testPaymentErrorCCExpired(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_CREDIT_EXPIRED);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        
        logFlowPoint("Login to Origin with a user with an Origin Access subscription and an expired credit card."); // 1
        logFlowPoint("Go to 'EA Account and Billing' web page and click 'Origin Access' tab."); // 2
        logFlowPoint("Verify an error message appears due to a credit card's expiration."); // 3
        logFlowPoint("Click 'Update payment' link and verify that 'Update Payment' dialog appears."); // 4
        
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
        logPassFail(originAccessSettingsPage.verifyCancelMembershipCTA(), true);
        
        // 3
        logPassFail(originAccessSettingsPage.verifyCreditCardExpiredMsg(), true);
        
        // 4
        originAccessSettingsPage.clickUpdatePaymentLink();
        EditPaymentDialog editPaymentDialog = new EditPaymentDialog(browserDriver);
        logPassFail(editPaymentDialog.verifyEditPaymentDialogReached(), true);
        
        softAssertAll();
    }
}