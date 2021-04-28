package com.ea.originx.automation.scripts.checkout;

import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.OriginAccessSettingsPage;
import com.ea.originx.automation.lib.pageobjects.dialog.AreYouSureDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.CancellationConfirmed;
import com.ea.originx.automation.lib.pageobjects.dialog.CancellationSurveyDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.WelcomeBackDialog;
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
 * Test clicking 'Rejoin' link on the 'Cancellation confirmation' dialog will
 * reactivate the user subscription
 *
 * @author cdeaconu
 */
public class OACancellationSurveyWelcomeBackModal extends EAXVxTestTemplate{
    
    @TestRail(caseId = 1376232)
    @Test(groups = {"checkout", "full_regression"})
    public void testCancellationSurveyWelcomeBackModal(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        
        logFlowPoint("Login as a newly registered user."); // 1
        logFlowPoint("Purchase an Origin Access subscription."); // 2
        logFlowPoint("Go to 'EA Account and Billing' web page and click 'Origin Access tab'."); // 3
        logFlowPoint("Click 'Cancel Membership' and verify the 'Are you sure' modal appears."); // 4
        logFlowPoint("Click 'Continue' and verify the 'Survey' modal opens."); // 5
        logFlowPoint("Click 'Complete Cancellation' and verify the 'Cancellation Confirmed' modal appears."); // 6
        logFlowPoint("Click 'Rejoin now' link and verify the 'Welcome Back' modal appears."); // 7
        logFlowPoint("Close the modal using the 'Close' button and verify the modal is closed."); // 8
        logFlowPoint("Refresh the EA Account page and verify the player is an active susbcriber"); // 9
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        
        // 3
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
        
        // 4
        originAccessSettingsPage.clickCancelMembership();
        if (!isClient) {
            driver.switchTo().window(tabs.get(0)); // switching back to the first tab
        }
        AreYouSureDialog areYouSureDialog = new AreYouSureDialog(browserDriver);
        logPassFail(areYouSureDialog.waitIsVisible(), true);
        
        // 5
        areYouSureDialog.clickContinueButton();
        CancellationSurveyDialog cancellationSurveyDialog = new CancellationSurveyDialog(browserDriver);
        logPassFail(cancellationSurveyDialog.waitIsVisible(), true);
        
        // 6
        cancellationSurveyDialog.clickCompleteCancellationCTA();
        CancellationConfirmed cancellationConfirmed = new CancellationConfirmed(browserDriver);
        logPassFail(cancellationConfirmed.waitIsVisible(), true);
        
        // 7
        cancellationConfirmed.clickRejoiningIsEasyLink();
        WelcomeBackDialog welcomeBackDialog = new WelcomeBackDialog(browserDriver);
        logPassFail(welcomeBackDialog.waitIsVisible(), true);
        
        // 8
        welcomeBackDialog.clickCloseCTA();
        logPassFail(welcomeBackDialog.waitIsClose(), true);
        
        // 9
        new MiniProfile(browserDriver).selectAccountBilling();
        tabs = new ArrayList<>(browserDriver.getWindowHandles());
        browserDriver.switchTo().window(tabs.get(1)); // switching to newly opened tab
        accountSettingsPage.gotoOriginAccessSection();
        logPassFail(originAccessSettingsPage.verifyCancelMembershipCTA(), true);
        
        softAssertAll();
    }
}