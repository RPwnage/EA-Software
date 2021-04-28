package com.ea.originx.automation.scripts.accountandbilling;

import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.account.PaymentAndShippingPage;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.utils.Waits;
import java.util.ArrayList;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test navigating to the 'Account and Billing' page from the 'Mini Profile'
 * dropdown and 'Origin' menu.
 *
 * @author palui
 */
public class OAAccountBilling extends EAXVxTestTemplate {
    
    public enum TEST_TYPE{
        ACCOUNT_BILLING_ORIGIN_MENU,
        ACCOUNT_BILLING_SIDEBAR
    }
    
    public void testAccountBilling(ITestContext context, OAAccountBilling.TEST_TYPE type) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        
        UserAccount userAccount = AccountManager.getRandomAccount();
        String username = userAccount.getUsername();
        
        logFlowPoint("Log into Origin"); // 1
        if (type == TEST_TYPE.ACCOUNT_BILLING_SIDEBAR){
            logFlowPoint("Navigate to the 'Account and Billing' page through the 'Mini Profile' dropdown and verify that the 'Payment Settings' page has been reached"); // 2
        }
        if (type == TEST_TYPE.ACCOUNT_BILLING_ORIGIN_MENU) {
            logFlowPoint("Navigate to the 'Account and Billing' page through the 'Main Menu' and verify that the 'Payment Settings' page has been reached."); // 2a
        }
        
        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful to user: " + username);
        } else {
            logFailExit("Failed: Cannot login to user: " + username);
        }
        
        //2
        MiniProfile miniProfile = new MiniProfile(driver);
        if(type == TEST_TYPE.ACCOUNT_BILLING_SIDEBAR){
            boolean paymentSettingsPage;
            if(isClient){
                miniProfile.hoverOverMiniProfile();
                miniProfile.selectAccountBilling();
                paymentSettingsPage = Waits.pollingWaitEx(() -> TestScriptHelper.verifyBrowserOpen(client));
                TestScriptHelper.killBrowsers(client);
            } else {
                miniProfile.selectAccountBilling();
                ArrayList<String> tabs = new ArrayList<>(driver.getWindowHandles());
                driver.switchTo().window(tabs.get(1)); // switching to newly opened tab
                PaymentAndShippingPage paymentAndShippingPage = new PaymentAndShippingPage(driver);
                boolean accountAndBilling = paymentAndShippingPage.verifyAccountSettingsPageReached();
                paymentAndShippingPage.gotoPaymentAndShippingSection();
                boolean paymentAndShipping = paymentAndShippingPage.verifyPaymentAndShippingSectionReached();
                paymentSettingsPage = accountAndBilling && paymentAndShipping;
                TestScriptHelper.killBrowsers(client);
            }
            if (paymentSettingsPage) {
                logPass("Verified the 'Payment Settings' page is accessible through the 'Mini Profile'");
            } else {
                logFail("Failed to verify the 'Payment Settings' page is accessible through the 'Mini Profile'");
            }
        }
        
        //2a
        if (type == TEST_TYPE.ACCOUNT_BILLING_ORIGIN_MENU) {
            new MainMenu(driver).selectAccountBilling();
            if (Waits.pollingWaitEx(() -> TestScriptHelper.verifyBrowserOpen(client))) {
                logPass("Verified the 'Payment Settings' page is accessible through the 'Main Menu'");
            } else {
                logFail("Failed to verify the 'Payment Settings' page is accessible through the 'Main Menu'");
            }
        }
       TestScriptHelper.killBrowsers(client);
        
        softAssertAll();
    }
}
