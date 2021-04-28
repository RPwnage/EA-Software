package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.MyGiftsPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the MyAccount-MyGifts tab for unopened,opened gifts in the receiver
 *
 * @author nvarthakavi
 */
public class OAMyAccountMyGiftsTab extends EAXVxTestTemplate {

    @TestRail(caseId = 561399)
    @Test(groups = {"gifting", "full_regression", "int_only"})
    public void testMyAccountMyGiftsTab(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userUnopenedGifts = AccountManagerHelper.getTaggedUserAccountWithCountry(AccountTags.ONLY_UNOPENED_GIFTS, "Canada");
        final UserAccount userOpenedGifts = AccountManagerHelper.getTaggedUserAccountWithCountry(AccountTags.ONLY_OPENED_GIFTS, "Canada");
        final UserAccount userNoGifiting = AccountManagerHelper.getUnentitledUserAccountWithCountry("Canada");
        final String userUnopenedGiftsName = userUnopenedGifts.getUsername();
        final String userOpenedGiftsName = userOpenedGifts.getUsername();
        final String userNoGifitingName = userNoGifiting.getUsername();

        logFlowPoint(String.format("Launch Origin and login as user '%s' with only unopened gifts and Navigate to MyAccount-My Gifts Page", userUnopenedGiftsName)); // 1
        logFlowPoint(String.format("Verify in My Gift Page the unopened gift section appear and opened gift section states 'No Opened Gift to display' for '%s' ", userUnopenedGiftsName)); // 2
        logFlowPoint(String.format("Launch Origin and login as user '%s' with only opened gifts and Navigate to MyAccount-My Gifts Page", userOpenedGiftsName)); // 3
        logFlowPoint(String.format("Verify in My Gift Page the unopened gift section does not appear and opened gift section appears for '%s' ", userOpenedGiftsName)); // 4
        logFlowPoint(String.format("Launch Origin and login as user '%s' with no gifting activity and Navigate to MyAccount Page", userNoGifitingName)); //5
        logFlowPoint(String.format("Verify in My-Account Page there is no My Gifts Section for '%s' ", userNoGifitingName)); // 6

        // 1
        WebDriver driver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        AccountSettingsPage accountSettingsPage1 = new AccountSettingsPage(driver);
        accountSettingsPage1.navigateToIndexPage();
        accountSettingsPage1.login(userUnopenedGifts);
        accountSettingsPage1.navigateToMyGiftsPage();
        MyGiftsPage myGiftsPageUnopened = new MyGiftsPage(driver);
        boolean accountMyGiftOpensForUnopened = myGiftsPageUnopened.verifyMyGiftsPageSectionReached();
        if (accountMyGiftOpensForUnopened) {
            logPass("Successfully logged into " + userUnopenedGiftsName + " and navigated to MyAccount-My Gifts Page");
        } else {
            logFailExit("Failed to log into " + userUnopenedGiftsName + " and navigate to MyAccount-My Gifts Page ");
        }

        // 2
        boolean unopenedGifts = myGiftsPageUnopened.verifyUnopenedGifts();
        boolean noOpenedGifts = myGiftsPageUnopened.verifyNoOpenedGifts();
        if (unopenedGifts && noOpenedGifts) {
            logPass(String.format("Verified in My Gift Page the unopened gift section appear and opened gift section states 'No Opened Gift to display' for '%s'", userUnopenedGiftsName));
        } else {
            logFail(String.format("Failed: In My Gift Page the unopened gift section does not appear or opened gift section does not show a message 'No Opened Gift to diplay' for '%s'", userUnopenedGiftsName));
        }

        // 3
        client.stop();
        driver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        AccountSettingsPage accountSettingsPage2 = new AccountSettingsPage(driver);
        accountSettingsPage2.navigateToIndexPage();
        accountSettingsPage2.login(userOpenedGifts);
        accountSettingsPage2.navigateToMyGiftsPage();
        MyGiftsPage myGiftsPageOpened = new MyGiftsPage(driver);
        boolean accountMyGiftOpensForOpened = myGiftsPageOpened.verifyMyGiftsPageSectionReached();
        if (accountMyGiftOpensForOpened) {
            logPass("Successfully logged into " + userOpenedGiftsName + " and navigated to MyAccount-My Gifts Page");
        } else {
            logFailExit("Failed to log into " + userOpenedGiftsName + " and navigate to MyAccount-My Gifts Page ");
        }

        // 4
        boolean openedGifts = myGiftsPageOpened.verifyOpenedGifts();
        boolean noUnopenedGifts = !myGiftsPageOpened.verifyUnopenedGifts();
        if (openedGifts && noUnopenedGifts) {
            logPass(String.format("Verified in My Gift Page the unopened gift section does not appear and opened gift section appears for '%s' ", userOpenedGiftsName));
        } else {
            logFail(String.format("Failed: In My Gift Page the unopened gift section does appear or opened gift section does not appear for '%s'", userOpenedGiftsName));
        }

        // 5
        client.stop();
        driver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        AccountSettingsPage accountSettingsPage3 = new AccountSettingsPage(driver);
        accountSettingsPage3.navigateToIndexPage();
        accountSettingsPage3.login(userNoGifiting);
        boolean accountMyAccount = accountSettingsPage3.verifyAccountSettingsPageReached();
        if (accountMyAccount) {
            logPass("Successfully logged into " + userNoGifitingName + " and navigated to MyAccount Page");
        } else {
            logFailExit("Failed to log into " + userNoGifitingName + " and navigate to MyAccount Page ");
        }

        // 6
        boolean accountNoGifting = !accountSettingsPage3.verifySectionContext(AccountSettingsPage.NavLinkType.MY_GIFTS);
        if (accountNoGifting) {
            logPass(String.format("Verified there is no My Gifts Section in MyAccount Page for '%s'", userNoGifitingName));
        } else {
            logFailExit(String.format("Failed: There is My Gifts Section in MyAccount Page for '%s'", userNoGifitingName));
        }

        softAssertAll();

    }
}
