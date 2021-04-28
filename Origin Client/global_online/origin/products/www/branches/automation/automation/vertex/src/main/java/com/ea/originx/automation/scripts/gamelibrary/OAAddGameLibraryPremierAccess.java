package com.ea.originx.automation.scripts.gamelibrary;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroRedeem;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test adding an entitlement to 'Game Library' for an account with 'Premier'
 * subscription
 *
 * @author cdeaconu
 */
public class OAAddGameLibraryPremierAccess extends EAXVxTestTemplate{
    
    @TestRail(caseId = 2911020)
    @Test(groups = {"gamelibrary", "full_regression", "release_smoke"})
    public void testAddGameLibraryPremierAccess(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.TITANFALL_2_DELUXE_EDITION);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();
        final String entitlementParentName = entitlement.getParentName();
        
        logFlowPoint("Launch Origin and login as newly registered account."); // 1
        logFlowPoint("Pruchase 'Origin Premium' subscription."); // 2
        logFlowPoint("Click on a 'Basic or Premier' vault game listed in the vault and verify it navigates to GDP."); // 3
        logFlowPoint("Clicks 'Add to Library' CTA and verify 'Added to library' modal apperas."); // 4
        logFlowPoint("Navigate to the 'Game Library' and verify that the vault game added appears in the library"); // 5
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        if (OSInfo.getEnvironment().equalsIgnoreCase("production")) {
            new MainMenu(driver).selectRedeemProductCode();
            logPassFail(MacroRedeem.redeemOAPremierSubscription(driver, userAccount.getEmail()), true );
        } else {
            logPassFail(MacroOriginAccess.purchaseOriginPremierAccess(driver), true);
        }
        
        // 3
        new VaultPage(driver).clickEntitlementByOfferID(entitlementOfferId);
        logPassFail(Waits.pollingWait(() -> new GDPHeader(driver).verifyGameTitle(entitlementParentName)), true);
        
        // 4
        new GDPActionCTA(driver).clickAddToLibraryVaultGameCTA();
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        logPassFail(checkoutConfirmation.waitIsVisible(), true);
        checkoutConfirmation.closeAndWait();
        
        // 5
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(gameLibrary.isGameTileVisibleByName(entitlementName), true);
        
        softAssertAll();
    }
}