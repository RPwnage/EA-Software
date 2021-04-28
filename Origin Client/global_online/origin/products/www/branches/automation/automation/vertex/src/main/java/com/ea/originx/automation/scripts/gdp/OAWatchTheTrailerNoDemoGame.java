package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.MediaDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
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
 * Tests clicking the secondary 'Watch Trailer' CTA opens a video player in
 * full-window mode
 *
 * @author cdeaconu
 */
public class OAWatchTheTrailerNoDemoGame extends EAXVxTestTemplate {
    
    public enum TEST_TYPE {
        SUBSCRIBER,
        NON_SUBSCRIBER,
        ANONYMOUS_USER
    }
    
    public void testWatchTheTrailerNoDemoGame (ITestContext context, TEST_TYPE type) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount;
        String accountType;
        
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BATTLEFIELD_HARDLINE);
        
        if (type == TEST_TYPE.SUBSCRIBER) {
            accountType = "subscriber";
            userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE);
        } else {
            accountType = "non-subscriber";
            userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_EXPIRED);
        }
        
        if (type != TEST_TYPE.ANONYMOUS_USER) {
            logFlowPoint("Log into Origin with a " + accountType + " account."); // 1
        }
        logFlowPoint("Navigate to GDP of an entitlement with a video configured in the media carousel that doesn't have Demo and/or Trial."); // 2
        logFlowPoint("Verify 'Watch Trailer' secondary button is showing below the primary CTA."); // 3
        logFlowPoint("Click on the 'Watch Trailer' CTA and verify a video player opens in full-window mode."); // 4
        
        WebDriver driver = startClientObject(context, client);
        
        // 1
        if (type != TEST_TYPE.ANONYMOUS_USER) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }
        
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