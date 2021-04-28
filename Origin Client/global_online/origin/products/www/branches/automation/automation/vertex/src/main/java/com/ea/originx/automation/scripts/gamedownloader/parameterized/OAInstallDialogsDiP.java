package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAInstallDialogs;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test case has boolean flag dipSmall=True to handle parameterization
 *
 * @author mkalaivanan
 */
public class OAInstallDialogsDiP extends OAInstallDialogs {

    @TestRail(caseId = 9844)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testInstallDialogDip(ITestContext context) throws Exception {
        testInstallDialog(context, true, this.getClass().getName());
    }

}
