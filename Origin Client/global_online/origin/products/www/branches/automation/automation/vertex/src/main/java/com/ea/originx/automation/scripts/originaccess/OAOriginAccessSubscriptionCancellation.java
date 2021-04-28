package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.OriginAccessSettingsPage;
import com.ea.originx.automation.lib.pageobjects.dialog.AreYouSureDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.CancellationConfirmed;
import com.ea.originx.automation.lib.pageobjects.dialog.CancellationSurveyDialog;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
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
 * Tests the flow of canceling origin access subscription
 *
 * @author cbalea
 */
public class OAOriginAccessSubscriptionCancellation extends EAXVxTestTemplate {

    @TestRail(caseId = 11123)
    @Test(groups = {"originaccess", "full_regression"})
    public void testSubscriptionCancellation(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        logFlowPoint("Log into Origin and purchase 'Origin Access'"); // 1
        logFlowPoint("Hover over mini profile and click on 'EA Account and Billing' tab"); // 2
        logFlowPoint("Click on the 'Subscription' tab and verify there is a CTA to cancel the subscription"); // 3
        logFlowPoint("Click the 'Cancel Membership' CTA and verify user is brought back to Origin and a dialog appears"); // 4
        logFlowPoint("Verify that the dialog explains that cancelling the subscription means opting out of reccuring billing"); // 5
        logFlowPoint("Verify that the dialog explains that after user opts out of reccuring billing, the subscription is active until the end of the paid period"); // 6
        logFlowPoint("Click 'Continue' CTA and verify a dialog with cancellation subscription appears"); // 7
        logFlowPoint("Click 'Complete Cancellation' CTA and verify the user is shown a confirmation that they successfully canceled their subscription dialog"); // 8

        // 1
        WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, userAccount);
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), isClient);

        // 2
        final WebDriver browserDriver;
        ArrayList<String> tabs = null;
        if (isClient) {
            browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
            MacroAccount.loginToAccountPage(browserDriver, userAccount);
        } else {
            browserDriver = driver;
            new MiniProfile(driver).selectAccountBilling();
            tabs = new ArrayList<>(driver.getWindowHandles());
            driver.switchTo().window(tabs.get(1)); // switching to newly opened tab
        }
        AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);
        logPassFail(accountSettingsPage.verifyAccountSettingsPageReached(), isClient);

        // 3
        OriginAccessSettingsPage originAccessSettingsPage = new OriginAccessSettingsPage(browserDriver);
        accountSettingsPage.gotoOriginAccessSection();
        logPassFail(originAccessSettingsPage.verifyCancelMembershipCTA(), false);

        // 4
        originAccessSettingsPage.clickCancelMembership();
        if (!isClient) {
            driver.switchTo().window(tabs.get(0)); // switching back to the first tab
        }
        AreYouSureDialog areYouSure = new AreYouSureDialog(browserDriver);
        logPassFail(areYouSure.waitIsVisible(), true);

        // 5
        logPassFail(areYouSure.verifyEndDateMessage(), false);

        // 6
        logPassFail(areYouSure.verifySubcriptionActiveEndPeriodMessage(), false);;

        // 7
        areYouSure.clickContinueButton();
        CancellationSurveyDialog cancellationSurveyDialog = new CancellationSurveyDialog(browserDriver);
        cancellationSurveyDialog.waitForVisible();
        logPassFail(cancellationSurveyDialog.verifyTitleExists(), true);

        // 8
        cancellationSurveyDialog.clickCompleteCancellationCTA();
        CancellationConfirmed cancellationConfirmed = new CancellationConfirmed(browserDriver);
        cancellationConfirmed.waitForVisible();
        logPassFail(cancellationConfirmed.verifyConfirmationMessage(), true);
    }

}
