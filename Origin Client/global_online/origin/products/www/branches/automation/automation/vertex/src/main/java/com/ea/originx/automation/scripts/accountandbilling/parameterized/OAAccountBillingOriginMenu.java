package com.ea.originx.automation.scripts.accountandbilling.parameterized;

import com.ea.originx.automation.scripts.accountandbilling.OAAccountBilling;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests navigating to the 'EA Account and Billing' page from the 'Main Menu'
 *
 * @author cbalea
 */
public class OAAccountBillingOriginMenu extends OAAccountBilling{
    
    @TestRail(caseId = 9602)
    @Test(groups = {"client_only", "accountandbilling", "full_regression"})
     public void testOAAccountBillingOriginMenu(ITestContext context) throws Exception{
        testAccountBilling(context, OAAccountBilling.TEST_TYPE.ACCOUNT_BILLING_ORIGIN_MENU);
    }
}
