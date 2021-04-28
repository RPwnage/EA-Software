package com.ea.originx.automation.scripts.checkout;

import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * To tests the Purchase flow of an entitlement
 *
 * @author nvarthakavi
 */
public class OABasicPurchase extends EAXVxTestTemplate {

    @TestRail(caseId = 3068171)
    @Test(groups = {"checkout", "long_smoke","client_only"})
    public void testBasicPurchase(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final String userName = userAccount.getUsername();

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BIG_MONEY);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        logFlowPoint("Launch Origin and login as a newly registered user"); //1
        logFlowPoint("Purchase any base game entitlement"); //2
        logFlowPoint("Navigate to 'Game Library' and verify the entitlement appears in the 'Game Library' after purchase"); //3
        logFlowPoint("Download the entitlement and verify the entitlement is playable"); //4

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully created user and logged in as: " + userName);
        } else {
            logFailExit("Failed: to create/login to a user: " + userName);
        }

        //2
        if (MacroPurchase.purchaseEntitlement(driver, entitlement)) {
            logPass(String.format("Verified successful purchase of '%s'", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot purchase '%s'", entitlementName));
        }

        //3
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        if (gameLibrary.isGameTileVisibleByName(entitlementName)) {
            logPass(entitlementName + " appears in game Library after a successful purchase");
        } else {
            logFailExit(entitlementName + " did not appear in game library after successful purchase");
        }

        //4
        MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId);
        boolean isActivationAdded = entitlement.addActivationFile(client);
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        gameTile.play();
        sleep(10000); // wait 10 seconds for game to start
        boolean isLaunched = entitlement.isLaunched(client);
        if (isLaunched && isActivationAdded) {
            logPass(String.format("Verified '%s' launches successfully", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot launch '%s'", entitlementName));
        }
        entitlement.killLaunchedProcess(client);

        softAssertAll();
    }

}
