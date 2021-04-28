package com.esn.sonar.allinone.util;

import com.esn.sonar.core.Token;
import com.esn.sonar.core.util.Utils;

import java.io.IOException;
import java.security.PrivateKey;

public class VoiceClientRunner extends ProcessRunner
{
    private Token token;
    private String reasonType;
    private String reasonDesc;

    public VoiceClientRunner(Token token, PrivateKey privateKey) throws IOException, InterruptedException
    {
        this.token = token;

        StringBuilder builder = new StringBuilder();
        builder.append("../ReleaseStatic/SonarRefConnection.exe ");
        builder.append(token.sign(privateKey));

        launch(builder.toString());
    }

    public Token getToken()
    {
        return token;
    }

    @Override
    protected void onStderr(String line)
    {
        if (!line.startsWith("onDisconnect:"))
        {
            return;
        }

        String args = line.substring(line.indexOf(':') + 2);
        String[] tokenize = Utils.tokenize(args, ' ');

        this.reasonType = tokenize[0];
        this.reasonDesc = tokenize[1];
    }

    @Override
    protected void onStdout(String line)
    {
        onStderr(line);
    }

    @Override
    public void shutdown() throws InterruptedException, IOException
    {
        stdinWriter.write('\n');
        stdinWriter.flush();
        process.waitFor();
        super.shutdown();
    }

    public String getReasonType()
    {
        return reasonType;
    }

    public String getReasonDesc()
    {
        return reasonDesc;
    }
}
