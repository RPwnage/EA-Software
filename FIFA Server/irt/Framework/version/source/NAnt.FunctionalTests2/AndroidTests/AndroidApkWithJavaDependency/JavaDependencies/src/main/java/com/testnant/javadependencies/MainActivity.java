package com.testnant.javadependencies;

import android.app.Activity;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;

import com.testnant.javalibrary.JavaLibrary;

public class MainActivity extends AppCompatActivity
{
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
		JavaLibrary.LibraryFunction();
        finish();
    }
}
