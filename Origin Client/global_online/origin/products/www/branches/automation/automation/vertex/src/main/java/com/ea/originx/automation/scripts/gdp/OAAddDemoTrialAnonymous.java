package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.dialog.TryTheGameOutDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.resources.OriginClientData;
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
 * Test adding Trial Entitlement to 'Game Library' for an anonymous user
 *
 * @author cbalea
 */
public class OAAddDemoTrialAnonymous extends EAXVxTestTemplate {

    @TestRail(caseId = 3065109)
    @Test(groups = {"gdp", "full_regression", "browser_only"})
    public void testAddDemoTrialAnonymous(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BATTLEFIELD_1_STANDARD);

        logFlowPoint("Navigate to a Demo/Trial entitlement"); // 1
        logFlowPoint("Click 'Try It First' CTA and verify 'Try the game out' dialog appears"); // 2
        logFlowPoint("Click 'Add to Game Library' and verify the 'Sign In' flow begins"); // 3
        logFlowPoint("Complete the 'Sign In' flow and verify the 'Added to your library' dialog appears"); // 4
        logFlowPoint("Navigate to 'Game Library' and verify user owns the Demo/Trial"); // 5

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 2
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        gdpActionCTA.clickTryItFirstCTA();
        TryTheGameOutDialog tryTheGameOutDialog = new TryTheGameOutDialog(driver);
        logPassFail(tryTheGameOutDialog.waitIsVisible(), true);

        // 3
        String trialOfferId = tryTheGameOutDialog.getTrialOfferId();
        tryTheGameOutDialog.clickAddGameToLibraryCTA();
        Waits.waitForPageThatMatches(driver, OriginClientData.LOGIN_PAGE_URL_REGEX, 30);
        LoginPage loginPage = new LoginPage(driver);
        logPassFail(loginPage.isOnLoginPage(), true);

        // 4
        MacroLogin.startLogin(driver, userAccount);
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        logPassFail(checkoutConfirmation.waitIsVisible(), true);

        // 5
        checkoutConfirmation.closeAndWait();
        new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(new GameTile(driver, trialOfferId).isGameTileVisible(), true);

        softAssertAll();
    }
}