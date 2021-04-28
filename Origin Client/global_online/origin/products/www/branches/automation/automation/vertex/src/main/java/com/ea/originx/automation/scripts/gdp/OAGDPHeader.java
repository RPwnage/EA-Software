package com.ea.originx.automation.scripts.gdp;


import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests a default 'GDP Header' Page
 *
 * @author nvarthakavi
 */
public class OAGDPHeader extends EAXVxTestTemplate {

    @TestRail(caseId = 3001070)
    @Test(groups = {"gdp", "full_regression"})
    public void testGDPHeader(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BATTLEFIELD_1_STANDARD);

        String sandboxKey = "SANDBOX_ADDRESS";
        String configURL = "https://config.x.origin.com/sb4";

        logFlowPoint("Login to Origin with newly registered user"); //1
        logFlowPoint("Navigate to a 'GDP' Page for an entitlement"); //2
        logFlowPoint("Verify the background image is visible"); //3
        logFlowPoint("Verify the 'Game Logo/Game Title' string is visible"); //4
        logFlowPoint("Verify the subtext string is visible"); //5
        logFlowPoint("Verify the description is visible"); //6
        logFlowPoint("Verify the 'Read More' link is visible"); //7

        WebDriver driver = startClientObject(context, client);

        // 1
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        //3
        GDPHeader gdpHeader = new GDPHeader(driver);
        logPassFail(gdpHeader.verifyHeroBackgroundVisible(), false);

        //4
        logPassFail(gdpHeader.verifyGameTitleOrLogoVisible(), false);

        //5
        logPassFail(gdpHeader.verifyProductHeroLeftRailHeaderVisible(), false);

        //6
        logPassFail(gdpHeader.verifyProductHeroLeftRailDescriptionVisible(), false);

        //7
        logPassFail(gdpHeader.verifyProductHeroLeftRailReadMoreVisible(), false);

        softAssertAll();
    }
}
