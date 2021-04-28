package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FileUtil;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test licenses are not restored when 'Origin' is launched for a subscribed
 * user
 *
 * @author cbalea
 */
public class OAOriginAccessLicenseNoRestoreOriginLaunch extends EAXVxTestTemplate {

    @TestRail(caseId = 11070)
    @Test(groups = {"full_regression", "client_only"})
    public void OAOriginAccessLicenseNoRestoreOriginLaunch(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE);
        final String licensePath = "C:\\ProgramData\\Electronic Arts\\EA Services\\License";

        logFlowPoint("Without launching 'Origin' go to 'C:\\ProgramData\\Electronic Arts\\EA Services\\License' and delete license files"); // 1
        logFlowPoint("Launch 'Origin' and verify license files are not restored"); // 2

        // 1
        WebDriver driver = startClientObject(context, client);
        FileUtil.deleteFolder(client, licensePath);
        logPassFail(!FileUtil.isDirectoryExist(client, licensePath), true);

        // 2
        MacroLogin.startLogin(driver, userAccount);
        logPassFail(!FileUtil.isDirectoryExist(client, licensePath), true);

        softAssertAll();
    }
}
