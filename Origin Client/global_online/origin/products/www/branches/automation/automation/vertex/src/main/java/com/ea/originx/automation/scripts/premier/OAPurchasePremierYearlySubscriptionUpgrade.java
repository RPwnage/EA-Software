package com.ea.originx.automation.scripts.premier;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

public class OAPurchasePremierYearlySubscriptionUpgrade extends EAXVxTestTemplate {

    @TestRail(caseId = 1016690)
    @Test(groups = {"checkout", "release_smoke", "int_only"})
    public void testPurchaseSubscriptionNUX(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();

        logFlowPoint("Login as a newly registered user"); //1
        logFlowPoint("Purchase 'Origin Access' monthly subscription"); //2
        logFlowPoint("Upgrade Subscription to Premier"); //3
        logFlowPoint("Click 'Go to the Vault' link in the NUX, and verify the user is navigated the 'Vault Page'"); //4

        final WebDriver driver = startClientObject(context, client);

        //1
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);

        //3
        logPassFail(MacroOriginAccess.purchaseOriginPremierAccess(driver), true);

        //4
        VaultPage vaultPage = new VaultPage(driver);
        logPassFail(vaultPage.verifyPageReached(), true);

        softAssertAll();
    }
}
