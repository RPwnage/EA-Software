package com.esn.sonar.allinone;

import com.esn.sonar.master.api.operator.OperatorService;
import com.esn.sonar.allinone.client.FakeOperatorClient;
import org.junit.After;

public abstract class OperatorServiceTabProtocolTest extends OperatorServiceInVmTest
{
    private FakeOperatorClient operatorClient;

    @Override
    protected OperatorService createOperatorService() throws InterruptedException
    {
        operatorClient = clientFactory.newFakeOperatorClient(config);
        operatorClient.connect().waitForConnect();
        return operatorClient;
    }

    @After
    public void cleanUp()
    {
        operatorClient.close();
    }

}
