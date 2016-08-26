package com.example.joseph.example_app;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import java.net.InetSocketAddress;
import android.util.*;

import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Arrays;

// For SOCKS5 Proxy example
import java.net.Proxy;
import java.net.Socket;
import java.net.SocketAddress;

import ZeroTier.SDK;
import ZeroTier.ZTAddress;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        String nwid = "8056c2e21c000001";
        // Set up service
        final SDK zt = new SDK();
        final String homeDir = getApplicationContext().getFilesDir() + "/zerotier";
        new Thread(new Runnable() {
            public void run() {
                // Calls to JNI code
                zt.start_service(homeDir);
            }
        }).start();
        while(!zt.running()) { }

        // client/server mode toggle
        int mode = 4, err;

        // Listen to incoming connections
        if(mode==1)
        {
            zt.join_network(nwid);
            int sock = zt.socket(SDK.AF_INET, SDK.SOCK_STREAM, 0);

            if((err = zt.bind(sock, "0.0.0.0", 8080, nwid)) < 0)
                Log.d("ZTSDK", "bind_err = " + err + "\n");

            if((err = zt.listen(sock,1)) < 0)
                Log.d("ZTSDK", "listen_err = " + err);

            if((err = zt.accept(sock,null)) < 0)
                Log.d("ZTSDK", "accept_err = " + err);

            Log.d("ZTSDK", "Waiting to accept connection...");

            // ...
        }

        // TCP Echo ZTSDK (CLIENT)
        if(mode==2)
        {
            Log.d("ZTSDK", "\n\nStarting TCP Echo ZTSDK\n\n");
            zt.join_network(nwid);
            int sock = zt.socket(SDK.AF_INET, SDK.SOCK_STREAM, 0);
            String msg = "Welcome to the machine!";
            err = zt.connect(sock,  "28.206.65.211", 8099, nwid);

            Log.d("ZTSDK", "err = " + err + "\n");

            //return;

            // ECHO
            while(true)
            {
                // TX
                if((err = zt.write(sock, msg.getBytes(), msg.length())) > 0) {
                    Log.d("ZTSDK", "TX: " + msg + " --- " + err + " bytes");
                }

                // RX
                byte[] buffer = new byte[32];
                Arrays.fill(buffer, (byte)0);
                if((err = zt.read(sock, buffer, buffer.length)) > 0) {
                    String bufStr = new String(buffer).substring(0, err);
                    Log.d("ZTSDK", "RX: " + bufStr + " --- " + err + " bytes");
                }
            }

            // zt.stop_service();
        }

        // SOCKS5 Proxy ZTSDK
        if(mode==3)
        {
            zt.join_network(nwid);
            int sock = zt.socket(SDK.AF_INET, SDK.SOCK_STREAM, 0);

            int proxyPort = zt.get_proxy_port(nwid);
            Log.d("ZTSDK", "Setting up connection to SDK proxy server");
            SocketAddress proxyAddr = new InetSocketAddress("127.0.0.1", proxyPort);
            Proxy proxy = new Proxy(Proxy.Type.SOCKS, proxyAddr);
            Log.d("ZTSDK", "toString() = " + proxy.toString());
            final Socket s = new Socket(proxy);
            final SocketAddress remoteAddr = new InetSocketAddress("10.9.9.100", 8080);

            // Wait for address to be assigned
            ArrayList<String> addresses = zt.get_addresses(nwid);
            for(int i=0; i<addresses.size(); i++) {
                Log.d("ZTSDK", "Address = " + addresses.get(i));
            }
            while(addresses.size() > 0 && addresses.get(0).equals("-1.-1.-1.-1/-1")) {
                try {
                    Log.d("ZTSDK", "waiting for address");
                    Thread.sleep(100);
                } catch (java.lang.InterruptedException e) {
                }
                addresses = zt.get_addresses(nwid);
            }

            // Attempt connection
            new Thread(new Runnable() {
                public void run() {
                    try {
                        s.connect(remoteAddr, 1000);
                    }
                    catch(java.io.IOException e) {
                        Log.d("ZTSDK", "Unable to establish connection to SOCKS5 Proxy server\n");
                    }
                }
            }).start();
        }

        // UDP Echo ZTSDK
        if(mode==4)
        {
            // Remote server address (will be populated by recvfrom()
            ZTAddress remoteServer = new ZTAddress();
            ZTAddress bindAddr = new ZTAddress("0.0.0.0", 8080);

            Log.d("ZTSDK", "\n\nStarting UDP Echo ZTSDK\n\n");
            nwid = "8056c2e21c000001";
            zt.join_network(nwid);
            int sock = zt.socket(SDK.AF_INET, SDK.SOCK_DGRAM, 0);

            Log.d("ZTSDK", "binding...");
            if((err = zt.bind(sock, bindAddr, nwid)) < 0)
                Log.d("ZTSDK", "bind_err = " + err + "\n");
            if((err = zt.listen(sock, 0)) < 0)
                Log.d("ZTSDK", "listen_err = " + err);
            ArrayList<String> addresses = zt.get_addresses(nwid);
            if(addresses.size() < 0) {
                Log.d("ZTSDK", "unable to obtain ZT address");
                return;
            }
            else {
                Log.d("ZTSDK", "App IP = " + addresses.get(0));
            }

            String bufStr;
            byte[] buffer = new byte[1024];

            zt.fcntl(sock, zt.F_SETFL, zt.O_NONBLOCK);

            // ECHO
            while(true) {

                // RX
                if((err = zt.recvfrom(sock, buffer, 32, 0, remoteServer)) > 0) {
                    bufStr = new String(buffer).substring(0, err);
                    Log.d("ZTSDK", "read (" + err + ") bytes from " + remoteServer.Address() + " : " + remoteServer.Port() + ", msg = " + bufStr);

                    // TX
                    String msg = "Welcome response from android\n";
                    err = zt.sendto(sock, msg.getBytes(), msg.length(), 0, remoteServer);
                    if (err < 0)
                        Log.d("ZTSDK", "sendto_err = " + err);
                }
            }
            //Log.d("ZTSDK", "leaving network");
            //zt.leave_network(nwid);
        }
    }
}