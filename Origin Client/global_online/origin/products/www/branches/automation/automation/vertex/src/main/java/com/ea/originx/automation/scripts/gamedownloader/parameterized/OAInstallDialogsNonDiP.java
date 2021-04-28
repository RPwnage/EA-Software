package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAInstallDialogs;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test case parameterized to have boolean flag dipSmall=False to the
 * handle the flow.
 *
 * @author mkalaivanan
 */
public class OAInstallDialogsNonDiP extends OAInstallDialogs {

    @TestRail(caseId = 9835)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testInstallDialogNonDip(ITestContext context) throws Exception {
        testInstallDialog(context, false, this.getClass().getName());
    }

}
