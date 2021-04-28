package com.esn.sonar.core;

import com.esn.sonar.core.util.Utils;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.util.HashMap;

public abstract class AbstractConfig
{
    protected HashMap<String, String> configParams = new HashMap<String, String>();

    public static class ConfigurationException extends Exception
    {
        public ConfigurationException(String msg)
        {
            super(msg);
        }
    }

    public AbstractConfig(String path) throws IOException, ConfigurationException
    {
        if (path == null)
        {
            return;
        }

        System.err.printf("Using config file %s\n", path);

        BufferedReader in = new BufferedReader(new FileReader(path));

        String line;

        try
        {
            while (true)
            {
                line = in.readLine();

                if (line == null)
                    return;

                if (line.startsWith("#"))
                    continue;

                line = line.trim();

                int offset = line.indexOf('#');

                if (offset != -1)
                {
                    line = line.substring(0, offset);
                }

                if (line.length() == 0)
                    continue;

                String[] args = Utils.tokenize(line, '=');

                if (args == null || args.length < 2)
                {
                    throw new ConfigurationException("Line '" + line + "' is invalid");
                }

                String name = args[0].trim();
                String value = args[1].trim();

                configParams.put(name.toLowerCase(), value);
            }
        } finally
        {
            try
            {
                in.close();
            } catch (IOException e)
            {
                // ignore, at least we tried
            }
        }
    }

    private static InetSocketAddress StringToInetAddress(String address)
    {
        int i = address.lastIndexOf(':');

        if (i == -1)
        {
            throw new IllegalArgumentException("No ':' found in address string " + address);
        }

        String hostname = address.substring(0, i);
        String portString = address.substring(i + 1, address.length());
        int port = Integer.parseInt(portString);

        if (port < 0 || port > 65536)
        {
            throw new IllegalArgumentException("Port has invalid range in " + address);
        }

        return new InetSocketAddress(hostname, port);
    }

    static public String guessLocalAddress()
    {
        try
        {
            Socket socket = new Socket();
            SocketAddress ra = new InetSocketAddress("www.google.com", 80);
            socket.connect(ra);
            InetAddress la = socket.getLocalAddress();
            socket.close();
            return la.toString().substring(1);
        } catch (IOException e)
        {
            return null;
        }
    }

    protected String getConfigValue(String inName)
    {
        String name = inName.toLowerCase();

        System.err.printf("%s: ", name);

        String cp = configParams.get(name);
        String sp = System.getProperty(name);

        if (sp == null)
        {
            sp = System.getProperty(inName);
        }

        if (cp == null)
        {
            if (sp == null)
                System.err.printf("default\n");
            else
                System.err.printf("%s (from command-line)\n", sp);

            return sp;
        }

        if (sp == null)
        {
            System.err.printf("%s (from config)\n", cp);
            return cp;
        }

        System.err.printf("%s (from command-line)\n", sp);
        return sp;
    }


    protected Integer readFromSystem(String propertyName, int defValue)
    {
        String sv = getConfigValue(propertyName);

        if (sv == null)
        {
            return defValue;
        }

        return Integer.parseInt(sv);
    }


    protected Long readFromSystem(String propertyName, long defValue)
    {
        String sv = getConfigValue(propertyName);

        if (sv == null)
        {
            return defValue;
        }

        return Long.parseLong(sv);
    }

    protected Double readFromSystem(String propertyName, double defValue)
    {
        String sv = getConfigValue(propertyName);

        if (sv == null)
        {
            return defValue;
        }

        return Double.parseDouble(sv);
    }

    protected InetSocketAddress readFromSystem(String propertyName, InetSocketAddress defValue)
    {

        String sv = getConfigValue(propertyName);

        if (sv == null)
        {
            return defValue;
        }

        return StringToInetAddress(sv);
    }

    protected String readFromSystem(String propertyName, String defValue)
    {
        String sv = getConfigValue(propertyName);

        if (sv == null)
        {
            return defValue;
        }

        return sv;
    }


    @Override
    protected Object clone()
    {
        return Utils.clone(this);
    }

}
