package com.example.exampleandroidapp;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import com.zerotier.libzt.ZeroTier;
import com.zerotier.libzt.ZTSocketAddress;
import com.zerotier.libzt.ZTFDSet;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("zt-shared");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final ZeroTier zt = new ZeroTier();

        new Thread(new Runnable() {
            public void run() {
                final String path = getApplicationContext().getFilesDir() + "/zerotier";
                long nwid = 0xe42a7f55c2b9be61L;

                // Test modes
                boolean blocking_start_call = true;
                boolean client_mode = true;
                boolean tcp = false;
                boolean loop = true;

                int fd = -1, client_fd = -1, err, r, w, length = 0, flags = 0;
                byte[] rxBuffer;
                byte[] txBuffer = "welcome to the machine".getBytes();
                String remoteAddrStr = "172.28.221.116";
                String localAddrStr = "1.2.3.4";
                int portNo = 4040;

                ZTSocketAddress remoteAddr, localAddr;
                ZTSocketAddress sockname = new ZTSocketAddress();
                ZTSocketAddress addr = new ZTSocketAddress();

                // METHOD 1 (easy)
                // Blocking call that waits for all components of the service to start
                System.out.println("Starting ZT service...");
                if (blocking_start_call) {
                    zt.startjoin(path, nwid);
                }
                System.out.println("ZT service ready.");

                // Device/Node address info
                System.out.println("path=" + zt.get_path());
                long nodeId = zt.get_node_id();
                System.out.println("nodeId=" + Long.toHexString(nodeId));
                int numAddresses = zt.get_num_assigned_addresses(nwid);
                System.out.println("this node has (" + numAddresses + ") assigned addresses on network " + Long.toHexString(nwid));
                for (int i=0; i<numAddresses; i++) {
                    zt.get_address_at_index(nwid, i, sockname);
                    //System.out.println("address[" + i + "] = " + sockname.toString()); // ip:port
                    System.out.println("address[" + i + "] = " + sockname.toCIDR());
                }

                zt.get_6plane_addr(nwid, nodeId, sockname);
                System.out.println("6PLANE address = " + sockname.toCIDR());

                System.out.println("mode:tcp");
                if ((fd = zt.socket(zt.AF_INET, zt.SOCK_STREAM, 0)) < 0) {
                    System.out.println("error creating socket");
                    return;
                }
                // CLIENT
                int lengthToRead;
                if (client_mode) {
                    remoteAddr = new ZTSocketAddress(remoteAddrStr, portNo);
                    if ((err = zt.connect(fd, remoteAddr)) < 0) {
                        System.out.println("error connecting (err=" + err + ")");
                        return;
                    }

                    int n = 20000;
                    String in = "echo!";
                    String echo_msg = "";
                    for (int i = 0; i < n; i += 1) {
                        echo_msg += in;
                    }

                    w = zt.write(fd, echo_msg.getBytes());
                    //rxBuffer = new byte[100];
                    //lengthToRead = 100;
                    System.out.println("w="+w);
                    //System.out.println("reading bytes...");
                    //r = zt.read(fd, rxBuffer, lengthToRead);
                    //System.out.println("r="+r);
                    //System.out.println("string="+new String(rxBuffer));
                }
            }
        }
        ).start();
    }
}
