package com.ea.originx.automation.scripts.feature.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test to Upgrade and Remove an Upgrade from a Vault Entitlement
 *
 * @author glivingstone
 */
public abstract class OAUpgradeEntitlement extends EAXVxTestTemplate {

    /**
     * The test function which the parameterized test cases call. Used by
     * OAUpgradeEntitlementBattlefield and OAUpgradeEntitlementCrysis.
     *
     * @param context     The context we are using
     * @param entStandard The EntitlementIds for the standard version of the
     *                    entitlement
     * @param entUpgraded The EntitlementIds for the upgraded version of the
     *                    entitlement
     * @throws Exception
     */
    public void testUpgradeEntitlement(ITestContext context, EntitlementId entStandard, EntitlementId entUpgraded) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo standardEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(entStandard);
        EntitlementInfo upgradedEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(entUpgraded);
        String standardEntName = standardEntitlement.getName();
        String upgradedEntName = upgradedEntitlement.getName();

        int TILE_WAIT_TIMEOUT = 10;

        UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_UPGRADE_TEST);

        final String username = userAccount.getUsername();

        logFlowPoint("Login to Origin as " + username + " and navigate to the Game Library"); // 1
        logFlowPoint(String.format("Verify %s is not in the Game Library; Remove if it is", upgradedEntName)); // 2
        logFlowPoint(String.format("Verify %s is in the Game Library", standardEntName)); // 3
        logFlowPoint(String.format("Open the Slideout for %s", standardEntName)); // 4
        logFlowPoint("Click 'Upgrade Now' on the Slideout and Close the Confirmation Dialog"); // 5
        logFlowPoint(String.format("Verify %s is in the Game Library", upgradedEntName)); // 6
        logFlowPoint(String.format("Verify %s is not in the Game Library", standardEntName)); // 7
        logFlowPoint(String.format("Remove %s from the Library", upgradedEntName)); // 8
        logFlowPoint(String.format("Verify %s is not in the Game Library", upgradedEntName)); // 9
        logFlowPoint(String.format("Verify %s Standard is in the Game Library", standardEntName)); // 10

        // 1
        final WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, userAccount);
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        // 2
        if (gameLibrary.isGameTileVisibleByName(upgradedEntName, TILE_WAIT_TIMEOUT)) {
            MacroGameLibrary.removeFromLibrary(driver, upgradedEntitlement.getOfferId());
        }
        logPassFail(!gameLibrary.isGameTileVisibleByName(upgradedEntName, TILE_WAIT_TIMEOUT), true);
        
        // 3
        logPassFail(gameLibrary.isGameTileVisibleByName(standardEntName, TILE_WAIT_TIMEOUT), true);

        // 4
        GameTile standardEntitlementTile = new GameTile(driver, standardEntitlement.getOfferId());
        GameSlideout standardEntitlementSlideout = standardEntitlementTile.openGameSlideout();
        logPassFail(standardEntitlementSlideout.verifyGameTitle(standardEntName), true);

        // 5
        standardEntitlementSlideout.upgradeEntitlement();
        CheckoutConfirmation vaultConfirmationDialog = new CheckoutConfirmation(driver);
        Waits.pollingWait(() -> vaultConfirmationDialog.isDialogVisible(), 20000, 1000, 10000);
        vaultConfirmationDialog.clickCloseCircle();
        logPassFail(!vaultConfirmationDialog.isDialogVisible(), true);

        // 6
        logPassFail(Waits.pollingWait(() -> gameLibrary.isGameTileVisibleByName(upgradedEntName, TILE_WAIT_TIMEOUT)), true);

        // 7
        logPassFail(!gameLibrary.isGameTileVisibleByName(standardEntName, TILE_WAIT_TIMEOUT), true);

        // 8
        logPassFail(MacroGameLibrary.removeFromLibrary(driver, upgradedEntitlement.getOfferId()), false);

        // 9
        logPassFail(!gameLibrary.isGameTileVisibleByName(upgradedEntName, TILE_WAIT_TIMEOUT), true);

        // 10
        logPassFail(gameLibrary.isGameTileVisibleByName(standardEntName, TILE_WAIT_TIMEOUT), true);

        softAssertAll();
    }
}
