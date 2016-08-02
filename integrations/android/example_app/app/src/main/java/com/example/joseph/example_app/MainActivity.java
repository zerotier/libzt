package com.example.joseph.example_app;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import java.net.InetSocketAddress;
import android.util.*;

import ZeroTier.SDK;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Set up service
        final SDK zt = new SDK();
        final String homeDir = getApplicationContext().getFilesDir() + "/zerotier";
        new Thread(new Runnable() {
            public void run() {
                // Calls to JNI code
                zt.zt_start_service(homeDir);
            }
        }).start();
        while(!zt.zt_running()) { }
        zt.zt_join_network("XXXXXXXXXXXXXXXX");

        // Create ZeroTier socket
        int sock = zt.zt_socket(SDK.AF_INET, SDK.SOCK_STREAM, 0);

        /*
        try {
            Thread.sleep(25000);
        }
        catch(java.lang.InterruptedException e) { }
        */

        int mode = 0; // client/server mode toggle

        // Establish outgoing connection
        if(mode==0)
        {
            int err = -1;
            while(err < 0) {

                try {
                    Thread.sleep(1000);
                }
                catch(java.lang.InterruptedException e) { }
                err = zt.zt_connect(sock, "10.9.9.100", 7003);
                Log.d("TEST", "err = " + err + "\n");
            }

            // TX
            zt.zt_write(sock, "Welcome to the machine".getBytes(), 16);

            // RX
            byte[] buffer = new byte[12];
            zt.zt_read(sock, buffer, 12);
            String bufStr = new String(buffer);
            Log.d("TEST", "response = " + bufStr);
        }

        // Listen to incoming connections
        if(mode==1)
        {
            int err;
            zt.zt_bind(sock, "0.0.0.0", 8081);
            zt.zt_listen(sock,1);
            err = zt.zt_accept(sock,null); // Pass a ZTAddress to get remote host's address (if you want)

            // Example ZTAddress usage
            /*
            ZeroTier.ZTAddress za = new ZeroTier.ZTAddress();
            za.addr = "0.0.0.0";
            za.port = -1;
            InetSocketAddress addr = za.ToInetSocketAddress();
            */
        }


        // Set up example proxy connection to SDK proxy server
        /*
        Log.d("ZTSDK", "Setting up connection to SDK proxy server");
        Socket s = new Socket();
        SocketAddress proxyAddr = new InetSocketAddress("0.0.0.0", 1337);
        Proxy proxy = new Proxy(Proxy.Type.SOCKS, proxyAddr);
        */
    }
}