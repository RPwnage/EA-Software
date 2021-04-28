package com.ea.originx.automation.scripts.accountandbilling.parameterized;

import com.ea.originx.automation.scripts.accountandbilling.OAAccountBilling;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests navigating to the 'Account and Billing' page from the 'Mini Profile'.
 *
 * @author cbalea
 */
public class OAAccountBillingSidebar extends OAAccountBilling{
    
    @TestRail(caseId = 9603)
    @Test(groups = {"accountandbilling", "full_regression"})
     public void testOAAccountBillingOriginMenu(ITestContext context) throws Exception{
        testAccountBilling(context, OAAccountBilling.TEST_TYPE.ACCOUNT_BILLING_SIDEBAR);
    }
}
