package com.ea.originx.automation.scripts.client.parameterized;

import com.ea.originx.automation.scripts.client.OAOfflineMainMenu;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the functionality of the main menu when in offline mode
 *
 * @author lscholte
 */
public class OAOfflineModeMainMenu extends OAOfflineMainMenu {

    @TestRail(caseId = 39464)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testOfflineModeMainMenu(ITestContext context) throws Exception {
        testOfflineMainMenu(context, true, this.getClass().getName());
    }

}
