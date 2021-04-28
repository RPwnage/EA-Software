package com.ea.originx.automation.scripts.accountandbilling;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.OrderHistoryPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;
import com.ea.vx.annotations.TestRail;

/**
 * Test 'Order History' section of 'EA Account and Billing' page by purchasing a
 * game
 *
 * @author palui
 */
public class OAOrderHistory extends EAXVxTestTemplate {

    @TestRail(caseId = 9601)
    @Test(groups = {"accountandbilling", "full_regression", "int_only"})
    public void testOrderHistory(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        String username = userAccount.getUsername();

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        String entitlementName = entitlement.getName();

        logFlowPoint("Launch Origin and login as newly registered user: " + username); //1
        logFlowPoint(String.format("Purchase '%s'", entitlementName)); //2
        logFlowPoint("From a browser, go to 'EA Account and Billing' page"); //3
        logFlowPoint("Navigate to the 'Order History' section"); //4
        logFlowPoint("Verify 'Order History' section shows exactly one 'Order History Line'"); //5
        logFlowPoint(String.format("Verify 'Order History Line' shows entitlement name '%s' in its description", entitlementName)); //6

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful as newly registered user: " + username);
        } else {
            logFailExit("Failed: Cannot login as newly registered user: " + username);
        }

        //2
        if (MacroPurchase.purchaseEntitlement(driver, entitlement)) {
            logPass(String.format("Verified successful purchase of '%s'", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot purchase '%s'", entitlementName));
        }

        //3
        WebDriver browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        final AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.navigateToIndexPage();
        accountSettingsPage.login(userAccount);
        if (accountSettingsPage.verifyAccountSettingsPageReached()) {
            logPass("Verified 'EA Account and Billing' page opens at a browser");
        } else {
            logFailExit("Failed: Cannot open 'EA Account and Billing page' at a browser");
        }

        //4
        accountSettingsPage.gotoOrderHistorySection();
        OrderHistoryPage orderHistoryPage = new OrderHistoryPage(browserDriver);
        if (orderHistoryPage.verifyOrderHistorySectionReached()) {
            logPass("Verified 'Order History' section opens");
        } else {
            logFailExit("Failed: Cannot open 'Order History' section");
        }

        //5
        int numOrders = orderHistoryPage.countOrders();
        if (numOrders == 1) {
            logPass("Verified 'Order History' section has exactly one order line");
        } else {
            logFailExit("Failed: 'Order History' section does not have exactly one order line");
        }

        //6
        String orderDescription = orderHistoryPage.getOrderHistoryLine(1).getDescription().trim();
        if (entitlementName.equals(orderDescription)) {
            logPass(String.format("Verified 'Order History Line' shows entitlement name '%s' in its description", entitlementName));
        } else {
            logFailExit(String.format("Failed: 'Order History Line' shows '%s' instead of '%s' in its description", orderDescription, entitlementName));
        }

        softAssertAll();
    }
}
