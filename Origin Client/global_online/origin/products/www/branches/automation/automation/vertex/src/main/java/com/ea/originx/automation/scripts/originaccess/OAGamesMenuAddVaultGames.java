package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
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
 * Test clicking 'Add Origin Access Vault games' from the 'Games' dropdown
 * redirect the user to the 'Vault' page.
 *
 * @author cdeaconu
 */
public class OAGamesMenuAddVaultGames extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3064015)
    @Test(groups = {"originaccess", "client_only", "full_regression"})
    public void testOAGamesMenuAddVaultGames (ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE);
        
        logFlowPoint("Login with a subscription account."); // 1
        logFlowPoint("Click on the 'Games' drop down menu on the top left and verify that the user can select 'Add Origin Access Vault game'."); // 2
        logFlowPoint("Click 'Add Origin Access Vault games' and verify the user is taken to the 'Vault' landing page."); // 3
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        MainMenu mainMenu = new MainMenu(driver);
        boolean isAddFromVaultInGamesMenu = mainMenu.verifyItemExistsInGames("Add Origin Access Vault game");
        logPassFail(isAddFromVaultInGamesMenu, true);
        
        // 3
        mainMenu.selectAddVaultGame();
        logPassFail(new VaultPage(driver).verifyPageReached(), true);
        
        softAssertAll();
    }
}