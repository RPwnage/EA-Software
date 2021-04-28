package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAShortcut;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the start menu shortcuts.
 *
 * @author lscholte
 */
public class OAStartMenuShortcut extends OAShortcut {

    @TestRail(caseId = 9860)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testStartMenuShortcut(ITestContext context) throws Exception {
        testShortcut(context, false, this.getClass().getName());
    }
}