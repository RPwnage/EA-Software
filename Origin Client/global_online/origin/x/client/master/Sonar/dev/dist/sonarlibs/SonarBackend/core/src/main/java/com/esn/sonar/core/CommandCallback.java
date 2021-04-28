package com.esn.sonar.core;

public interface CommandCallback
{
    public void call(Object instance, Message msg) throws Exception;
}
