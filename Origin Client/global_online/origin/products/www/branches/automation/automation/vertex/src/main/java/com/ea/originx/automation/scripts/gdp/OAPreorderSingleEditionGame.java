package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test pre-ordering a Single edition game
 *
 * @author tdhillon
 */
public class OAPreorderSingleEditionGame extends EAXVxTestTemplate {

    public enum TEST_TYPE {
        SIGNED_IN,
        ANONYMOUS_USER
    }

    protected void testPreorderSingleEditionGame(ITestContext context, TEST_TYPE type) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.GDP_PRE_RELEASE_TEST_OFFER01_STANDARD);

        if (type == TEST_TYPE.SIGNED_IN) {
            logFlowPoint("Login as a newly registered account"); // 1
        }

        logFlowPoint("Navigate to the GDP of an unreleased Single edition game available for pre-order and click 'Preorder' button"); // 2
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logFlowPoint("Verify the login page appears and login to continue"); // 3
        }

        logFlowPoint("Complete the purchase flow"); // 4
        logFlowPoint("Click on the 'View in Library' primary button and verify the page is navigated to 'Game Library'."); // 5
        logFlowPoint("Verify the purchased game is now in 'Game Library'."); // 6

        // 1
        WebDriver driver = startClientObject(context, client);
        if (type == TEST_TYPE.SIGNED_IN) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        gdpActionCTA.clickPreorderCTA();

        // 3
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        // 4
        new PaymentInformationPage(driver).waitForPaymentInfoPageToLoad();
        logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);

        // 5
        gdpActionCTA.clickViewInLibraryCTA();
        GameLibrary gameLibrary = new GameLibrary(driver);
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        // 6
        logPassFail(gameLibrary.isGameTileVisibleByName(entitlement.getName()), true);

        softAssertAll();
    }
}
