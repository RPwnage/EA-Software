package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
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
 * Tests the game rating in 'OSP' for games with multiple editions is country specific
 *
 * @author alcervantes
 */
public class OAOfferSelectionPageRating extends EAXVxTestTemplate{

    public void testOfferSelectionPageRating(ITestContext context, CountryInfo.CountryEnum country) throws Exception{

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getUnentitledUserAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);

        final WebDriver driver;
        final String storefrontOverride = "/" + country.getCountryCodeX() + "/en-us";
        String countryRating = country.getCountryGameRating();

        boolean isClient = ContextHelper.isOriginClientTesing(context);

        logFlowPoint("Open Origin and login"); // 1
        logFlowPoint("Load GDP page"); // 2
        logFlowPoint("In "+ country.getCountry() +" storefront ("+country.getCountryCode()+"), navigate to a Offer Selection Page that: has a "+countryRating+" rating"); // 3
        logFlowPoint("Verify "+countryRating+" game rating is showing at the bottom right of the OSP header"); // 4

        if (isClient) {
            EACoreHelper.overrideCountryTo(country, client.getEACore());
            driver = startClientObject(context, client);
        }else{
            driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, storefrontOverride);
        }

        //1
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        //3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        gdpActionCTA.waitForPageToLoad();
        gdpActionCTA.clickPreorderCTA();

        AccessInterstitialPage accessInterstitialPage = new AccessInterstitialPage(driver);
        accessInterstitialPage.waitForPageToLoad();
        accessInterstitialPage.clickBuyGameOSPCTA();

        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        offerSelectionPage.waitForPageToLoad();
        logPassFail(offerSelectionPage.verifyOfferSelectionPageReached(), true);

        //4
        logPassFail(offerSelectionPage.verifyOSPEntitlementRating(country), true);
        softAssertAll();
    }
}
