package com.esn.sonar.core;

import java.util.HashMap;

public class CommandHandler
{
    private final HashMap<String, CommandCallback> commandMap = new HashMap<String, CommandCallback>();

    public boolean call(Object instance, Message msg) throws Exception
    {
        CommandCallback cb = commandMap.get(msg.getName());

        if (cb == null)
            return false;

        cb.call(instance, msg);
        return true;
    }

    public void registerCallback(String name, CommandCallback callback)
    {
        commandMap.put(name, callback);
    }
}
