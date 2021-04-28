package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

    /**
     * Tests the info block icon and title in 'GDP'
     *
     * @author bbelyk
     */
    public class OAInfoBlockAvailableThroughSubscription extends EAXVxTestTemplate {

        @TestRail(caseId = 3064041)
        @Test(groups = {"gdp", "full_regression"})
        public void testGDPInfoBlock(ITestContext context) throws Exception {

            final OriginClient client = OriginClientFactory.create(context);
            final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
            final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.TITANFALL_2);
            final String subscriptionType = "basic";

            logFlowPoint("Log and log into Origin"); // 1
            logFlowPoint("Navigate to the GDP of a released game that is available through subscription"); // 2
            logFlowPoint("Verify the info block icon is displayed"); // 3
            logFlowPoint("Verify the info block text is displayed to the right of the icon and 'Basic' subscription logo is displayed"); // 4
            logFlowPoint("Verify the info block is still showing after user entitles the game with subscription"); // 5

            // 1
            WebDriver driver = startClientObject(context, client);
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
            MacroOriginAccess.purchaseOriginAccess(driver);

            // 2
            logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

            // 3
            GDPHeader gdpHeader = new GDPHeader(driver);
            logPassFail(gdpHeader.verifyProductHeroInfoBlockIconVisible(), false);

            // 4
            logPassFail(gdpHeader.verifyProductHeroInfoBlockTitle(subscriptionType), true);

            // 5
            new GDPActionCTA(driver).clickAddToLibraryVaultGameCTA();
            CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
            checkoutConfirmation.waitIsVisible();
            checkoutConfirmation.clickCloseCircle();
            logPassFail(Waits.pollingWait(() -> gdpHeader.verifyProductHeroInfoBlockTitle(subscriptionType)), true);

            softAssertAll();
        }

    }
