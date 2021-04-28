package com.esn.sonar.allinone.util;

import java.io.*;

public class ProcessRunner
{
    public void launch(String path) throws IOException, InterruptedException
    {
        process = Runtime.getRuntime().exec(path);

        stderrReader = new BufferedReader(new InputStreamReader(process.getErrorStream()));
        stdoutReader = new BufferedReader(new InputStreamReader(process.getInputStream()));
        stdinWriter = new BufferedWriter(new OutputStreamWriter(process.getOutputStream()));

        stderrThread = new Thread(new Runnable()
        {
            public void run()
            {
                try
                {
                    String line;
                    while ((line = stderrReader.readLine()) != null)
                        onStderr(line);
                } catch (IOException e)
                {
                    return;
                }
            }
        });


        stdoutThread = new Thread(new Runnable()
        {
            public void run()
            {
                try
                {
                    String line;
                    while ((line = stdoutReader.readLine()) != null)
                        onStdout(line);
                } catch (IOException e)
                {
                    return;
                }
            }
        });

        stdoutThread.start();
        stderrThread.start();
    }

    public void shutdown() throws InterruptedException, IOException
    {
        process.destroy();
        stderrThread.join();
        stdoutThread.join();
    }

    protected void onStderr(String line)
    {
    }

    ;

    protected void onStdout(String line)
    {
    }

    ;

    protected Process process;
    private BufferedReader stderrReader;
    private BufferedReader stdoutReader;
    protected BufferedWriter stdinWriter;

    private Thread stderrThread;
    private Thread stdoutThread;


}
