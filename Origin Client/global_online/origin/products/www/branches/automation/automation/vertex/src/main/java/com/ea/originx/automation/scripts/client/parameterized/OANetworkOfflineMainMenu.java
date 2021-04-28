package com.ea.originx.automation.scripts.client.parameterized;

import com.ea.originx.automation.scripts.client.OAOfflineMainMenu;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the functionality of the main menu when the network is offline
 *
 * @author lscholte
 */
public class OANetworkOfflineMainMenu extends OAOfflineMainMenu {

    @TestRail(caseId = 39464)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testNetworkOfflineMainMenu(ITestContext context) throws Exception {
        testOfflineMainMenu(context, false, this.getClass().getName());
    }

}
