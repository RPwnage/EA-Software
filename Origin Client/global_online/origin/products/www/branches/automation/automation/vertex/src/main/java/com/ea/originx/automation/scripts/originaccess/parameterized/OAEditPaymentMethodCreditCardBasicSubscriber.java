package com.ea.originx.automation.scripts.originaccess.parameterized;

import com.ea.originx.automation.scripts.originaccess.OAEditPaymentMethodCreditCard;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test users ability to edit 'Credit Card' payment method from the account portal for
 * a basic subscriber
 *
 * @author ivlim
 */
public class OAEditPaymentMethodCreditCardBasicSubscriber extends OAEditPaymentMethodCreditCard {
    
    @TestRail(caseId = 11021)
    @Test(groups = {"originaccess", "full_regression"})
    public void testEditPaymentMethodPaypalBasicSubscriber(ITestContext context) throws Exception {
        testEditPaymentMethodCreditCard(context, false);
    }
}