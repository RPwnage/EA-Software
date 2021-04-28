package com.ea.originx.automation.scripts.legal;

import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.pageobjects.dialog.MediaDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
import com.ea.originx.automation.lib.pageobjects.store.MediaCarousel;
import com.ea.originx.automation.lib.resources.games.Battlefield4;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Tests that anonymous users are unable to view age gated videos.  Only testable on the browser because the client can't reach the SPA without signing in.
 *
 * @author cvanichsarn
 */
public class OAWebVideoAgeGate extends EAXVxTestTemplate {

    public void testWebVideoAgeGate(ITestContext context, CountryInfo.CountryEnum countryEnum) throws Exception {

        final Battlefield4 entitlementM = new Battlefield4();
        final EntitlementInfo entitlementE = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.UNRAVEL);
        final OriginClient client = OriginClientFactory.create(context);
        int ofAge = countryEnum.getOfAge();
        final String countryCode = countryEnum.getCountryCodeX();
        final String storefrontOverride = "/" + countryCode + "/en-us";
        final String originQAOfferUrl = "https://dev.www.x.origin.com/" + countryCode + "/en-us/store/originqa/originqa-02/standard-edition?cmsstage=preview";
        WebDriver driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, storefrontOverride);

        logFlowPoint("Without signing in, navigate to the GDP Page of a Mature rated game and click on a video"); // 1
        logFlowPoint(String.format("Enter an age under %s and verify restricted message appears", ofAge)); // 2
        logFlowPoint("Navigate to the GDP page of an Everyone rated game and click on a video"); // 3
        logFlowPoint(String.format("Enter an age of %s or younger and verify restricted message does not appear", ofAge)); // 4
        logFlowPoint("Navigate to the GDP page of an Mature rated game and click on a video"); // 5
        logFlowPoint(String.format("Enter an age of %s or older and verify restricted message does not appear", ofAge)); // 6
        // Steps 7 and 8 will fail until OPOI-71818 is fixed
        logFlowPoint("Navigate to the GDP page of an Rating Pending game and click on a video"); // 7
        logFlowPoint(String.format("Enter an age of %s or younger and verify restricted message appears", ofAge)); // 8

        // 1
        MacroGDP.loadGdpPage(driver, entitlementM);
        MediaCarousel mediaCarousel = new MediaCarousel(driver);
        mediaCarousel.openItemOnCarousel(0);
        MediaDialog mediaDialog = new MediaDialog(driver);
        logPassFail(mediaDialog.verifyAgeGate(), true);

        //2
        mediaDialog.enterVideoAgeGateBirthday(TestScriptHelper.getBirthday(ofAge - 1));
        logPassFail(!mediaDialog.isAgeRequirementsMet(), true);

        //3
        client.stop();
        driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, storefrontOverride);
        MacroGDP.loadGdpPage(driver, entitlementE);
        mediaCarousel = new MediaCarousel(driver);
        mediaCarousel.openItemOnCarousel(0);
        mediaDialog = new MediaDialog(driver);
        logPassFail(!mediaDialog.verifyAgeGate(), true);

        //4
        logPassFail(mediaDialog.isAgeRequirementsMet(), true);

        //5
        client.stop();
        driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, storefrontOverride);
        MacroGDP.loadGdpPage(driver, entitlementM);
        mediaCarousel = new MediaCarousel(driver);
        mediaCarousel.openItemOnCarousel(0);
        mediaDialog = new MediaDialog(driver);
        logPassFail(mediaDialog.verifyAgeGate(), true);

        //6
        mediaDialog.enterVideoAgeGateBirthday(TestScriptHelper.getBirthday(ofAge + 1));
        logPassFail(mediaDialog.isAgeRequirementsMet(), true);

        // Steps 7 and 8 will fail until OPOI-71818 is fixed
        //7
        client.stop();
        driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, storefrontOverride);
        driver.get(originQAOfferUrl);
        mediaCarousel = new MediaCarousel(driver);
        new GDPHeader(driver).verifyGDPHeaderReached();
        mediaCarousel.openItemOnCarousel(0);
        mediaDialog = new MediaDialog(driver);
        logPassFail(mediaDialog.verifyAgeGate(), true);

        //8
        mediaDialog.enterVideoAgeGateBirthday(TestScriptHelper.getBirthday(ofAge - 1));
        logPassFail(!mediaDialog.isAgeRequirementsMet(), true);

        softAssertAll();
    }

}
