package com.ea.originx.automation.scripts.originaccess.parameterized;

import com.ea.originx.automation.scripts.originaccess.OAEditPaymentMethodPaypal;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test users ability to edit 'Paypal' payment method from the account portal for
 * a premier subscriber
 *
 * @author cdeaconu
 */
public class OAEditPaymentMethodPaypalPremierSubscriber extends OAEditPaymentMethodPaypal{
    
    @TestRail(caseId = 3689805)
    @Test(groups = {"originaccess", "full_regression"})
    public void testEditPaymentMethodPaypalPremierSubscriber(ITestContext context) throws Exception {
        testEditPaymentMethodPaypal(context, true);
    }
}