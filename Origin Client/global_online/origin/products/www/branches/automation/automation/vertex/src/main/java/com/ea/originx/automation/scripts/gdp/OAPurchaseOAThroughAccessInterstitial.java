package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
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
 * Test to subscribe to 'Origin Access' from the 'Origin Access Interstitial'
 * page and add a game to library from the 'Vault' page
 *
 * @author cdeaconu
 */
public class OAPurchaseOAThroughAccessInterstitial extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3126069)
    @Test(groups = {"gdp", "full_regression"})
    public void testPurchaseOAThroughAccessInterstitial(ITestContext context) throws Exception{
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.MINI_METRO);
        final EntitlementInfo vaultEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.TITANFALL_2);
        final String vaultEntitlementId = vaultEntitlement.getOfferId();
        final String vaultEntitlementName = vaultEntitlement.getName();
        
        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Navigate to GDP of a single edition vault entitlement."); // 2
        logFlowPoint("Click on the primary CTA and verify the 'Origin Access Interstitial' page loads."); // 3
        logFlowPoint("Click on the 'Join Origin Access' CTA and skip the 'New User Experience' page."); // 4
        logFlowPoint("Verify the mini profile in the left nav now shows 'OA (Origin Access)' badging."); // 5
        logFlowPoint("Navigate to the 'Vault' page by clicking the 'Origin Access' tab in the left nav"); // 6
        logFlowPoint("Add a game from the 'Vault' page by clicking the 'Add to Library' CTA on a tile."); // 7
        logFlowPoint("Navigate to 'Game Library' and verify the game appears."); // 8
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 3
        new GDPActionCTA(driver).clickGetTheGameCTA();
        logPassFail(new AccessInterstitialPage(driver).verifyOSPInterstitialPageReached(), true);
        
        // 4
        logPassFail(MacroOriginAccess.purchaseOriginAccessThroughAccessInterstitialPage(driver), true);
        
        // 5
        logPassFail(new MiniProfile(driver).verifyOriginAccessBadgeVisible(), false);
        
        // 6
        new NavigationSidebar(driver).clickOriginAccessLink();
        VaultPage vaultPage = new VaultPage(driver);
        logPassFail(vaultPage.verifyPageReached(), true);
        
        // to make popup menu for Origin Access sub-menu disappear
        new NavigationSidebar(driver).hoverOnStoreButton();
        
        // 7
        vaultPage.clickEntitlementByOfferID(vaultEntitlementId);
        GDPHeader gdpHeader = new GDPHeader(driver);
        gdpHeader.verifyGDPHeaderReached();
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        gdpActionCTA.clickAddToLibraryVaultGameCTA();
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        logPassFail(checkoutConfirmation.waitIsVisible(), true);
        checkoutConfirmation.clickCloseCircle();
        
        // 8
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(gameLibrary.isGameTileVisibleByName(vaultEntitlementName), true);
        
        softAssertAll();
    }
}