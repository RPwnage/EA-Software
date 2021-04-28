package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.MediaDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Tests clicking the secondary 'Watch Trailer' CTA opens a video player in
 * full-window mode for a demo game
 *
 * @author cdeaconu
 */
public class OAWatchTheTrailerDemoGame extends EAXVxTestTemplate {
    
    public enum TEST_TYPE {
        SUBSCRIBER,
        NON_SUBSCRIBER
    }
    
    public void testWatchTheTrailerDemoGame (ITestContext context, TEST_TYPE type) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount;
        
        EntitlementInfo entitlement;
        String accountType;
        
        if ( type == TEST_TYPE.SUBSCRIBER ) {
            accountType = "subscriber";
            entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
            Criteria criteria = new Criteria.CriteriaBuilder().tag(AccountTags.SUBSCRIPTION_ACTIVE.name()).build();
            criteria.addEntitlement(entitlement.getName());
            userAccount = AccountManager.getInstance().requestWithCriteria(criteria);
        } else {
            accountType = "non-subscriber";
            entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);
            Criteria criteria = new Criteria.CriteriaBuilder().build();
            criteria.addEntitlement(entitlement.getName());
            userAccount = AccountManager.getInstance().requestWithCriteria(criteria);
        }
        
        logFlowPoint("Log into Origin with a " + accountType + " account."); // 1
        logFlowPoint("Navigate to GDP of an entitlement that has a video configured in the media carousel."); // 2
        logFlowPoint("Verify 'Watch Trailer' secondary button is showing below the primary CTA."); // 3
        logFlowPoint("Click on the 'Watch Trailer' CTA and verify a video player opens in full-window mode."); // 4
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true); 
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyWatchTrailerCTAIsVisibleBelowPrimaryCTA(), true);
        
        // 4
        gdpActionCTA.clickWatchTrailerCTA();
        logPassFail(new MediaDialog(driver).verifyVideoAllowingFullScreen(), true);
        
        softAssertAll();
    }
}