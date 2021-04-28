package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPNavBar;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPSections;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the GDP navigation bar.
 *
 * @author esdecastro 
 */
public class OAGDPNavigationBar extends EAXVxTestTemplate {

    @TestRail(caseId = 12286)
    @Test(groups = {"gdp", "full_regression"})
    public void testGDPNavigationBar(ITestContext context) throws Exception{

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount user = AccountManager.getRandomAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.TITANFALL_2);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        
        logFlowPoint("Open Origin and login"); // 1
        logFlowPoint("Navigate to any GDP that is not a currency GDP and verfiy that the menu bar is visible"); // 2
        logFlowPoint("Scroll down the page and verify that the menu bar is still visible"); // 3
        logFlowPoint("Verify that the primary CTA is visible within the menu bar"); // 4
        logFlowPoint("Verify that the pack art is visible within the menu bar"); // 5
        logFlowPoint("Click the pack art and verify that the user is scrolled back up to the top of the page"); // 6
        logFlowPoint("Click on a menu item in the menu bar and verify that the user is taken down to the specified section"); // 7

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        // 2
         if (isClient) {
            new MainMenu(driver).clickMaximizeButton();
        }
        MacroGDP.loadGdpPage(driver, entitlement);
        GDPNavBar gdpNavBar = new GDPNavBar(driver);
        logPassFail(gdpNavBar.isVisible(), true);

        // 3
        GDPSections gdpSections = new GDPSections(driver);
        // slow scroll to the bottom of the page
        int pageHeight = EAXVxSiteTemplate.getOriginClientWindowSize(driver).height;
        int i = 0;
        while (i < pageHeight) {
            gdpSections.scrollByVerticalOffset(2);
            i++;
        }
        logPassFail(gdpNavBar.verifyNavBarIsPinned(), true);

        // 4
        logPassFail(gdpNavBar.verifyBuyCtaIsVisible(), false);

        // 5
        logPassFail(gdpNavBar.verifyPackArtIsVisible(), false);

        // 6
        gdpNavBar.clickPackArt();
        logPassFail(new GDPHeader(driver).verifyGameTitleOrLogoVisible(), false);

        // 7
        gdpNavBar.clickSystemRequirements();
        logPassFail(gdpSections.verifySystemRequirementsSectionVisible(), false);

        softAssertAll();
    }
}