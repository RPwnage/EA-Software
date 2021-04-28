package com.ea.originx.automation.scripts.cloudsave.parameterized;

import com.ea.originx.automation.scripts.cloudsave.OACloudSave;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests 'New cloud save data' dialog when user has a local save file and tries to save the local save to the cloud.
 *
 * @author nvarthakavi
 */
public class OACloudSaveLocalSave extends OACloudSave {

    @TestRail(caseId = 1016730)
    @Test(groups = {"services_smoke", "client_only", "cloudsaves", "int_only"})
    public void testCloudSaveLocalSave(ITestContext context) throws Exception {
        testCloudSave(context,TEST_TYPE.SAVE);
    }
}
