package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeroActionDescriptors;
import com.ea.originx.automation.lib.resources.games.Sims4;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests discount information on 'GDP Header' Page
 *
 * @author micwang
 */
public class OAGDPHeaderDiscount extends EAXVxTestTemplate {

    @TestRail(caseId = 3001996)
    @Test(groups = {"gdp", "full_regression"})
    public void testGDPHeaderDiscount(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Login to Origin"); //1
        logFlowPoint("Navigate to a 'GDP' Page for an entitlement that is on sale"); //2
        logFlowPoint("Verify 'Save <discount percentage> <cross out price before discount>' texts are visible"); //3

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, Sims4.SIMS4_DISCOUNT_TEST_DLC_NAME, Sims4.SIMS4_DISCOUNT_TEST_DLC_PARTIAL_PDP_URL), true);

        // 3
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        boolean isDiscountPercentageVisible = gdpHeroActionDescriptors.verifyOriginAccessSavingsPercent();
        boolean isCrossOutPriceBeforeDiscountVisible = gdpHeroActionDescriptors.verifyOriginalPriceIsStrikedThrough();
        logPassFail(isDiscountPercentageVisible && isCrossOutPriceBeforeDiscountVisible, true);
    }
}