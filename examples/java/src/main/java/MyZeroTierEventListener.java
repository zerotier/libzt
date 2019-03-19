import com.zerotier.libzt.ZeroTier;
import com.zerotier.libzt.ZeroTierEventListener;
import com.zerotier.libzt.ZeroTierPeerDetails;

public class MyZeroTierEventListener implements ZeroTierEventListener {

    public boolean isNetworkReady = false;
    public boolean isOnline = false;

    public void onZeroTierEvent(long id, int eventCode)
    {
        if (eventCode == ZeroTier.EVENT_NODE_UP) {
            System.out.println("EVENT_NODE_UP: nodeId=" + Long.toHexString(id));
        }
        if (eventCode == ZeroTier.EVENT_NODE_ONLINE) {
            // The core service is running properly and can join networks now
            System.out.println("EVENT_NODE_ONLINE: nodeId=" + Long.toHexString(ZeroTier.get_node_id()));
            isOnline = true;
        }
        if (eventCode == ZeroTier.EVENT_NODE_OFFLINE) {
            // Network does not seem to be reachable by any available strategy
            System.out.println("EVENT_NODE_OFFLINE");
        }
        if (eventCode == ZeroTier.EVENT_NODE_DOWN) {
            // Called when the node is shutting down
            System.out.println("EVENT_NODE_DOWN");
        }
        if (eventCode == ZeroTier.EVENT_NODE_IDENTITY_COLLISION) {
            // Another node with this identity already exists
            System.out.println("EVENT_NODE_IDENTITY_COLLISION");
        }
        if (eventCode == ZeroTier.EVENT_NODE_UNRECOVERABLE_ERROR) {
            // Try again
            System.out.println("EVENT_NODE_UNRECOVERABLE_ERROR");
        }
        if (eventCode == ZeroTier.EVENT_NODE_NORMAL_TERMINATION) {
            // Normal closure
            System.out.println("EVENT_NODE_NORMAL_TERMINATION");
        }
        if (eventCode == ZeroTier.EVENT_NETWORK_READY_IP4) {
            // We have at least one assigned address and we've received a network configuration
            System.out.println("ZTS_EVENT_NETWORK_READY_IP4: nwid=" + Long.toHexString(id));
            if ( id == 0xa09acf0233e4b070L) {
                isNetworkReady = true;
            }
        }
        if (eventCode == ZeroTier.EVENT_NETWORK_READY_IP6) {
            // We have at least one assigned address and we've received a network configuration
            System.out.println("ZTS_EVENT_NETWORK_READY_IP6: nwid=" + Long.toHexString(id));
            //isNetworkReady = true;
        }
        if (eventCode == ZeroTier.EVENT_NETWORK_DOWN) {
            // Someone called leave(), we have no assigned addresses, or otherwise cannot use this interface
            System.out.println("EVENT_NETWORK_DOWN: nwid=" + Long.toHexString(id));
        }
        if (eventCode == ZeroTier.EVENT_NETWORK_REQUESTING_CONFIG) {
            // Waiting for network configuration
            System.out.println("EVENT_NETWORK_REQUESTING_CONFIG: nwid=" + Long.toHexString(id));
        }
        if (eventCode == ZeroTier.EVENT_NETWORK_OK) {
            // Config received and this node is authorized for this network
            System.out.println("EVENT_NETWORK_OK: nwid=" + Long.toHexString(id));
        }
        if (eventCode == ZeroTier.EVENT_NETWORK_ACCESS_DENIED) {
            // You are not authorized to join this network
            System.out.println("EVENT_NETWORK_ACCESS_DENIED: nwid=" + Long.toHexString(id));
        }
        if (eventCode == ZeroTier.EVENT_NETWORK_NOT_FOUND) {
            // The virtual network does not exist
            System.out.println("EVENT_NETWORK_NOT_FOUND: nwid=" + Long.toHexString(id));
        }
        if (eventCode == ZeroTier.EVENT_NETWORK_CLIENT_TOO_OLD) {
            // The core version is too old
            System.out.println("EVENT_NETWORK_CLIENT_TOO_OLD: nwid=" + Long.toHexString(id));
        }
        if (eventCode == ZeroTier.EVENT_PEER_P2P) {
            System.out.println("EVENT_PEER_P2P: id=" + Long.toHexString(id));
/*
            ZeroTierPeerDetails details = new ZeroTierPeerDetails();
            ZeroTier.get_peer(id, details);
            System.out.println("address="+Long.toHexString(details.address));
            System.out.println("pathCount="+details.pathCount);
            System.out.println("version="+details.versionMajor+"."+details.versionMinor+"."+details.versionRev);
            System.out.println("latency="+details.latency); // Not relevant
            System.out.println("role="+details.role); // Not relevent
            // Print all known paths
            for (int i=0; i<details.pathCount; i++) {
                System.out.println("addr="+details.paths[i].toString());
            }
*/
        }
        if (eventCode == ZeroTier.EVENT_PEER_RELAY) {
            System.out.println("EVENT_PEER_RELAY: id=" + Long.toHexString(id));
        }
    }
}
