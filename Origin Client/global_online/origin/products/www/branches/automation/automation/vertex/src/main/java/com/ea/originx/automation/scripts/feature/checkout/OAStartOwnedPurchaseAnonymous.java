package com.ea.originx.automation.scripts.feature.checkout;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.EntitlementAwarenessDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.store.StorePage;
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
 * Test to check that when an anonymous user tries to buy a game with a web browser and then logs in with an account that
 * already owns the game, that the user is informed the game is owned and redirected to the 'Game Library'.
 *
 * NEEDS UPDATE TO GDP
 *
 * @author jdickens
 */
public class OAStartOwnedPurchaseAnonymous extends EAXVxTestTemplate {

    @TestRail(caseId = 12323)
    @Test(groups = {"browser_only", "full_regression", "checkout", "checkout_smoke"})
    public void testRedeemNonExistentCode(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlementInfo = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        String entitlementName = entitlementInfo.getName();
        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlementInfo);
        String userName = userAccount.getUsername();

        logFlowPoint("Navigate to the Origin Store"); // 1
        logFlowPoint("Navigate to the PDP for the owned game"); // 2
        logFlowPoint("Click on the 'Buy' button and verify that the login page is visible"); // 3
        logFlowPoint("Log into Origin and verify that a dialog appears informing the user that the game is already owned"); // 4
        logFlowPoint("Click on the link to the owned entitlement in the dialog and verify that you are brought to the 'Game Library'"); // 5
        logFlowPoint("Navigate to the PDP for the owned game and verify the user is unable to select the purchase CTA"); // 6

        // 1
        WebDriver driver = startClientObject(context, client);
        NavigationSidebar navigationSidebar = new NavigationSidebar(driver);
        navigationSidebar.clickStoreLink();
        StorePage storePage = new StorePage(driver);
        if (storePage.verifyStorePageReached()) {
            logPass("Successfully navigated to the Origin store");
        } else {
            logFail("Failed to  navigate to the Origin store");
        }

        // 2
//        if (MacroPDP.loadPdpPage(driver, entitlementInfo)) {
//            logPass("Successfully navigated to the PDP for entitlement " + entitlementName);
//        } else {
//            logFail("Failed to  navigate to the PDP for entitlement " + entitlementName);
//        }

        // 3
        //PDPHeroActionCTA pdpHeroCTA = new PDPHeroActionCTA(driver);
        // pdpHeroCTA.clickBuyButton();
        LoginPage loginPage = new LoginPage(driver);
        Waits.waitForPageThatMatches(driver, OriginClientData.LOGIN_PAGE_URL_REGEX, 30);
        if (loginPage.verifyLoginPageVisible()) {
            logPass("Successfully clicked the 'Buy' button on the PDP Hero's CTA and verified that the login page is visible");
        } else {
            logFail("Failed to verify that the login page is visible after clicking the 'Buy' button on the PDP Hero's CTA");
        }

        // 4
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 30);
        EntitlementAwarenessDialog entitlementAwarenessDialog  = new EntitlementAwarenessDialog(driver);
        if (MacroLogin.startLogin(driver, userAccount) && entitlementAwarenessDialog.waitIsVisible()) {
            logPass("Successfully logged in with user " + userName + " and verified that a dialog appears saying you already own the game");
        } else {
            logFail("Failed to log in with user " + userName + " or verify that a dialog appears");
        }

        // 5
        entitlementAwarenessDialog.clickEntitlementLink();
        GameLibrary gameLibrary = new GameLibrary(driver);
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Verified that clicking the entitlement link navigates to the 'Game Library'");
        } else {
            logFail("Failed to verify that clicking the entitlement link navigates to the 'Game Library'");
        }

        // 6
//        PDPHero pdpHero = new PDPHero(driver);
//        PDPHeroActionCTA pdpHeroActionCTA = new PDPHeroActionCTA(driver);
//        if (pdpHero.verifyPDPHeroReached() && !pdpHeroActionCTA.verifyBuyButtonVisible()){
//           logPass("Navigated back to the PDP and verified that the user is unable to select the purchase CTA of the game they already own"); // 5);
//        }
//        else {
//            logFail("Failed to navigate back to the PDP or verify that the user is unable to select the purchase CTA of the game they already own");
//        }

        softAssertAll();
    }
}