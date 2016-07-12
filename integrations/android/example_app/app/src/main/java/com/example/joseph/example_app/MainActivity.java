package com.example.joseph.example_app;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import ZeroTier.SDK;

import java.net.InetSocketAddress;
import java.net.*;

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


        // Set up example proxy connection to SDK proxy server
        Log.d("ZTSDK-InJavaland", "Setting up connection to SDK proxy server");
        Socket s = new Socket();
        SocketAddress proxyAddr = new InetSocketAddress("0.0.0.0", 1337);
        Proxy proxy = new Proxy(Proxy.Type.SOCKS, proxyAddr);

    }
}