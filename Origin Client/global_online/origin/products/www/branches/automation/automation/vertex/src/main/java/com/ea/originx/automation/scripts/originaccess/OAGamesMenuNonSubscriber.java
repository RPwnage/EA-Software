package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Verify 'Games' menu doesn't have 'add from the vault' item visible for a
 * non-subscriber account
 *
 * @author cdeaconu
 */
public class OAGamesMenuNonSubscriber extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3064016)
    @Test(groups = {"originaccess", "client_only", "full_regression"})
    public void testGamesMenuNonSubscriber(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_EXPIRED);
        
        logFlowPoint("Log into with a non-subscriber account."); // 1
        logFlowPoint("Click on the 'Games' drop down and verify the 'add from the vault' item is not visible."); // 2
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(!new MainMenu(driver).verifyItemExistsInGames("Add Origin Access Vault game"), true);
        
        softAssertAll();
    }
}