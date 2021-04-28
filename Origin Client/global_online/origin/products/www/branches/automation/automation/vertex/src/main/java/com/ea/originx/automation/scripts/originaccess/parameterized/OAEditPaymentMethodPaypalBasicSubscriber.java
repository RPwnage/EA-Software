package com.ea.originx.automation.scripts.originaccess.parameterized;

import com.ea.originx.automation.scripts.originaccess.OAEditPaymentMethodPaypal;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test users ability to edit 'Paypal' payment method from the account portal for
 * a basic subscriber
 *
 * @author cdeaconu
 */
public class OAEditPaymentMethodPaypalBasicSubscriber extends OAEditPaymentMethodPaypal{
    
    @TestRail(caseId = 11022)
    @Test(groups = {"originaccess", "full_regression"})
    public void testEditPaymentMethodPaypalBasicSubscriber(ITestContext context) throws Exception {
        testEditPaymentMethodPaypal(context, false);
    }
}