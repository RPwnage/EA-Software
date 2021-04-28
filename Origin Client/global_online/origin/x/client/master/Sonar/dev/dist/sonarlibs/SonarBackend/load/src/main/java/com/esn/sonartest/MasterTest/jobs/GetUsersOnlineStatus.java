package com.esn.sonartest.MasterTest.jobs;

import com.esn.sonar.core.Message;
import com.esn.sonartest.MasterTest.MasterTestDirector;
import com.esn.sonartest.MasterTest.User;
import com.esn.sonartest.OperatorClient;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.JobTrigger;
import com.esn.sonartest.verifier.Expectation;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Random;

/**
 * Created by IntelliJ IDEA.
 * User: jonas
 * Date: 2011-06-23
 * Time: 16:14
 * To change this template use File | Settings | File Templates.
 */
public class GetUsersOnlineStatus extends AbstractJob
{
    private MasterTestDirector masterTestDirector;

    private static Random random = new Random();
    private static final int MAX_USERS = 15;

    private Collection<User> users;

    public GetUsersOnlineStatus(JobTrigger trigger, MasterTestDirector masterTestDirector)
    {
        super(trigger);
        this.masterTestDirector = masterTestDirector;
    }

    @Override
    protected boolean onStart() throws Exception
    {
        int numUsers = 1 + random.nextInt(MAX_USERS);
        users = new ArrayList<User>(numUsers);

        for (int index = 0; index < numUsers; index++)
        {
            User user = masterTestDirector.getUserInChannelPool().acquire();
            if (user == null) break;
            users.add(user);
        }

        if (users.size() == 0)
        {
            return false;
        }

        return true;
    }

    @Override
    protected void onRun() throws Exception
    {
        OperatorClient oc = masterTestDirector.getOperatorClientPool().acquire();
        oc.getUsersOnlineStatus(MasterTestDirector.operatorId, users, new Expectation("GetUsersOnlineStatus")
        {
            @Override
            public void verify(Message message)
            {
                for (User user : users)
                {
                    masterTestDirector.getUserInChannelPool().release(user);
                }
                signalDone();
            }
        });

        masterTestDirector.getOperatorClientPool().release(oc);
    }


}
