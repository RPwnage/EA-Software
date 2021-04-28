package com.ea.AarTopLibrary;

import com.ea.AarBottomLibrary.AarBottomLibrary;

public class AarTopLibrary
{
    public AarTopLibrary()
    {
		FunctionThatDoesImportantThings(7);
    }

    public int FunctionThatDoesImportantThings(int x)
    {
		return AarBottomLibrary.FunctionThatDoesImportantThings(x);
    }
}