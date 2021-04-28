package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAShortcut;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * @author lscholte
 */
public class OADesktopShortcut extends OAShortcut {

    @TestRail(caseId = 9859)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testDesktopShortcut(ITestContext context) throws Exception {
        testShortcut(context, true, this.getClass().getName());
    }

}
