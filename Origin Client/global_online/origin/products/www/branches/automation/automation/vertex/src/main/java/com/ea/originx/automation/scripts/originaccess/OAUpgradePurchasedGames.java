package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.macroaction.MacroRedeem;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the possibility to upgrade entitlement to 'Vault' version when owning a
 * lesser version
 *
 * @author cbalea
 */
public class OAUpgradePurchasedGames extends EAXVxTestTemplate {

    @TestRail(caseId = 11107)
    @Test(groups = {"originaccess", "full_regression","release_smoke"})
    public void testUpgradePurchasedGames(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final EntitlementInfo premiunEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
        final String entitlementOfferId = entitlement.getOfferId();

        logFlowPoint("Launch and log into Origin"); // 1
        logFlowPoint("Navigate to the GDP of an entitlement included in the vault"); // 2
        logFlowPoint("Purchase standard edition"); // 3
        logFlowPoint("Navigate to 'Game Library' and verify game tile has a message stating that it can be upgraded to the 'Vault' version"); // 4
        logFlowPoint("Open OGD by clicking on the game tile and verify the OGD indicates that it can be upgraded to the 'Vault' version"); // 5
        logFlowPoint("Click on the 'Upgrade Now' link and verify that the purchased game gets upgraded to the 'Origin Access' edition of the game."); // 6
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        if (OSInfo.getEnvironment().equalsIgnoreCase("production")) {
            new MainMenu(driver).selectRedeemProductCode();
            MacroRedeem.redeemOAPremierSubscription(driver, userAccount.getEmail());
        } else {
            MacroOriginAccess.purchaseOriginPremierAccess(driver);
        }

        // 3
        if (OSInfo.getEnvironment().equalsIgnoreCase("production")) {
            logPassFail(MacroPurchase.purchaseThroughPaypalOffCode(driver, entitlement, userAccount.getEmail()), true);
        } else {
            logPassFail(MacroPurchase.purchaseEntitlement(driver, entitlement), true);
        }

        // 4
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        logPassFail(gameTile.verifyViolatorUpgradeGameVisible(), true);

        // 5
        GameSlideout gameSlideout = gameTile.openGameSlideout();
        gameSlideout.waitForSlideout();
        logPassFail(gameSlideout.verifyViolatorUpgradeGameVisible(), true);

        // 6
        gameSlideout.clickUpgradeNowLink();
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        checkoutConfirmation.waitIsVisible();
        checkoutConfirmation.closeAndWait();
        logPassFail(gameLibrary.isGameTileVisibleByOfferID(premiunEntitlement.getOfferId()), true);
    }
}