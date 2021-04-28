package com.ea.originx.automation.scripts.gamelibrary;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
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
 * Test a standard edition game have a text violator displayed in game library
 * and OGD for upgrading it
 *
 * @author cdeaconu
 */
public class OAGameEditionUpgradeViolator extends EAXVxTestTemplate{
    
    @TestRail(caseId = 10865)
    @Test(groups = {"gamelibrary", "client_only", "full_regression"})
    public void testGameEditionUpgradeViolator (ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String offerId = entitlement.getOfferId();
        
        logFlowPoint("Login as a newly registered user."); // 1
        logFlowPoint("Purchase the standard edition of an entitlement that has a higher edition on the vault."); // 2
        logFlowPoint("Purchase 'Origin Access'."); // 3
        logFlowPoint("Navigate to 'Game Library' and verify that the text violator 'Upgrade your game at no extra cost' is displayed in game tile."); // 4
        logFlowPoint("Navigate to the OGD and verify that the text 'Upgrade your game at no extra cost. Upgrade Now.' is displayed."); // 5
        logFlowPoint("Verify that there is a white download icon."); // 6
        logFlowPoint("Click on the 'Upgrade now' link and verify the entitlement is upgraded."); // 7
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroPurchase.purchaseEntitlement(driver, entitlement), true);
        
        // 3
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        
        // 4
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPageToLoad();
        GameTile gameTile = new GameTile(driver, offerId);
        boolean isViolatorVisible = gameTile.verifyViolatorUpgradeGameVisible();
        logPassFail(isViolatorVisible, false);
        
        // 5
        GameSlideout baseGameSlideout = new GameTile(driver, offerId).openGameSlideout();
        logPassFail(baseGameSlideout.verifyViolatorUpgradeGameVisible(), false);
        
        // 6
        boolean isDownloadIconVisible = baseGameSlideout.verifyDownloadIconVisible();
        boolean isDownloadIconWhite = baseGameSlideout.verifyDownloadIconColor();
        logPassFail(isDownloadIconVisible && isDownloadIconWhite, false);
        
        // 7
        baseGameSlideout.upgradeEntitlement();
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        checkoutConfirmation.waitIsVisible();
        checkoutConfirmation.closeAndWait();
        logPassFail(!gameTile.hasViolator(), true);
        
        softAssertAll();
    }
}