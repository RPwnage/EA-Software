package com.esn.sonartest.MasterTest.jobs;

import com.esn.sonar.core.Message;
import com.esn.sonar.core.Token;
import com.esn.sonartest.MasterTest.MasterTestDirector;
import com.esn.sonartest.MasterTest.User;
import com.esn.sonartest.OperatorClient;
import com.esn.sonartest.UserEdgeClient;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;
import com.esn.sonartest.verifier.EventVerifier;
import com.esn.sonartest.verifier.Expectation;

public class UserConnect extends AbstractJob
{
    private MasterTestDirector masterTestDirector;
    private User user;

    public UserConnect(JobTrigger trigger, MasterTestDirector masterTestDirector)
    {
        super(trigger);
        this.masterTestDirector = masterTestDirector;
    }

    @Override
    protected boolean onStart() throws Exception
    {
        this.user = masterTestDirector.getDisconnectedUserPool().acquire();

        if (user == null)
        {
            return false;
        }

        return true;
    }

    @Override
    protected void onRun() throws Exception
    {
        Expectation eventExpectation = new Expectation("UserConnectedEvent")
        {
            @Override
            public void verify(Message message)
            {

                if (!message.getArgument(1).equals(user.getUserId()))
                {
                    throw new RuntimeException("User doesn't match");
                }

                if (user.getEdgeClient() == null)
                {
                    throw new RuntimeException();
                }

                masterTestDirector.getConnectedUserPool().add(user);
                signalDone();
            }
        };

        Expectation resultExpectation = new Expectation("getUserControlToken")
        {
            @Override
            public void verify(Message message)
            {
                Token token = null;
                try
                {
                    token = new Token(message.getArgument(1), null, -1);
                } catch (Token.InvalidToken invalidToken)
                {
                    throw new RuntimeException(invalidToken);
                }

                UserEdgeClient userEdgeClient = masterTestDirector.getUserEdgeClientByPort(token.getControlPort());
                userEdgeClient.connectUser(user);
            }
        };


        EventVerifier.getInstance().addUserShouldConnect(user, eventExpectation);

        OperatorClient oc = masterTestDirector.getOperatorClientPool().acquire();
        oc.getUserControlToken(MasterTestDirector.operatorId, user.getUserId(), user.getUserDesc(), "", "", "", resultExpectation);
        masterTestDirector.getOperatorClientPool().release(oc);


    }

}
