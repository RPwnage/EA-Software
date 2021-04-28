package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.UpgradeWarningMessageDialog;
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
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test verifies the 'Upgrade Warning' dialog contents
 *
 * @author cbalea
 */
public class OAFirstLaunchUpgradeWarningMessage extends EAXVxTestTemplate {

    @TestRail(caseId = 1534204)
    @Test(groups = {"originaccess", "full_regression", "client_only"})
    public void testFirstLaunchUpgradeWarningMessage(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);
        final String entitlementOfferId = entitlement.getOfferId();

        logFlowPoint("Launch and log into Origin"); // 1
        logFlowPoint("Purchase 'Origin Access' and a base edition of a vault game"); // 2
        logFlowPoint("Navigate to Game Library and download entitlement"); // 3
        logFlowPoint("Expand the OGD for the base game, click on 'Play' CTA and verify warning modal opens"); // 4
        logFlowPoint("Verify the warning modal title"); // 5
        logFlowPoint("Verify there is an upgrade available message in the modal"); // 6
        logFlowPoint("Verify there is a saved games upgrade warning message"); // 7
        logFlowPoint("Verify the 'Upgrade' and 'No Thanks' CTA's are displayed"); // 8

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        MacroOriginAccess.purchaseOriginPremierAccess(driver);
        logPassFail(MacroPurchase.purchaseEntitlement(driver, entitlement), true);

        // 3
        new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId), true);

        // 4
        GameSlideout gameSlideout = new GameTile(driver, entitlementOfferId).openGameSlideout();
        gameSlideout.waitForSlideout();
        gameSlideout.startPlay(30);
        UpgradeWarningMessageDialog upgradeWarningMessageDialog = new UpgradeWarningMessageDialog(driver);
        logPassFail(upgradeWarningMessageDialog.waitIsVisible(), true);

        // 5
        logPassFail(upgradeWarningMessageDialog.verifyDialogTitle(), false);

        // 6
        logPassFail(upgradeWarningMessageDialog.verifyUpgradeMessageWarning(), false);

        // 7
        logPassFail(upgradeWarningMessageDialog.verifyNewSaveGameWarning(), false);

        // 8
        logPassFail(upgradeWarningMessageDialog.verifyUpgradeNowCTA(), false);

        // 9
        logPassFail(upgradeWarningMessageDialog.verifyNoThanksCTA(), true);

        softAssertAll();
    }
}
