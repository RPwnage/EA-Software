package com.esn.sonartest.UserEdgeTest;

import com.esn.sonar.core.AbstractConfig;
import com.esn.sonar.core.Maintenance;
import com.esn.sonar.core.Token;
import com.esn.sonar.core.util.Utils;
import com.esn.sonartest.OperatorClient;
import com.esn.sonartest.UserEdgeTest.jobs.ConnectUserJob;
import com.esn.sonartest.UserEdgeTest.jobs.DisconnectUserJob;
import com.esn.sonartest.UserEdgeTest.jobs.JoinUserToChannelJob;
import com.esn.sonartest.director.AbstractJob;
import com.esn.sonartest.director.Director;
import com.esn.sonartest.director.JobTrigger;
import com.esn.sonartest.director.Pool;

import java.io.IOException;
import java.security.KeyPair;
import java.security.NoSuchAlgorithmException;
import java.security.spec.InvalidKeySpecException;
import java.util.Collection;

/**
 * User: ronnie
 * Date: 2011-06-29
 * Time: 11:46
 */
public class UserEdgeTestDirector extends Director
{
    public static final String LOCAL_ADDRESS = AbstractConfig.guessLocalAddress();
    private UserEdgeTestConfig config;
    private final Pool<User> disconnectUserPool = new Pool<User>();
    private Pool<User> connectedUserPool = new Pool<User>();
    private KeyPair keyPair;
    private Pool<OperatorClient> operatorPool = new Pool<OperatorClient>();

    public UserEdgeTestDirector(UserEdgeTestConfig config) throws IOException, InterruptedException, InvalidKeySpecException, NoSuchAlgorithmException
    {
        Utils.create();
        Maintenance.configure(100);
        this.config = config;
        this.keyPair = Utils.loadOrGenerateKeyPair(System.getProperty("user.home"), "sonarmaster.pub", "sonarmaster.prv");
    }

    @Override
    protected void onStart() throws Exception
    {

        for (int uid = config.getUserStart(); uid < config.getUserEnd(); uid++)
        {
            String userId = String.format("UID-%08x", uid);
            String userDesc = String.format("Description of UID-%08x", uid);

            Token token = new Token("OPER", userId, userDesc, "", "", "", 0, "", 0, "", 0);

            User user = new User(token, keyPair.getPrivate(), getBootstrap(), config.getUserEdgeAddress());
            disconnectUserPool.add(user);
        }

        for (int index = 0; index < config.getOperatorCount(); index++)
        {
            OperatorClient operatorClient = new OperatorClient(config.getOperatorAddress(), getBootstrap());

            operatorClient.connect().awaitUninterruptibly();
            operatorPool.add(operatorClient);
        }
    }

    @Override
    protected void onStop() throws Exception
    {
        disconnectUserPool.acquireAll();
        Collection<User> users = connectedUserPool.acquireAll();
        for (User user : users)
        {
            user.close().awaitUninterruptibly();
        }

        Collection<OperatorClient> operatorClients = operatorPool.acquireAll();

        for (OperatorClient operatorClient : operatorClients)
        {
            operatorClient.close().awaitUninterruptibly();
        }
    }

    public static void main(String[] args) throws Exception
    {
        UserEdgeTestConfig config = new UserEdgeTestConfig(System.getProperty("config"));

        new UserEdgeTestDirector(config).start();
    }

    @Override
    protected JobTrigger[] setupJobTriggers()
    {
        return new JobTrigger[]{
                new JobTrigger(workerThreadPool, "ConnectUser", config.getUserConnects())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new ConnectUserJob(this, UserEdgeTestDirector.this);
                    }
                },

                new JobTrigger(workerThreadPool, "DisconnectNice", config.getUserDisconnect())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new DisconnectUserJob(this, UserEdgeTestDirector.this, true);
                    }
                },

                new JobTrigger(workerThreadPool, "JoinUser", config.getJoinUser())
                {
                    @Override
                    public AbstractJob createJob()
                    {
                        return new JoinUserToChannelJob(this, UserEdgeTestDirector.this);
                    }
                }


        };
    }

    public Pool<User> getDisconnectedUserPool()
    {
        return disconnectUserPool;
    }

    public Pool<User> getConnectedUserPool()
    {
        return connectedUserPool;
    }

    public Pool<OperatorClient> getOperatorPool()
    {
        return operatorPool;
    }

    public String getRandomChannelId()
    {
        return String.format("CID-%08x", random.nextInt(100000));
    }
}
