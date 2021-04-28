package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeroActionDescriptors;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.Sims4;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;
/**
 * Tests OA Subscriber discount stacks with regular sale discounts.
 *
 * @author esdecastro
 */
public class OAStackedSubDiscount extends EAXVxTestTemplate {

    @TestRail(caseId = 10986)
    @Test(groups = {"originaccess", "int_only"})
    public void testStackedSubDiscount(ITestContext context) throws Exception{

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount nonSubAccount = AccountManager.getUnentitledUserAccount();
        final UserAccount subAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE_NO_ENTITLEMENTS);

        final String dlcName = Sims4.SIMS4_DISCOUNT_TEST_DLC_NAME;
        final String dlcPartialURL = Sims4.SIMS4_DISCOUNT_TEST_DLC_PARTIAL_PDP_URL;

        logFlowPoint("Open origin and log into a non subscriber account"); // 1
        logFlowPoint("Navigate to a GDP for an item that is on sale and verify that the item is on sale"); // 2
        logFlowPoint("Log out and log into a subscriber account"); // 3
        logFlowPoint("Navigate to a GDP for an item that is on sale and verify that the item is on sale"); // 4
        logFlowPoint("Verify that the price is 10% less than the price from step 2"); // 5

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, nonSubAccount), true);

        // 2
        boolean isSaleGDPLoaded = MacroGDP.loadGdpPage(driver, dlcName, dlcPartialURL);
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        boolean isDiscountPercentVisible = gdpHeroActionDescriptors.verifyOriginAccessSavingsPercent();
        float nonSubPrice = Float.parseFloat(new GDPActionCTA(driver).getPriceFromBuyButton());
        logPassFail(isSaleGDPLoaded && isDiscountPercentVisible, true);

        // 3
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.selectSignOut();
        Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.MAIN_SPA_PAGE_URL);
        logPassFail(MacroLogin.startLogin(driver, subAccount), true);

        // 4
        MacroGDP.loadGdpPage(driver, dlcName, dlcPartialURL);
        gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        logPassFail(gdpHeroActionDescriptors.verifyOriginAccessSavingsPercent(), true);

        // 5
        float subPrice = Float.parseFloat(new GDPActionCTA(driver).getPriceFromBuyButton());
        logPassFail(TestScriptHelper.verifyOriginAccessDiscountApplied(nonSubPrice, subPrice), false);

        softAssertAll();
    }
}
