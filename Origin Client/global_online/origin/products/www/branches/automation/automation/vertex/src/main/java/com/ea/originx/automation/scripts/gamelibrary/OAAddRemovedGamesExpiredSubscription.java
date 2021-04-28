package com.ea.originx.automation.scripts.gamelibrary;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.RemoveFromLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideoutCogwheel;
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
import com.ea.vx.originclient.utils.OriginAccessService;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.ITestResult;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.Test;

/**
 * Test Purchased Vault content is available after Sub expires
 * 
 * @author mdobre
 */
public class OAAddRemovedGamesExpiredSubscription extends EAXVxTestTemplate{
    
    OriginClient client = null;
    EntitlementInfo purchasedVaultContent = null;
    
    @TestRail(caseId = 11106)
    @Test(groups = {"client_only", "gamelibrary", "full_regression"})
    public void testAddRemovedGamesExpiredSubscription (ITestContext context) throws Exception {
     
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        EntitlementInfo expiredAccessContent = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.PEGGLE);
        String expiredAccessConetentOfferID = expiredAccessContent.getOfferId();
        
        logFlowPoint("Launch and log into Origin with a new account."); // 1
        logFlowPoint("Purchase Origin Access basic subscription, add an entitlement to the game library, cancel the subscription and navigate to the 'Game Library'."); // 2
        logFlowPoint("Remove vault title from the 'Game Library'."); // 3
        logFlowPoint("Purchase Origin Access basic subscription."); // 4
        logFlowPoint("Verify the previously removed vault title can be re-added to the 'Game Library'."); // 5
        logFlowPoint("Download and play the purchased entitlement."); // 6
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        //2
        MacroOriginAccess.purchaseOriginAccess(driver);
        MacroGDP.loadGdpPage(driver, expiredAccessContent);
        MacroGDP.addVaultGameAndCheckViewInLibraryCTAPresent(driver);
        OriginAccessService.immediateCancelSubscription(userAccount);
        // wait for Origin page to refresh
        sleep(20000);
        new NavigationSidebar(driver).gotoGameLibrary();
        GameLibrary gameLibrary = new GameLibrary(driver);
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);
        
        //3
        GameTile expiredAccessContentGameTile = new GameTile(driver, expiredAccessConetentOfferID);
        GameSlideout expiredAccessContentGameSlideOut = expiredAccessContentGameTile.openGameSlideout();
        GameSlideoutCogwheel cogwheel = expiredAccessContentGameSlideOut.getCogwheel();
        cogwheel.removeFromLibrary();
        new RemoveFromLibrary(driver).confirmRemoval();
        sleep(4000); //wait for game to be removed
        logPassFail(!gameLibrary.isGameTileVisibleByName(expiredAccessContent.getName()), true);
        
        //4
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        
        //5
        MacroGDP.loadGdpPage(driver, expiredAccessContent);
        MacroGDP.addVaultGameAndCheckViewInLibraryCTAPresent(driver);
        new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(gameLibrary.isGameTileVisibleByOfferID(expiredAccessConetentOfferID), true);
        
        //6
        MacroGameLibrary.downloadFullEntitlement(driver, expiredAccessConetentOfferID);
        expiredAccessContent.addActivationFile(client);
        expiredAccessContentGameTile.play();
        MacroGameLibrary.handlePlayDialogs(driver);
        logPassFail(Waits.pollingWaitEx(() -> expiredAccessContent.isLaunched(client)), true);
        
        softAssertAll();
    }
    
    /**
     * override testCleanup to kill launched game
     * @param context (@link ITestContext) context for the running test
     */
    @Override
    @AfterMethod(alwaysRun = true)
    protected void testCleanUp(ITestResult result, ITestContext context) {
        if(client != null && purchasedVaultContent != null){
            purchasedVaultContent.killLaunchedProcess(client);
        }
        super.testCleanUp(result, context);
    }
}