package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test clicking 'Upgrade my edition' option in the drop-down menu redirects the
 * user to the correct page
 *
 * @author cdeaconu
 */
public class OAUpgradeEditionPrimaryDropDown extends EAXVxTestTemplate{
    
    public enum TEST_TYPE {
        SUBSCRIBER,
        NON_SUBSCRIBER,
    }
    
    public void testUpgradeEditionPrimaryDropDown(ITestContext context, TEST_TYPE type) throws Exception{
        
        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        UserAccount userAccount;
        String accountType;
        
        if ( type == TEST_TYPE.SUBSCRIBER ) {
            accountType = "subscriber";
            userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.BF4_IN_LIBRARY_THROUGH_PURCHASE_SUBSCRIBER);
        } else {
            accountType = " non-subscriber";
            userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.BF4_IN_LIBRARY_THROUGH_PURCHASE_NON_SUBSCRIBER);
        }
                
        logFlowPoint("Log into Origin with a " + accountType + " account."); // 1
        logFlowPoint("Navigate to GDP of a multiple edition entitlement with the lesser edition owned through purchase."); // 2
        logFlowPoint("Verify an 'Upgrade my edition' option is showing in the drop-down menu."); // 3
        logFlowPoint("Select 'Upgrade my edition' option in the drop-down menu and verify the page redirects to 'Offer Selection Interstitial' page."); // 4
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyDropdownUpgradeMyEditionVisible(), true);
        
        // 4
        gdpActionCTA.selectDropdownUpgradeMyEdition();
        logPassFail(new AccessInterstitialPage(driver).verifyOSPInterstitialPageReached(), true);
        
        softAssertAll();
    }
}