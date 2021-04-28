package com.ea.originx.automation.scripts.gdp.header;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeroActionDescriptors;
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
 * Test 'Origin Access discount applied' text is visible for a subscriber user
 * for an entitlement that isn't on the vault
 *
 * @author cdeaconu
 */
public class OAGDPHeaderOADiscount extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3064069)
    @Test(groups = {"gdp", "full_regression"})
    public void testOAGDPHeaderOADiscount(ITestContext context) throws Exception{
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BLACK_MIRROR);
        
        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Purchase Origin Access."); // 2
        logFlowPoint("Navigate to GDP of a single edition non vault entitlement."); // 3
        logFlowPoint("Verify 'Save <discount percentage> <strike-through original price>' is visible"); // 4a
        logFlowPoint("Verify 'Origin Access discount applied' text is showing above the primary button"); // 4b
    
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        
        // 3
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 4a
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        boolean isSavingPercentVisible = gdpHeroActionDescriptors.verifyOriginAccessSavingsPercent();
        boolean isOriginalPriceIsStrikedThrough = gdpHeroActionDescriptors.verifyOriginalPriceIsStrikedThrough();
        logPassFail(isSavingPercentVisible && isOriginalPriceIsStrikedThrough, true);
        
        // 4b
        logPassFail(gdpHeroActionDescriptors.verifyOriginAccessDiscountIsVisible(), true);
        
        softAssertAll();
    }
}