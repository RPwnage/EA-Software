package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeroActionDescriptors;
import com.ea.originx.automation.lib.resources.AccountTags;
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
 * Test Info Block for a purchased entitlement 
 *
 * @author mdobre
 */
public class OAInfoBlockOwnThroughPurchase extends EAXVxTestTemplate {

    @TestRail(caseId = 3064040)
    @Test(groups = {"gdp", "full_regression"})
    public void testInfoBlockOwnThroughPurchase(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String entitlementId = entitlement.getOfferId();
        final UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        
        logFlowPoint("Log into Origin with an existing account."); // 1
        logFlowPoint("Navigate to the GDP of the purchased entitlement."); //2
        logFlowPoint("Verify a green mark icon is showing."); //3
        logFlowPoint("Verify the 'You have the <edition text> of this game.' text is showing on the right of the icon."); //4
        
         // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        //2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        //3
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        boolean isFirstInfoBlockIconVisible = gdpHeroActionDescriptors.verifyFirstInfoBlockIconVisible();
        boolean isFirstInfoBlockIconGreen = gdpHeroActionDescriptors.verifyFirstInfoBlockIconGreen();
        logPassFail(isFirstInfoBlockIconVisible & isFirstInfoBlockIconGreen, false);
        
        //4
        boolean isFirstInfoBlockTextVisible = gdpHeroActionDescriptors.verifyFirstInfoBlockTextVisible();
        boolean isFirstInfoBlockTestInTheRight = gdpHeroActionDescriptors.verifyFirstInfoBlockTextIsRightOfIcon();
        logPassFail(isFirstInfoBlockTextVisible && isFirstInfoBlockTestInTheRight, false);

        softAssertAll();
    }
}
