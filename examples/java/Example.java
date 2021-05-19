import com.zerotier.sockets.*;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.math.BigInteger;
import java.net.DatagramPacket;
import java.net.InetAddress;

public class selftest {
    public static void main(String[] args)
    {
        if (args.length < 4 || args.length > 5) {
            System.err.println("Invalid arguments");
            System.err.println(" Usage: <server|client> <id_path> <network> <addr> <port>");
            System.err.println("  example server ./ 0123456789abcdef 8080");
            System.err.println("  example client ./ 0123456789abcdef 192.168.22.1 8080\n");
            System.exit(1);
        }

        String storagePath = "";
        String remoteAddr = "";
        int port = 0;
        String mode = args[0];
        storagePath = args[1];
        BigInteger nwid = new BigInteger(args[2], 16);
        Long networkId = nwid.longValue();

        if (args.length == 4) {
            port = Integer.parseInt(args[3]);
        }
        if (args.length == 5) {
            remoteAddr = args[3];
            port = Integer.parseInt(args[4]);
        }
        System.out.println("mode        = " + mode);
        System.out.println("networkId   = " + Long.toHexString(networkId));
        System.out.println("storagePath = " + storagePath);
        System.out.println("remoteAddr  = " + remoteAddr);
        System.out.println("port        = " + port);

        // ZeroTier setup

        ZeroTierNode node = new ZeroTierNode();
        node.initFromStorage(storagePath);
        node.initSetEventHandler(new MyZeroTierEventListener());
        // node.initSetPort(9994);
        node.start();

        System.out.println("Waiting for node to come online...");
        while (! node.isOnline()) {
            ZeroTierNative.zts_util_delay(50);
        }
        System.out.println("Node ID: " + Long.toHexString(node.getId()));
        System.out.println("Joining network...");
        node.join(networkId);
        System.out.println("Waiting for network...");
        while (! node.isNetworkTransportReady(networkId)) {
            ZeroTierNative.zts_util_delay(50);
        }
        System.out.println("Joined");

        // IPv4

        InetAddress addr4 = node.getIPv4Address(networkId);
        System.out.println("IPv4 address = " + addr4.getHostAddress());

        // IPv6

        InetAddress addr6 = node.getIPv6Address(networkId);
        System.out.println("IPv6 address = " + addr6.getHostAddress());

        // MAC address

        System.out.println("MAC address = " + node.getMACAddress(networkId));

        // Socket logic

        if (mode.equals("server")) {
            System.out.println("Starting server...");
            try {
                ZeroTierServerSocket listener = new ZeroTierServerSocket(port);
                ZeroTierSocket conn = listener.accept();
                ZeroTierInputStream inputStream = conn.getInputStream();
                DataInputStream dataInputStream = new DataInputStream(inputStream);
                String message = dataInputStream.readUTF();
                System.out.println("recv: " + message);
                listener.close();
                conn.close();
            }
            catch (Exception ex) {
                System.out.println(ex);
            }
        }

        if (mode.equals("client")) {
            System.out.println("Starting client...");
            try {
                ZeroTierSocket socket = new ZeroTierSocket(remoteAddr, port);
                ZeroTierOutputStream outputStream = socket.getOutputStream();
                DataOutputStream dataOutputStream = new DataOutputStream(outputStream);
                dataOutputStream.writeUTF("Hello from java!");
                socket.close();
            }
            catch (Exception ex) {
                System.out.println(ex);
            }
        }
    }
}

/**
 * (OPTIONAL) event handler
 */
class MyZeroTierEventListener implements ZeroTierEventListener {
    public void onZeroTierEvent(long id, int eventCode)
    {
        if (eventCode == ZeroTierNative.ZTS_EVENT_NODE_UP) {
            System.out.println("EVENT_NODE_UP");
        }
        if (eventCode == ZeroTierNative.ZTS_EVENT_NODE_ONLINE) {
            System.out.println("EVENT_NODE_ONLINE: " + Long.toHexString(id));
        }
        if (eventCode == ZeroTierNative.ZTS_EVENT_NODE_OFFLINE) {
            System.out.println("EVENT_NODE_OFFLINE");
        }
        if (eventCode == ZeroTierNative.ZTS_EVENT_NODE_DOWN) {
            System.out.println("EVENT_NODE_DOWN");
        }
        if (eventCode == ZeroTierNative.ZTS_EVENT_NETWORK_READY_IP4) {
            System.out.println("ZTS_EVENT_NETWORK_READY_IP4: " + Long.toHexString(id));
        }
        if (eventCode == ZeroTierNative.ZTS_EVENT_NETWORK_READY_IP6) {
            System.out.println("ZTS_EVENT_NETWORK_READY_IP6: " + Long.toHexString(id));
        }
        if (eventCode == ZeroTierNative.ZTS_EVENT_NETWORK_DOWN) {
            System.out.println("EVENT_NETWORK_DOWN: " + Long.toHexString(id));
        }
        if (eventCode == ZeroTierNative.ZTS_EVENT_NETWORK_OK) {
            System.out.println("EVENT_NETWORK_OK: " + Long.toHexString(id));
        }
        if (eventCode == ZeroTierNative.ZTS_EVENT_NETWORK_ACCESS_DENIED) {
            System.out.println("EVENT_NETWORK_ACCESS_DENIED: " + Long.toHexString(id));
        }
        if (eventCode == ZeroTierNative.ZTS_EVENT_NETWORK_NOT_FOUND) {
            System.out.println("EVENT_NETWORK_NOT_FOUND: " + Long.toHexString(id));
        }
    }
}
