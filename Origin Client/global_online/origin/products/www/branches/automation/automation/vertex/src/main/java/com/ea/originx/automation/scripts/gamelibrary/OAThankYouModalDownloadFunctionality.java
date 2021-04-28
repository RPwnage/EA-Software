package com.ea.originx.automation.scripts.gamelibrary;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadDialog;
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
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test clicking the 'Download with Origin' button initiates a download of the
 * entitlement.
 *
 * @author cdeaconu
 */
public class OAThankYouModalDownloadFunctionality extends EAXVxTestTemplate{
    
    @TestRail(caseId = 11098)
    @Test(groups = {"gamelibrary", "client_only", "full_regression"})
    public void testThankYouModalDownloadFunctionality (ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.TITANFALL_2);
        final String offerId = entitlement.getOfferId();
        
        logFlowPoint("Login as a newly registered user."); // 1
        logFlowPoint("Purchase 'Origin Access' and go to the 'Vault' page."); // 2
        logFlowPoint("Click on 'Add to Game Library' on a game and verify 'Thank You' modal appears."); // 3
        logFlowPoint("Verify clicking the 'Download with Origin' button initiates a download of the entitlement."); // 4
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        
        // 3
        new VaultPage(driver).clickEntitlementByOfferID(offerId);
        new GDPHeader(driver).verifyGDPHeaderReached();
        new GDPActionCTA(driver).clickAddToLibraryVaultGameCTA();
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        logPassFail(checkoutConfirmation.waitIsVisible(), true);
        
        // 4
        checkoutConfirmation.clickDownload();
        logPassFail(new DownloadDialog(driver).waitIsVisible(), true);
        
        softAssertAll();
    }
}