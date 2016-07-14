package com.example.joseph.example_app;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;

import ZeroTier.SDK;

import java.net.InetSocketAddress;
import java.net.*;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final SDK wrapper = new SDK();
        final String homeDir = "sdcard/zerotier";

        // Service thread
        new Thread(new Runnable() {
            public void run() {
                wrapper.startOneService(homeDir); // Calls to JNI code
            }
        }).start();

        // Wait for service before joining network
        Log.d("SDK-Javaland", "Waiting for service to start...\n");
        while(!wrapper.isRunning()) {
            Log.d("SDK-Javaland", "Waiting...\n");
        }
        Log.d("SDK-Javaland","Joining network...\n");
        wrapper.joinNetwork("565799d8f65063e5");

        // Set up example proxy connection to SDK proxy server
        Log.d("ZTSDK-InJavaland", "Setting up connection to SDK proxy server");
        Socket s = new Socket();
        SocketAddress proxyAddr = new InetSocketAddress("0.0.0.0", 1337);
        Proxy proxy = new Proxy(Proxy.Type.SOCKS, proxyAddr);
    }
}