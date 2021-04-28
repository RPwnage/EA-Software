package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Basic test flow for purchasing Origin Access
 *
 * @author cbalea
 */
public class OAPurchaseOriginAccess extends EAXVxTestTemplate{

    @TestRail(caseId = 3121917)
    @Test(groups = {"long_smoke", "originaccess"})
    public void purchaseOriginAccess(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        String username = userAccount.getUsername();
        
        logFlowPoint("Launch Origin and log in with a new account"); // 1
        logFlowPoint("Purchase 'Origin Access'"); // 2
        logFlowPoint("Click 'Skip and go to Vault Page' link and verify user is navigated to 'Vault Page' "); // 3
        logFlowPoint("Click on any vault tile and click 'Add to Game Library' then verify game is added to the library"); // 4
        
        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully log into Origin with user: " + username);
        } else {
            logFailExit("Failed to log into Origin with user: " + username);
        }
        
        // 2
        if(MacroOriginAccess.purchaseOriginAccess(driver)){
            logPass("Successfully purchase 'Origin Access'");
        } else {
            logFailExit("Failed to purchase 'Origin Access'");
        }
        
        // 3
        VaultPage vaultPage = new VaultPage(driver);
        vaultPage.waitForGamesToLoad();
        if(vaultPage.verifyPageReached()){
            logPass("Successfully reached 'Vault Page'");
        } else {
            logFail("Failed to reach 'Vault Page'");
        }
        
        // 4
        String offerID = vaultPage.getFirstVaultTileOfferId();
        MacroOriginAccess.addEntitlementAndHandleDialogs(driver, offerID);
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPageToLoad();
        if(new GameTile(driver, offerID).verifyOfferID()){
            logPass("Successfully verified entitlement is present in 'Game Library'");
        } else {
            logFailExit("Failed to verify entitlement presence in 'Game Library'");
        }
    }
}
