package com.ea.originx.automation.scripts.feature.checkout;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.OrderHistoryLine;
import com.ea.originx.automation.lib.pageobjects.account.OrderHistoryPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests if user can see refund request link from purchase history page after
 * purchasing base game
 *
 * @author rchoi
 */
public class OAEntitlementRefundRequest extends EAXVxTestTemplate {

    @TestRail(caseId = 3068177)
    @Test(groups = {"checkout_smoke", "checkout", "int_only", "allfeaturesmoke"})
    public void testEntitlementRefundRequest(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_18);
        final String entitlementName = entitlement.getName();

        logFlowPoint("Launch Origin and register as a new user");  //1
        logFlowPoint("Purchase entitlement from it's GDP page"); //2
        logFlowPoint("Open 'EA Account and Billing' page from Chrome browser"); //3
        logFlowPoint("Open 'Order History' page"); //4
        logFlowPoint("Verify top 'Order History Line' shows proper entitlement name and order number which is same as one on thank you page when purchasing " + entitlementName); //5
        logFlowPoint("Verify there is refund text and link for the order history line"); //6

        //1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        String orderNumberInThankYouPage = MacroPurchase.purchaseEntitlementReturnOrderNumber(driver, entitlement);
        logPassFail((orderNumberInThankYouPage != null), true);

        //3
        final WebDriver browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        final AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.navigateToIndexPage();
        accountSettingsPage.login(userAccount);
        logPassFail(accountSettingsPage.verifyAccountSettingsPageReached(), true);

        //4
        accountSettingsPage.gotoOrderHistorySection();
        OrderHistoryPage orderHistoryPage = new OrderHistoryPage(browserDriver);
        logPassFail(orderHistoryPage.verifyOrderHistorySectionReached(), true);

        //5
        OrderHistoryLine line = orderHistoryPage.getOrderHistoryLine(1);
        String orderDescription = line.getDescription().trim();
        String orderNumerInHistory = line.getOrderNumber().trim();
        logPassFail((entitlementName.contains(orderDescription) && orderNumberInThankYouPage.equals(orderNumerInHistory)), true);

        //6
        // clicking expand button for the order history line
        line.clickExpandButton();
        logPassFail(line.verifyRequestRefundTextLinkExist(), true);
        
        softAssertAll();
    }
}
