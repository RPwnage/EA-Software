package com.ea.originx.automation.scripts.feature.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Login to Origin client with expired subscription account and check if vault
 * entitlement 'BF4 Digital Deluxe' is downloadable.
 *
 * @author Rocky
 */
public class OAExpiredSubscriptionDownload extends EAXVxTestTemplate {

    @TestRail(caseId = 3103326)
    @Test(groups = {"originaccess", "originaccess_smoke", "client_only", "allfeaturesmoke"})
    public void testOAExpiredSubscriptionDownload(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo battleField4Premium = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
        String bf4DDOfferID = battleField4Premium.getOfferId();

        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_EXPIRED_OWN_ENT);

        logFlowPoint("Log into client with an expired subscription"); //1
        logFlowPoint("Navigate to 'Game Library'"); //2
        logFlowPoint("Verify that the vault game in 'Game Library'"); //3
        logFlowPoint("Verify there is a violator stating Your 'Origin Access' membership has expired for the vault"); //4
        logFlowPoint("Verify the user is unable to begin downloading for the vault game"); //5

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user who has expired subscription.");
        } else {
            logFailExit("Could not log into Origin with the user who has expired subscription");
        }

        // 2
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigated to the 'Game Library'.");
        } else {
            logFailExit("Could not navigate to the 'Game Library'.");
        }

        // 3
        GameTile vaultGame = new GameTile(driver, bf4DDOfferID);
        if (gameLibrary.isGameTileVisibleByOfferID(bf4DDOfferID)) {
            logPass("Verified that the vault game is found in the 'Game Library'");
        } else {
            logFailExit("The vault game could not be found in the 'Game Library'");
        }

        // 4
        if (vaultGame.verifyViolatorStatingMembershipIsExpired()) {
            logPass("Verified that the vault game has a violator stating it is expired");
        } else {
            logFailExit("Could not find violator for vault entitlement stating it is expired");
        }

        // 5
        if (!vaultGame.isDownloadable()) {
            logPass("Verified that the vault game is not downloadable");
        } else {
            logFailExit("The vault game is downloadable");
        }

        softAssertAll();
    }
}
