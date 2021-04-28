package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Tests the game rating in 'GDP' is country specific
 *
 * @author cbalea
 */
public class OAGDPHeaderRating extends EAXVxTestTemplate {

    public void testGDPHeaderRating(ITestContext context, CountryInfo.CountryEnum country) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getRandomAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);

        final WebDriver driver;
        final String storefrontOverride = "/" + country.getCountryCodeX() + "/en-us";

        logFlowPoint("Log into Origin with an existing user"); // 1
        logFlowPoint("Navigate to the GDP of an entitlement with 'Mature' game rating"); // 2
        logFlowPoint("Verify the game rating matches the country"); // 3

        if (ContextHelper.isOriginClientTesing(context)) {
            EACoreHelper.overrideCountryTo(country, client.getEACore());
            driver = startClientObject(context, client);
        }else{
            driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, storefrontOverride);
        }

        // 1
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 3
        GDPHeader gdpHeader = new GDPHeader(driver);
        gdpHeader.waitForPageToLoad();
        logPassFail(gdpHeader.verifyGDPEntitlementRating(country), true);

        softAssertAll();
    }
}
