package com.ea.originx.automation.scripts.tools;

import com.ea.originx.automation.lib.helpers.csv.OriginEntitlementHelper;
import com.ea.originx.automation.lib.helpers.csv.OriginEntitlementReader;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.net.helpers.CrsHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * To check all games's GDP page exist
 *
 * @author svaghayenegar
 */
public class OAGDPPageLoads extends EAXVxTestTemplate {

    @Test(groups = {"gdp_tools"})
    public void OAGDPPageLoads(ITestContext context) throws Exception {
        // Get the report id and the current test-result-id to get the index number to load the corresponding GDP
        String reportID = CrsHelper.getReportId();
        String testCaseName = "com.ea.originx.automation.scripts.tools.OAGDPPageLoads";

        // Get the TestCase Id from the testcase Name
        int testCaseID = CrsHelper.getTestCaseId(testCaseName);
        int index = Integer.parseInt(resultLogger.getCurrentTestResultId()) - CrsHelper.getTestResultId(reportID, testCaseID, true);
        OriginEntitlementHelper originEntitlementHelper = OriginEntitlementReader.getEntitlementInfo(index);

        EntitlementInfo entitlement = originEntitlementHelper.getAsEntitlementInfo();

        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getRandomAccount();
        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint(String.format("Go to GDP page of [%s] and verify GDP page loads", entitlement.getName())); // 2


        WebDriver driver = startClientObject(context, client);

        // 1
        if (isClient) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        //2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        softAssertAll();
    }
}
