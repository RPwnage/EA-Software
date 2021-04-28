package com.esn.sonar.core;

import org.jboss.netty.util.CharsetUtil;

import java.util.Arrays;

public class Message
{
    private final long id;
    private final String name;
    private final String[] args;

    public Message(long id, String name, String... args)
    {
        this.id = id & 0xffffffffL;
        this.args = args;
        this.name = name;
    }

    public String getArgument(int index)
    {
        return args[index];
    }

    public String getName()
    {
        return name;
    }

    public int getArgumentCount()
    {
        return args.length;
    }

    public long getId()
    {
        return id;
    }

    @Override
    public String toString()
    {
        StringBuilder sb = new StringBuilder();
        sb.append("[Message ");
        sb.append(id);
        sb.append(" ");
        sb.append(name);
        sb.append(" ");
        for (String arg : args)
        {
            sb.append(arg);
            sb.append('\t');
        }
        sb.append("]");

        return sb.toString();
    }

    public String[] getArguments()
    {
        return args;
    }

    private static final ThreadLocal<StringBuilder> localStringBuilder = new ThreadLocal<StringBuilder>()
    {
        @Override
        protected StringBuilder initialValue()
        {
            return new StringBuilder(4096);
        }
    };

    public byte[] getBytes()
    {
        StringBuilder sb = localStringBuilder.get();

        try
        {
            sb.append(id);
            sb.append('\t');
            sb.append(name);
            for (String arg : args)
            {
                sb.append('\t');
                sb.append(arg);
            }

            sb.append('\n');

            return sb.toString().getBytes(CharsetUtil.UTF_8);
        } finally
        {
            sb.setLength(0);
        }
    }

    @Override
    public int hashCode()
    {
        int result = name != null ? name.hashCode() : 0;
        result = 31 * result + (args != null ? Arrays.hashCode(args) : 0);
        return result;
    }

    @Override
    public boolean equals(Object obj)
    {
        if (!(obj instanceof Message)) return false;

        Message o = (Message) obj;

        if (!this.name.equals(o.name)) return false;
        if (!Arrays.equals(this.args, o.args)) return false;

        return true;
    }
}
