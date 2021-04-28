package com.ea.originx.automation.scripts.gamelibrary;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.Sims4;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import java.util.Arrays;
import java.util.List;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.ITestResult;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.Test;

/**
 * Test purchased vault content shows as owned entitlement and the entitlement can 
 * be downloaded, installed and playable after subscription expired.
 * @author sunlee
 */
public class OAPurchasedVaultContentAfterSubscriptionExpires extends EAXVxTestTemplate{
    
    OriginClient client = null;
    EntitlementInfo purchasedVaultContent = null;
    
    @TestRail(caseId = 11106)
    @Test(groups = {"client_only", "gamelibrary", "full_regression"})
    public void testPurchasedVaultContentAfterSubscriptionExpires (ITestContext context) throws Exception {
        client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_EXPIRED_OWN_VAULT_CONTENT);
        
        purchasedVaultContent = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_DIGITAL_DELUXE);
        String purchasedVaultContentOfferID = purchasedVaultContent.getOfferId();
        String purchasedVaultContentDLCOfferID = Sims4.SIMS4_CITY_LIVING_NAME_OFFER_ID;
        List<String> purchasedVaultContentDLCOfferIDList = Arrays.asList(purchasedVaultContentDLCOfferID);
        EntitlementInfo expiredAccessContent = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.UNRAVEL);
        String expiredAccessConetentOfferID = expiredAccessContent.getOfferId();

        logFlowPoint("Launch and log into Origin with an account has expired subscription and purchased vault content."); // 1
        logFlowPoint("Go to my game library and verify purchased vault entitlment and expired access content game tiles visible."); // 2
        logFlowPoint("View the game detail for the purchased vault entitlement and verify it displayed as owned."); // 3
        logFlowPoint("Veiw the game detail for the expired access content and verify it is not shown as owned."); // 4
        logFlowPoint("Download, install the purchased entitlement with DLC."); // 5
        logFlowPoint("Play the purchased entitlement."); // 6
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        new NavigationSidebar(driver).gotoGameLibrary();
        GameTile purchasedVaultContentGameTile = new GameTile(driver, purchasedVaultContentOfferID);
        GameTile expiredAccessContentGameTile = new GameTile(driver, expiredAccessConetentOfferID);
        logPassFail(purchasedVaultContentGameTile.isGameTileVisible() && expiredAccessContentGameTile.isGameTileVisible(), true);
        
        // 3
        GameSlideout purchasedVaultContentGameSlideOut = purchasedVaultContentGameTile.openGameSlideout();
        logPassFail(!purchasedVaultContentGameSlideOut.verifyOriginAccessMembershipHasExpired() && purchasedVaultContentGameSlideOut.verifyGameEnabled(), true);
        purchasedVaultContentGameSlideOut.clickSlideoutCloseButton();
        
        // 4
        GameSlideout expiredAccessContentGameSlideOut = expiredAccessContentGameTile.openGameSlideout();
        logPassFail(expiredAccessContentGameSlideOut.verifyOriginAccessMembershipHasExpired() && !expiredAccessContentGameSlideOut.verifyGameEnabled(), true);
        expiredAccessContentGameSlideOut.clickSlideoutCloseButton();
        
        // 5
        logPassFail(MacroGameLibrary.downloadFullEntitlementWithSelectedDLC(driver, purchasedVaultContentOfferID, new DownloadOptions(), purchasedVaultContentDLCOfferIDList), true);
        purchasedVaultContent.addActivationFile(client);
        // 6
        purchasedVaultContentGameTile.play();
        MacroGameLibrary.handlePlayDialogs(driver);
        logPassFail(Waits.pollingWaitEx(() -> purchasedVaultContent.isLaunched(client)), true);
        
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
