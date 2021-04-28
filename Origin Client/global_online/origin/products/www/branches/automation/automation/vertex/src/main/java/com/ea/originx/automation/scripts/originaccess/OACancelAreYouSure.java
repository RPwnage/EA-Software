package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.OriginAccessSettingsPage;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.dialog.AreYouSureDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.CancellationSurveyDialog;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.webdrivers.BrowserType;
import java.util.ArrayList;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the "Are You Sure" modal which appears when cancelling an Origin Access
 * subscription
 *
 * @author glivingstone
 */
public class OACancelAreYouSure extends EAXVxTestTemplate {

    @TestRail(caseId = 1376227)
    @Test(groups = {"originaccess", "full_regression"})
    public void testPurchaseEntitlement(ITestContext context) throws Exception {
        
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        final OriginClient client = OriginClientFactory.create(context);
        UserAccount account = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();

        logFlowPoint("Login to Origin and purchase Origin Access"); // 1
        logFlowPoint("Open the account settings page, navigate to the Origin Access section, and select 'Cancel'"); // 2
        logFlowPoint("Verify the header exists on the dialog"); // 3
        logFlowPoint("Verify the sub-header exists on the dialog"); // 4
        logFlowPoint("Verify the player is informed their cancellation will take effect after the next billing date"); // 5
        logFlowPoint("Verify the next billing date is shown"); // 6
        logFlowPoint("Verify the player is informed they may rejoin at any time"); // 7
        logFlowPoint("Verify there is a 'Continue' CTA"); // 8
        logFlowPoint("Verify there is a 'Close' CTA"); // 9
        logFlowPoint("Click close and verify the modal is closed"); // 10
        logFlowPoint("Go to the account settings and verify the subscription is still active"); // 11
        logFlowPoint("Click the 'X' button and verify the modal is closed"); // 12
        logFlowPoint("Go to the account settings and verify the subscription is still active"); // 13
        logFlowPoint("Click outside the modal and verify the modal is closed"); // 14
        logFlowPoint("Go to the account settings and verify the subscription is still active"); // 15
        logFlowPoint("Click 'Continue' and verify the user is brought to the 'Cancel Survey Questionairre'"); // 16

        // 1
        WebDriver driver = startClientObject(context, client);
        boolean isLoggedIn = MacroLogin.startLogin(driver, account);
        boolean isOriginAccessPurchased = MacroOriginAccess.purchaseOriginAccess(driver);
                
        logPassFail(isLoggedIn && isOriginAccessPurchased, true);

        // 2
        final WebDriver browserDriver;
        ArrayList<String> tabs = null;
        if (isClient) {
            browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
            MacroAccount.loginToAccountPage(browserDriver, account);
        } else {
            browserDriver = driver;
            new MiniProfile(driver).selectAccountBilling();
            tabs = new ArrayList<>(driver.getWindowHandles());
            driver.switchTo().window(tabs.get(1)); // switching to newly opened tab
        }
        AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.gotoOriginAccessSection();
        OriginAccessSettingsPage accessSettings = new OriginAccessSettingsPage(browserDriver);
        accessSettings.clickCancelMembership();
        if (!isClient) {
            driver.switchTo().window(tabs.get(0)); // switching back to the first tab
        }
        AreYouSureDialog areYouSure = new AreYouSureDialog(browserDriver);
        logPassFail(areYouSure.waitIsVisible(), true);

        // 3
        logPassFail(areYouSure.verifyTitleExists(), false);

        // 4
        logPassFail(areYouSure.verifySubHeaderExists(), false);

        // 5
        boolean isBodyVisible = areYouSure.verifyBodyExists();
        boolean isEndDateMessageVisible = areYouSure.verifyEndDateMessage();
        logPassFail(isBodyVisible && isEndDateMessageVisible, false);

        // 6
        logPassFail(areYouSure.verifyEndDate(), false);

        // 7
        logPassFail(areYouSure.verifyMayRejoinMessage(), false);

        // 8
        logPassFail(areYouSure.verifyContinueButtonExists(), false);

        // 9
        logPassFail(areYouSure.verifyCloseButtonExists(), true);

        // 10
        areYouSure.clickCloseButton();
        boolean isAreYouSureClosed = areYouSure.waitIsClose();
        logPassFail(isAreYouSureClosed, true);

        // 11
        accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.navigateToIndexPage();
        accountSettingsPage.gotoOriginAccessSection();
        boolean isMembershipActive = accessSettings.isMembershipActive();
        accessSettings.clickCancelMembership();
        areYouSure = new AreYouSureDialog(browserDriver);
        boolean isAreYouSureReached = areYouSure.waitIsVisible();
        logPassFail(isMembershipActive && isAreYouSureReached, false);

        // 12
        areYouSure.clickCloseCircle();
        isAreYouSureClosed = areYouSure.waitIsClose();
        logPassFail(isAreYouSureClosed, true);

        // 13
        accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.navigateToIndexPage();
        accountSettingsPage.gotoOriginAccessSection();
        isMembershipActive = accessSettings.isMembershipActive();
        accessSettings = new OriginAccessSettingsPage(browserDriver);
        accessSettings.clickCancelMembership();
        areYouSure = new AreYouSureDialog(browserDriver);
        areYouSure.waitIsVisible();
        isAreYouSureReached = areYouSure.waitIsVisible();
        logPassFail(isMembershipActive && isAreYouSureReached, false);

        // 14
        new GlobalSearch(browserDriver).actionClickGlobalSearch();
        isAreYouSureClosed = areYouSure.waitIsClose();
        logPassFail(isAreYouSureClosed, true);

        // 15
        accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.navigateToIndexPage();
        accountSettingsPage.gotoOriginAccessSection();
        isMembershipActive = accessSettings.isMembershipActive();
        accessSettings = new OriginAccessSettingsPage(browserDriver);
        accessSettings.clickCancelMembership();
        areYouSure = new AreYouSureDialog(browserDriver);
        areYouSure.waitIsVisible();
        isAreYouSureReached = areYouSure.waitIsVisible();
        logPassFail(isMembershipActive && isAreYouSureReached, false);

        // 16
        areYouSure.clickContinueButton();
        logPassFail(new CancellationSurveyDialog(browserDriver).verifyTitleExists(), true);
        softAssertAll();
    }
}
