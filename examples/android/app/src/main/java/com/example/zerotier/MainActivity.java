package com.example.zerotier;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.TextView;

import zerotier.*;
import java.net.*;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("zt");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);


        final ZeroTier libzt = new ZeroTier();

        new Thread(new Runnable() {
            public void run() {
                final String path = getApplicationContext().getFilesDir() + "/zerotier";
                long nwid = 0xac9afb026544b071L;

                // Test modes
                boolean blocking_start_call = true;
                boolean client_mode = false;
                boolean tcp = false;
                boolean loop = true;

                int fd = -1, client_fd = -1, err, r, w, length = 0, flags = 0;
                byte[] rxBuffer;
                byte[] txBuffer = "welcome to the machine".getBytes();
                String remoteAddrStr = "11.7.7.224";
                String localAddrStr = "1.2.3.4";
                int portNo = 4040;

                ZTSocketAddress remoteAddr, localAddr;
                ZTSocketAddress sockname = new ZTSocketAddress();
                ZTSocketAddress addr = new ZTSocketAddress();

                // METHOD 1 (easy)
                // Blocking call that waits for all components of the service to start
                System.out.println("Starting ZT service...");
                if (blocking_start_call) {
                    libzt.startjoin(path, nwid);
                }
                System.out.println("Complete");
            }
        }).start();
    }
}
