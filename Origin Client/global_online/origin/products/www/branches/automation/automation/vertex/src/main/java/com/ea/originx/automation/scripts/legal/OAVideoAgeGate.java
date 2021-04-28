package com.ea.originx.automation.scripts.legal;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.MediaDialog;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.store.MediaCarousel;
import com.ea.originx.automation.lib.resources.games.Battlefield4;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Tests that underage users cannot view above age videos on the client and the
 * browser
 *
 * @author mdobre
 */
public class OAVideoAgeGate extends EAXVxTestTemplate {

    public void testVideoAgeGate(ITestContext context, CountryInfo.CountryEnum countryEnum) throws Exception {
        String country = countryEnum.getCountry();

        UserAccount underAgeAccount = AccountManagerHelper.createNewThrowAwayAccount(country, 16);
        UserAccount ofAgeAccount = AccountManagerHelper.getUserAccountWithCountry(country);
        Battlefield4 entitlementM = new Battlefield4();

        final OriginClient client = OriginClientFactory.create(context);
        //Change the country to either USA or Taiwan
        EACoreHelper.overrideCountryTo(countryEnum, client.getEACore());
        WebDriver driver = startClientObject(context, client);

        logFlowPoint("Launch Origin and login as underage " + country + " user"); // 1
        logFlowPoint("Navigate to the GDP Page of an Mature rated game and click on a video"); // 2
        logFlowPoint("Log out of origin and login as an of age " + country + " account " + ofAgeAccount.getUsername()); // 3
        logFlowPoint("Navigate to the GDP Page of a Mature rated game and verify you can start the video without a message."); // 4

        // 1
        logPassFail(MacroLogin.startLogin(driver, underAgeAccount), true);

        // 2
        MacroGDP.loadGdpPage(driver, entitlementM);
        new MediaCarousel(driver).openItemOnCarousel(0);
        MediaDialog mediaDialog = new MediaDialog(driver);
        logPassFail(!mediaDialog.isAgeRequirementsMet(), true);

        //3
        mediaDialog.close();
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.selectSignOut();
        logPassFail(MacroLogin.startLogin(driver, ofAgeAccount), true);

        // 4
        MacroGDP.loadGdpPage(driver, entitlementM);
        new MediaCarousel(driver).openItemOnCarousel(0);
        logPassFail(Waits.pollingWait(mediaDialog::isAgeRequirementsMet), true);

        softAssertAll();
    }
}
