package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGifting;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.OrderHistoryLine;
import com.ea.originx.automation.lib.pageobjects.account.OrderHistoryPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.webdrivers.BrowserType;
import java.util.ArrayList;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test user can see gifting information from Order History from My Account page
 *
 * @author rocky
 * @author sbentley
 */
public class OAMyAccountSentGifts extends EAXVxTestTemplate {

    @TestRail(caseId = 11775)
    @Test(groups = {"gifting", "full_regression", "int_only"})
    public void testMyAccountSentGifts(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount gifter = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);
        final String gifterName = gifter.getUsername();

        final UserAccount recipient = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);
        final String recipientName = recipient.getUsername();

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String entitlementName = entitlement.getName();

        logFlowPoint("Login to Origin with an account that gifted a game");    //1
        logFlowPoint("Navigate to My Account pages");   //2
        logFlowPoint("Open 'Order History' section");   //3
        logFlowPoint("Locate the previous order and verify there is an entry for a gifted order by looking at 'Sent gift' text");    //5
        logFlowPoint("Expand the order information by clicking the '+' and  verify name of the entitlement is what gift sender sent");  //6
        logFlowPoint("Verify all order information for the order is present and correct");  //7

        gifter.addFriend(recipient);

        //1
        WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, recipient);
        new MiniProfile(driver).selectSignOut();
        logPassFail(MacroLogin.startLogin(driver, gifter), true);

        String orderNumber = MacroGifting.purchaseGiftForFriendReturnOrderNumber(driver, entitlement, recipientName);

        //2
        AccountSettingsPage accountSettingsPage;
        if (ContextHelper.isOriginClientTesing(context)) {
            driver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
            accountSettingsPage = new AccountSettingsPage(driver);
            accountSettingsPage.navigateToIndexPage();
            accountSettingsPage.login(gifter);
        } else {
            new MiniProfile(driver).selectAccountBilling();
            driver.switchTo().window(new ArrayList<String>(driver.getWindowHandles()).get(1));
            accountSettingsPage = new AccountSettingsPage(driver);
        }

        logPassFail(accountSettingsPage.verifyAccountSettingsPageReached(), true);

        //3
        accountSettingsPage.gotoOrderHistorySection();
        OrderHistoryPage orderHistoryPage = new OrderHistoryPage(driver);
        logPassFail(orderHistoryPage.verifyOrderHistorySectionReached(), true);

        //4
        // get latest order history line
        OrderHistoryLine line = orderHistoryPage.getOrderHistoryLine(1);
        logPassFail(line.verifySentGiftText(), true);

        //5
        line.clickExpandButton();
        logPassFail(line.verifyNameOfReceiver(recipientName), true);

        //6
        boolean isGameNameCorrect = line.verifyNameOfEntitlement(entitlementName);
        boolean isOrderNumberTheSame = line.getOrderNumber().equalsIgnoreCase(orderNumber);
        logPassFail(isGameNameCorrect && isOrderNumberTheSame, true);

        softAssertAll();
    }
}
