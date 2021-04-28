package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test gifting a multiple editions game
 *
 * @author cdeaconu
 */
public class OAGiftingMultipleEditionsGame extends EAXVxTestTemplate{
    
    public enum TEST_TYPE {
        SIGNED_IN,
        ANONYMOUS_USER
    }
    
    public void testGiftingMultipleEditionsGame(ITestContext context, TEST_TYPE type) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.TITANFALL_2);
        
        if(type == TEST_TYPE.SIGNED_IN) {
            logFlowPoint("Log into Origin as newly registered account."); // 1
        }
        logFlowPoint("Navigate to GDP of a giftable multiple edition entitlement"); // 2
        logFlowPoint("Verify 'Purchase as a Gift' option is showing in the drop-down menu and click on it"); // 3
        logFlowPoint("Verify the page redirects to 'Offer Selection' page and click on a 'Purchase as a Gift $<price text>' CTA"); // 4
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logFlowPoint("Log in as a newly registered user."); // 5
        }
        logFlowPoint("Verify the gifting flow starts."); // 6
        
        WebDriver driver = startClientObject(context, client);

        // 1
        if (type == TEST_TYPE.SIGNED_IN) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyDropdownPurchaseAsGiftVisible(), true);
        gdpActionCTA.selectDropdownPurchaseAsGift();
        
        // 4
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        logPassFail(offerSelectionPage.verifyOfferSelectionPageReached(), true);
        offerSelectionPage.clickPrimaryButton( entitlement.getOcdPath());
        
        // 5
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }
        
        // 6
        logPassFail(new FriendsSelectionDialog(driver).waitIsVisible(), true);
        
        softAssertAll();
    }
}