package com.example.joseph.example_app;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import ZeroTier.SDK;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        new Thread(new Runnable() {
            public void run() {
                SDK wrapper = new SDK();
                wrapper.startOneService(); // Calls to JNI code
            }
        }).start();
    }
}