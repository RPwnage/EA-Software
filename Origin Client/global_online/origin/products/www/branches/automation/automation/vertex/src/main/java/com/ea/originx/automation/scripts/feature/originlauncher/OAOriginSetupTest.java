package com.ea.originx.automation.scripts.feature.originlauncher;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.setup.ThinSetup;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

public class OAOriginSetupTest extends EAXVxTestTemplate {

    @TestRail(caseId = 3120664)
    @Test(groups = {"originlauncher", "originlauncher_smoke"})
    public void localTest(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        logFlowPoint("1"); // 1

        final WebDriver driver = startClientObject(context, client);
        final ThinSetup thinSetup = new ThinSetup(driver);
        thinSetup.clickInstallOrigin();
        thinSetup.toggleAutoUpdateCheckbox(false);
        thinSetup.toggleAcceptEULACheckbox(true);
        thinSetup.clickContinue();
        if (thinSetup.waitInstallationSucceeded()) {
            logPass("1");
        } else {
            logFailExit("1");
        }

        softAssertAll();
    }

}
