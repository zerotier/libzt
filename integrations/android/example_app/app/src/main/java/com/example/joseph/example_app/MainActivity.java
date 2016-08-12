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
        int mode = 4, err = -1;

        // Establish outgoing connection
        if(mode==0)
        {
            zt.join_network(nwid);
            int sock = zt.socket(SDK.AF_INET, SDK.SOCK_STREAM, 0);

            err = zt.connect(sock, "10.9.9.100", 7004, nwid);
            Log.d("TEST", "err = " + err + "\n");

            // TX
            zt.write(sock, "Welcome to the machine".getBytes(), 16);

            // Test section
            for(int i=0; i<1000; i++)
            {
                try {
                    Thread.sleep(20);
                }
                catch(java.lang.InterruptedException e) { }

                String msg = "Welcome to the machine!";
                int written = zt.write(sock, msg.getBytes(), msg.length());
                Log.d("TEST", "TX[" + i + "] = " + written);

                // RX
                byte[] buffer = new byte[1024];
                zt.read(sock, buffer, buffer.length);
                String bufStr = new String(buffer);
                Log.d("TEST", "RX[" + i + "] = " + bufStr);
            }
        }

        // Listen to incoming connections
        if(mode==1)
        {
            zt.join_network(nwid);
            int sock = zt.socket(SDK.AF_INET, SDK.SOCK_STREAM, 0);

            err = zt.bind(sock, "0.0.0.0", 8080, nwid);
            Log.d("TEST", "bind_err = " + err + "\n");

            err = zt.listen(sock,1);
            Log.d("TEST", "listen_err = " + err);

            err = zt.accept(sock,null); // Pass a ZTAddress to get remote host's address (if you want)
            Log.d("TEST", "accept_err = " + err);
        }

        if(mode==2)
        {
            zt.join_network(nwid);
            int sock = zt.socket(SDK.AF_INET, SDK.SOCK_STREAM, 0);

            Log.d("TEST", "RX / TX stress test\n");
            err = zt.connect(sock, "10.9.9.100", 8080, nwid);
            Log.d("TEST", "err = " + err + "\n");

            // TX
            zt.write(sock, "Welcome to the machine".getBytes(), 16);

            // Test section
            for(int i=0; i<1000; i++)
            {
                try {
                    Thread.sleep(0);
                }
                catch(java.lang.InterruptedException e) { }

                String msg = "Welcome to the machine!";
                int written = zt.write(sock, msg.getBytes(), msg.length());
                Log.d("TEST", "TX[" + i + "] = " + written);

                // RX
                byte[] buffer = new byte[32];
                Arrays.fill(buffer, (byte)0);
                zt.read(sock, buffer, buffer.length);
                String bufStr = new String(buffer);
                Log.d("TEST", "RX[" + i + "] = " + bufStr);
            }
        }

        // SOCKS5 Proxy test
        if(mode==3)
        {
            zt.join_network(nwid);
            int sock = zt.socket(SDK.AF_INET, SDK.SOCK_STREAM, 0);

            int proxyPort = zt.get_proxy_port(nwid);
            Log.d("ZTSDK", "Setting up connection to SDK proxy server");
            SocketAddress proxyAddr = new InetSocketAddress("127.0.0.1", proxyPort);
            Proxy proxy = new Proxy(Proxy.Type.SOCKS, proxyAddr);
            Log.d("TEST", "toString() = " + proxy.toString());
            final Socket s = new Socket(proxy);
            final SocketAddress remoteAddr = new InetSocketAddress("10.9.9.100", 8080);

            // Wait for address to be assigned
            ArrayList<String> addresses = new ArrayList<String>();
            addresses = zt.get_addresses(nwid);
            for(int i=0; i<addresses.size(); i++) {
                Log.d("TEST", "Address = " + addresses.get(i));
            }
            while(addresses.size() > 0 && addresses.get(0).equals("-1.-1.-1.-1/-1")) {
                try {
                    Log.d("TEST", "waiting for address");
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
                        Log.d("TEST", "Unable to establish connection to SOCKS5 Proxy server\n");
                    }
                }
            }).start();
        }

        // UDP test
        if(mode==4)
        {
            // Remote server address (will be populated by recvfrom()
            ZTAddress remoteServer = new ZTAddress();
            ZTAddress bindAddr = new ZTAddress("0.0.0.0", 8080);

            Log.d("TEST", "\n\nStarting UDP Test\n\n");
            nwid = "8056c2e21c000001";
            zt.join_network(nwid);
            int sock = zt.socket(SDK.AF_INET, SDK.SOCK_DGRAM, 0);

            Log.d("TEST", "binding...");
            err = zt.bind(sock, bindAddr, nwid);
            if (err < 0)
                Log.d("TEST", "bind_err = " + err + "\n");

            err = zt.listen(sock, 0);
            if(err < 0)
                Log.d("TEST", "listen_err = " + err);

            while(err >= 0) {
                // RX
                byte[] buffer = new byte[32];
                String addr_string = "-1.-1.-1.-1";
                int port_no = -1;

                err = zt.recvfrom(sock, buffer, 32, 0, remoteServer);
                String bufStr = new String(buffer).substring(0, err);
                Log.d("TEST", "read (" + err + ") bytes from " + remoteServer.Address() + " : " + remoteServer.Port() + ", msg = " + bufStr);

                // TX

                if(err > 0) {
                    String msg = "Welcome response from android";
                    err = zt.sendto(sock, msg.getBytes(), msg.length(), 0, remoteServer);
                    if (err < 0)
                        Log.d("TEST", "sendto_err = " + err);
                }

            }
            Log.d("TEST", "leaving network");
            zt.leave_network(nwid);
        }
    }
}