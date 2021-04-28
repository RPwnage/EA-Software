package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.FileUtil;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests verifies the vault entitlement license gets removed when the offline
 * period expires
 *
 * @author cbalea
 */
public class OASubscriptionLicenseOfflineRemove extends EAXVxTestTemplate {

    @TestRail(caseId = 11076)
    @Test(groups = {"originacess", "full_regression", "client_only"})
    public void testOriginAccessLicenseOfflineRemove(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.ENTITLEMENTS_IN_LIBRARY_THROUGH_PURCHASE_AND_SUBSCRIPTION);
        final EntitlementInfo subscriptionEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.Limbo);
        final EntitlementInfo purchasedEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FE);
        final String subscriptionEntitlementOfferID = subscriptionEntitlement.getOfferId();
        final String purchasedEntitlementOfferID = purchasedEntitlement.getOfferId();
        final String subscriptionEntitlementProcess = subscriptionEntitlement.getProcessName();
        final String purchasedEntitlementmentProcess = purchasedEntitlement.getProcessName();
        final String licensePath = "C:\\ProgramData\\Electronic Arts\\EA Services\\License";
        final String subscriptionEntitlementLicense = "196395.dlf";
        final String purchasedEntitlementLicense = "1041863.dlf";

        logFlowPoint("Launch and log into Origin"); // 1
        logFlowPoint("Launch an entitlement added through subscription and a purchased non vault entitlement"); // 2
        logFlowPoint("Enter offline mode"); // 3
        logFlowPoint("Go to licenses cache and verify the subscription licenses have been removed"); // 4
        logFlowPoint("Verify the license for the purchased entitlement is not removed"); // 5

        // 1
        EACoreHelper.setSubscriptionMaxOfflinePlay(client.getEACore(), "60000");
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        FileUtil.deleteFolder(client, licensePath); // deleting folder to ensure test accuracy
        new NavigationSidebar(driver).gotoGameLibrary();
        MacroGameLibrary.downloadFullEntitlement(driver, subscriptionEntitlementOfferID);
        MacroGameLibrary.downloadFullEntitlement(driver, purchasedEntitlementOfferID);
        subscriptionEntitlement.addActivationFile(client);
        purchasedEntitlement.addActivationFile(client);
        new GameTile(driver, subscriptionEntitlementOfferID).play();
        boolean isSubscriptionEntitlementLaunched = subscriptionEntitlement.isLaunched(client);
        Waits.pollingWaitEx(() -> ProcessUtil.killProcess(client, subscriptionEntitlementProcess), 60000, 1000, 20000);
        new GameTile(driver, purchasedEntitlementOfferID).play();
        boolean isPurchasedEntitlementLaunched = purchasedEntitlement.isLaunched(client);
        Waits.pollingWaitEx(() -> ProcessUtil.killProcess(client, purchasedEntitlementmentProcess), 60000, 1000, 20000);
        logPassFail(isPurchasedEntitlementLaunched && isSubscriptionEntitlementLaunched, true);

        // 3
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectGoOffline();
        logPassFail(mainMenu.verifyOfflineModeButtonVisible(), isPurchasedEntitlementLaunched);

        // 4
        // the file is deleted after atleast 3 minutes have passed after going 'Offline'
        logPassFail(Waits.pollingWaitEx(() -> !FileUtil.isFileExist(client, licensePath + "\\" + subscriptionEntitlementLicense), 360000, 3000, 180000), true);

        // 5
        logPassFail(FileUtil.isFileExist(client, licensePath + "\\" + purchasedEntitlementLicense), true);

        softAssertAll();
    }
}
