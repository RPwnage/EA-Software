package com.ea.originx.automation.scripts.gamelibrary;

import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test clicking the 'Get it Now' button add a game to 'Game Library'
 *
 * @author cdeaconu
 */
public class OARedeemVaultBaseGame extends EAXVxTestTemplate{
    
    @TestRail(caseId = 11093)
    @Test(groups = {"gamelibrary", "full_regression"})
    public void testRedeemVaultBaseGame (ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo entitlementInfo = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.PEGGLE);
        final String entitlementName = entitlementInfo.getName();
        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        logFlowPoint("Sign into an account that is an Origin Access Subscriber"); // 1
        logFlowPoint("Go to the Origin Access tab."); // 2
        logFlowPoint("Click on any vault title listed"); // 3

        if(!isClient){
            logFlowPoint("In the GDP for the game click on the 'Add to Library' CTA, verify the user can entitle the game to their Library"); // 4
            logFlowPoint("After adding the game to the account, go to the game library and verify the correct entitlement is added to the user's Game Library"); // 5
        }else{
            logFlowPoint("Click the 'Download with Origin' button, verify the user is sent to the Game Library tab");//4
            logFlowPoint("Verify the game starts downloading."); // 5
            logFlowPoint("After adding the game to the account, go to the game library and verify the correct entitlement is added to the user's Game Library"); // 6
        }

        //1
        final WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, userAccount);
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);

        //2
        Waits.sleep(5000);//Wait for Origin page to refresh
        OriginAccessPage originAccessPage = new OriginAccessPage(driver);
        logPassFail(originAccessPage.isPageLoaded(), true);

        //3
        VaultPage vaultPage = new VaultPage(driver);
        vaultPage.clickEntitlementByName(entitlementName);
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(Waits.pollingWait(() -> gdpActionCTA.verifyGDPHeaderReached()), true);

        gdpActionCTA.clickAddToLibraryVaultGameCTA();
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        checkoutConfirmation.waitIsVisible();

        if (!isClient) {
            //4
            logPassFail(checkoutConfirmation.isDialogVisible(), true);
            checkoutConfirmation.close();

            //5
            GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
            gameLibrary.waitForPageToLoad();
            logPassFail(gameLibrary.isGameTileVisibleByName(entitlementName), true);
        } else {
            //4
            checkoutConfirmation.close();
            gdpActionCTA.clickViewInLibraryCTA();
            new GameSlideout(driver).clickSlideoutCloseButton();
            GameLibrary gameLibrary = new GameLibrary(driver);
            gameLibrary.waitForPageToLoad();
            logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

            //5
            MacroGameLibrary.startDownloadingEntitlement(driver, entitlementInfo.getOfferId());
            logPassFail(new DownloadManager(driver).isDownloadManagerVisible(), true);

            //6
            logPassFail(gameLibrary.isGameTileVisibleByName(entitlementName), true);
        }
        softAssertAll();
    }
}