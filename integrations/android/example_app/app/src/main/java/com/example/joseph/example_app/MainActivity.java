package com.example.joseph.example_app;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import java.net.InetSocketAddress;
import java.net.*;
import android.content.Context;
import android.os.Handler;

import ZeroTier.SDK;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final SDK zt = new SDK();
        final String homeDir = getApplicationContext().getFilesDir() + "/zerotier";

        // Service thread
        new Thread(new Runnable() {
            public void run() {
                // Calls to JNI code
                zt.startOneService(homeDir);
            }
        }).start();

        Log.d("SDK", "Starting service...\n");
        while(!zt.isRunning()) { }
        Log.d("SDK","Joining network...\n");
        zt.joinNetwork("XXXXXXXXXXXXXXXX");

        // Create ZeroTier socket
        Log.d("","ztjniSocket()\n");
        int sock = zt.ztjniSocket(zt.AF_INET, zt.SOCK_STREAM, 0);
        Log.d("", "ztjniSocket() = " + sock + "\n");

        try {
            Thread.sleep(10000);
        }
        catch(java.lang.InterruptedException e) { }

        // Connect to remote host
        Log.d("","ztjniConnect()\n");
        int err = zt.ztjniConnect(sock, "10.9.9.203", 8080);

        // Set up example proxy connection to SDK proxy server
        /*
        Log.d("ZTSDK-InJavaland", "Setting up connection to SDK proxy server");
        Socket s = new Socket();
        SocketAddress proxyAddr = new InetSocketAddress("0.0.0.0", 1337);
        Proxy proxy = new Proxy(Proxy.Type.SOCKS, proxyAddr);
        */
    }
}