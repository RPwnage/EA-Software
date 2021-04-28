package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.dialog.TryTheGameOutDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test adding Trial Entitlement to 'Game Library'
 *
 * @author cbalea
 */
public class OAAddDemoTrial extends EAXVxTestTemplate {

    public enum TEST_TYPE {
        SUBSCRIBER,
        NON_SUBSCRIBER,
    }

    public void testAddDemoTrial(ITestContext context, OAAddDemoTrial.TEST_TYPE type) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);

        logFlowPoint("Log into Origin as a newly registered user"); // 1
        if (type == TEST_TYPE.SUBSCRIBER) {
            logFlowPoint("Purchase Origin Access"); // 2
        }
        logFlowPoint("Navigate to a Demo/Trial entitlement"); // 3
        logFlowPoint("Click 'Try It First' CTA and verify 'Try the game out' dialog appears"); // 4
        logFlowPoint("Click 'Add to Game Library' and verify the 'Added to your library' dialog appears"); // 5
        logFlowPoint("Navigate to 'Game Library' and verify user owns the Demo/Trial"); // 6

        WebDriver driver = startClientObject(context, client);
        // 1
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        if (type == TEST_TYPE.SUBSCRIBER) {
            logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        }

        // 3
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 4
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        TryTheGameOutDialog tryTheGameOutDialog = new TryTheGameOutDialog(driver);
        gdpActionCTA.clickTryItFirstCTA();
        logPassFail(tryTheGameOutDialog.waitIsVisible(), true);

        //5
        String trialOfferId = tryTheGameOutDialog.getTrialOfferId();
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        tryTheGameOutDialog.clickAddGameToLibraryCTA();
        logPassFail(checkoutConfirmation.waitIsVisible(), true);

        //6
        checkoutConfirmation.closeAndWait();
        new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(new GameTile(driver, trialOfferId).isGameTileVisible(), true);

        softAssertAll();
    }
}