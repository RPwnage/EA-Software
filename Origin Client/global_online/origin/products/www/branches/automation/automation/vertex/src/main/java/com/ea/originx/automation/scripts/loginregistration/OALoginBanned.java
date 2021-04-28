package com.ea.originx.automation.scripts.loginregistration;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test for banned user login
 *
 * @author palui
 */
public final class OALoginBanned extends EAXVxTestTemplate {

    @TestRail(caseId = 9469)
    @Test(groups = {"loginregistration", "full_regression"})
    public void testLoginBanned(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount bannedAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.BANNED);
        String username = bannedAccount.getUsername();

        logFlowPoint("Launch Origin client and login as banned user: " + username); //1

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, bannedAccount)) {
            logPass("Verified login successful as banned user: " + username);
        } else {
            logFailExit("Failed: Cannot login as banned user: " + username);
        }

        softAssertAll();
    }

}
