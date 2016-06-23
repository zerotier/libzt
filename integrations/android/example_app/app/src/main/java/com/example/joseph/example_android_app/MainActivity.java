package com.example.joseph.example_android_app;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        new Thread(new Runnable() {
            public void run() {
                ZeroTierSDK wrapper = new ZeroTierSDK();
                wrapper.startOneService(); // Calls to JNI code
            }
        }).start();

    }
}
