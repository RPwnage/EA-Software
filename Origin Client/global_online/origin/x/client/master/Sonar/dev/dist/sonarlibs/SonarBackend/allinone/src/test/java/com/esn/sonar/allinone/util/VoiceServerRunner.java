package com.esn.sonar.allinone.util;

import java.io.IOException;
import java.util.concurrent.locks.Condition;

public class VoiceServerRunner extends ProcessRunner
{
    public VoiceServerRunner(int controlPort) throws IOException, InterruptedException
    {
        StringBuilder sb = new StringBuilder();

        sb.append("../ReleaseStatic/SonarVoiceServer.exe");
        sb.append(" -voipport 0");
        sb.append(" -shutdowndelay 120");
        sb.append(" -backendaddress 127.0.0.1:");
        sb.append(controlPort);

        launch(sb.toString());
        while (voicePort == -1)
            Thread.sleep(20);
    }

    @Override
    protected void onStderr(String line)
    {
        if (voicePort != -1)
            return;

        if (!line.startsWith("Public VoIP address:"))
            return;

        String s = line.substring(line.lastIndexOf(':') + 1);

        voicePort = Integer.parseInt(s);
    }

    @Override
    protected void onStdout(String line)
    {
        onStderr(line);
    }

    private int voicePort = -1;

    public int getVoipPort()
    {
        return voicePort;
    }


}
