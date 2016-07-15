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
        zt.joinNetwork("565799d8f65063e5");

/*
        final Handler handler = new Handler();
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                //Do something after 100ms
            }
        }, 10000);
*/

        try
        {
            Thread.sleep(1000);
        }
        catch(java.lang.InterruptedException e)
        {

        }


        // Create ZeroTier socket
        Log.d("","ztjniSocket()\n");
        int sock = zt.ztjniSocket(zt.AF_INET, zt.SOCK_STREAM, 0);
        Log.d("", "ztjniSocket() = " + sock + "\n");


        // Construct remote host address
        //InetAddress addr = InetAddress.getByName("10.144.211.245");
        //int port = 8080;
        //SocketAddress sockaddr = new InetSocketAddress(addr, port);

        // Connect to remote host
        //Log.d("","ztjniConnect()\n");
        //int err = zt.ztjniConnect(sock, "10.144.211.245", 8080);
        //Log.d("", "ztjniConnect() = " + err + "\n");

        // Set up example proxy connection to SDK proxy server
        /*
        Log.d("ZTSDK-InJavaland", "Setting up connection to SDK proxy server");
        Socket s = new Socket();
        SocketAddress proxyAddr = new InetSocketAddress("0.0.0.0", 1337);
        Proxy proxy = new Proxy(Proxy.Type.SOCKS, proxyAddr);
        */
    }
}