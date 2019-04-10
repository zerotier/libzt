package com.example.exampleandroidapp;

// OS imports
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.os.Environment;
import android.Manifest;

// Misc imports
import java.util.ArrayList;
import java.util.List;

// ZeroTier imports
import com.zerotier.libzt.ZeroTier;
import com.zerotier.libzt.ZeroTierSocket;
import com.zerotier.libzt.ZeroTierSocketFactory;
import com.zerotier.libzt.ZeroTierSSLSocketFactory;
import com.zerotier.libzt.ZeroTierSocketAddress;
import com.zerotier.libzt.ZeroTierSocketOptionValue;
import com.zerotier.libzt.ZeroTierSocketImplFactory;
import com.zerotier.libzt.ZeroTierProtoStats;

// Custom ZeroTierEventListener
import com.example.exampleandroidapp.MyZeroTierEventListener;

public class MainActivity extends AppCompatActivity {

    static void sleep(int ms)
	{
		try { Thread.sleep(ms); } 
		catch (InterruptedException e) { e.printStackTrace(); }
    }
    
    void tests()
    {
        // Start ZeroTier service and wait for it to come online
        System.out.println("Starting ZeroTier...");
        MyZeroTierEventListener listener = new MyZeroTierEventListener();
        ZeroTier.start(getApplicationContext().getFilesDir() + "/zerotier3", listener, 9994);
        while (listener.isOnline == false) { sleep (50); }
        System.out.println("joining network...");
        ZeroTier.join(0xa09acf0233e4b070L);
        System.out.println("waiting for callback");
        while (listener.isNetworkReady == false) { sleep (50); }

        boolean testBackgroundWorkerGET = true;
        boolean testRestart = true;
        boolean testProtocolStats = true;

        if (testRestart) {
            for (int i=0; i<10; i++) {
                System.out.println("restarting...");
                ZeroTier.restart();
                sleep(10000);
            }
        }

        if (testProtocolStats) {
            ZeroTierProtoStats protocolSpecificStats = new ZeroTierProtoStats();
            int numPings = 0;
            System.out.println("recording stats...");
            while (true) {
                sleep(50);
                ZeroTier.get_protocol_stats(ZeroTier.STATS_PROTOCOL_ICMP, protocolSpecificStats);
                if (protocolSpecificStats.recv > numPings) {
                    numPings = protocolSpecificStats.recv;
                    System.out.println("icmp.recv="+numPings);
                }
            }
        }

        if (testBackgroundWorkerGET) {
            // Start worker threads (staggered by)
            List<Thread> threads = new ArrayList<>();
            for (int i = 0; i < 5; i++) {
                sleep(500);
                HTTPWorker thread = new HTTPWorker();
                thread.start();
                threads.add(thread);
            }
            try {
                Thread.sleep(60000000);
            } catch (Exception e) {
            }
            System.exit(0);
        }
    }
    
    // Entry point
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        tests();
    }
}