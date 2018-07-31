package com.example.zerotier;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.TextView;

import com.zerotier.libzt.ZeroTier;
import com.zerotier.libzt.ZTSocketAddress;
import com.zerotier.libzt.ZTFDSet;

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
                long nwid = 0xac9afb023544b071L;

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
                System.out.println("ZT service ready.");

                // Device/Node address info
                System.out.println("path=" + libzt.get_path());
                long nodeId = libzt.get_node_id();
                System.out.println("nodeId=" + Long.toHexString(nodeId));
                int numAddresses = libzt.get_num_assigned_addresses(nwid);
                System.out.println("this node has (" + numAddresses + ") assigned addresses on network " + Long.toHexString(nwid));
                for (int i=0; i<numAddresses; i++) {
                    libzt.get_address_at_index(nwid, i, sockname);
                    //System.out.println("address[" + i + "] = " + sockname.toString()); // ip:port
                    System.out.println("address[" + i + "] = " + sockname.toCIDR());
                }

                libzt.get_6plane_addr(nwid, nodeId, sockname);
                System.out.println("6PLANE address = " + sockname.toCIDR());


            }
        }).start();
    }
}
