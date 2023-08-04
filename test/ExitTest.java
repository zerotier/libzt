import com.zerotier.sockets.*;

//
// Test for:
// Java process does not exit after finishing libzt work
// https://github.com/zerotier/libzt/issues/242
//
// Run and ensure that process exits
//
public class ExitTest {

    public static void main(String[] args) throws InterruptedException {

        long networkId = Long.parseUnsignedLong("ebe7fbd445e76ac6", 16);
        String storagePath = "exittest_storage";

        ZeroTierNode node = new ZeroTierNode();
        node.initFromStorage(storagePath);
        node.initSetEventHandler(new ZeroTierEventListener() {
            @Override
            public void onZeroTierEvent(long l, int eventCode) {
                if (eventCode == ZeroTierNative.ZTS_EVENT_NODE_UP) {
                    System.err.println("ZTS_EVENT_NODE_UP");
                }
                if (eventCode == ZeroTierNative.ZTS_EVENT_NODE_ONLINE) {
                    System.err.println("ZTS_EVENT_NODE_ONLINE");
                }
                if (eventCode == ZeroTierNative.ZTS_EVENT_NODE_OFFLINE) {
                    System.err.println("ZTS_EVENT_NODE_OFFLINE");
                }
                if (eventCode == ZeroTierNative.ZTS_EVENT_NODE_DOWN) {
                    System.err.println("ZTS_EVENT_NODE_DOWN");
                }
                if (eventCode == ZeroTierNative.ZTS_EVENT_NETWORK_READY_IP4) {
                    System.err.println("ZTS_EVENT_NETWORK_READY_IP4");
                }
                if (eventCode == ZeroTierNative.ZTS_EVENT_NETWORK_READY_IP6) {
                    System.err.println("ZTS_EVENT_NETWORK_READY_IP6");
                }
                if (eventCode == ZeroTierNative.ZTS_EVENT_NETWORK_DOWN) {
                    System.err.println("ZTS_EVENT_NETWORK_DOWN");
                }
                if (eventCode == ZeroTierNative.ZTS_EVENT_NETWORK_OK) {
                    System.err.println("ZTS_EVENT_NETWORK_OK");
                }
                if (eventCode == ZeroTierNative.ZTS_EVENT_NETWORK_ACCESS_DENIED) {
                    System.err.println("ZTS_EVENT_NETWORK_ACCESS_DENIED");
                }
                if (eventCode == ZeroTierNative.ZTS_EVENT_NETWORK_NOT_FOUND) {
                    System.err.println("ZTS_EVENT_NETWORK_NOT_FOUND");
                }
            }
        });

        System.err.println("start");
        node.start();

        System.err.println("delay until online");
        while (!node.isOnline()) {
            ZeroTierNative.zts_util_delay(50);
        }
        System.err.println("done delaying");

        System.out.println("join");
        node.join(networkId);

        System.out.println("delaying until transport ready");
        while (! node.isNetworkTransportReady(networkId)) {
            ZeroTierNative.zts_util_delay(50);
        }

        System.err.println("stop");
        node.stop();

        System.err.println("exiting");
    }
}
