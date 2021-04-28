package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
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
import java.io.IOException;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests if the license files are restored when launching an entitlement from
 * vault
 *
 * @author cbalea
 */
public class OAOriginAccessLicenseRestore extends EAXVxTestTemplate {

    @TestRail(caseId = 11071)
    @Test(groups = {"originaccess", "full_regression", "client_only"})
    public void testOriginAccessLicenseRestore(ITestContext context) throws Exception, IOException {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_LICENSE_TEST);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.Limbo);
        final String offerId = entitlement.getOfferId();
        final String licensePath = "C:\\ProgramData\\Electronic Arts\\EA Services\\License";

        logFlowPoint("Launch and log into Origin with an account with full subscription"); // 1
        logFlowPoint("Download and install an 'Origin Access' entitlement then verify that 'Origin' downloads a license file"); // 2
        logFlowPoint("Verify entitlement launches"); // 3

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        FileUtil.deleteFolder(client, licensePath); // making sure license directory does not exist before launching entitlement
        new NavigationSidebar(driver).gotoGameLibrary();
        MacroGameLibrary.downloadFullEntitlement(driver, offerId);
        boolean isLicenseAfterInstall = FileUtil.isDirectoryExist(client, licensePath);
        logPassFail(isLicenseAfterInstall, true);

        // 3
        new GameTile(driver, offerId).play();
        logPassFail(entitlement.isLaunched(client), true);
        sleep(2000); // waiting for entitlement to fully launch to avoid cloud save conflict
        ProcessUtil.killProcess(client, entitlement.getProcessName());

        softAssertAll();
    }
}
