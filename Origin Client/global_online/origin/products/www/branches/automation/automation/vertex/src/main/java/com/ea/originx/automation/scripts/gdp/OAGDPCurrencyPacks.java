package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPCurrencySelector;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.Fifa18;
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
 * Tests Currency Pack Options
 *
 * @author tdhillon
 */
public class OAGDPCurrencyPacks extends EAXVxTestTemplate {

    @TestRail(caseId = 3001990)
    @Test(groups = {"gdp", "full_regression"})
    public void testCurrencyPacks(ITestContext context) throws Exception{

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getRandomAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_18_CURRENCY_PACK);
        String[] currencyPackNames = Fifa18.FIFA18_CURRENCY_PACK_NAMES;
        String[] currencyPackOfferIds = Fifa18.FIFA18_CURRENCY_PACK_OFFER_IDS;

        logFlowPoint("Log into Origin with random account"); // 1
        logFlowPoint("Navigate to the GDP of an Currency Pack entitlement"); // 2
        logFlowPoint("Verify Currency Pack Selection text and DropDown button is visible"); // 3
        logFlowPoint("Click dropdown button and verify options menu is visible and correct"); // 4
        logFlowPoint("Verify option in dropdown menu is selected"); // 5
        logFlowPoint("Verify 'Price' text on 'Buy' Button changes on selecting new Currency pack"); // 6

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 3
        GDPCurrencySelector gdpCurrencySelector = new GDPCurrencySelector(driver);
        gdpCurrencySelector.waitForPageToLoad();
        boolean isCurrencyPackSelectionTextVisible = gdpCurrencySelector.verifyCurrencyPackSelectionText();
        boolean isCurrencyPackDropDownVisible = gdpCurrencySelector.verifyCurrencyPackDropDownButtonVisible();
        logPassFail(isCurrencyPackSelectionTextVisible && isCurrencyPackDropDownVisible, false);

        // 4
        gdpCurrencySelector.clickCurrencyPackDropdown();
        logPassFail(gdpCurrencySelector.verifyCurrencyPackOptions(currencyPackNames, currencyPackOfferIds), false);

        // 5
        logPassFail(gdpCurrencySelector.verifySelectingCurrencyPackFromDropdownWorks(currencyPackOfferIds), false);

        // 6
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpCurrencySelector.verifyPriceOnBuyButtonMatchesPriceOnDropdownButton(gdpActionCTA.getPriceFromBuyButton()), false);

        softAssertAll();
    }
}
