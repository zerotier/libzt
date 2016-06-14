package com.zerotier.one.service;

import android.content.Context;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.VpnService;
import android.content.Intent;
import android.os.ParcelFileDescriptor;
import android.preference.PreferenceManager;
import android.util.Log;
import android.widget.Toast;

import com.zerotier.one.events.ErrorEvent;
import com.zerotier.one.events.JoinNetworkEvent;
import com.zerotier.one.events.LeaveNetworkEvent;
import com.zerotier.one.events.ManualDisconnectEvent;
import com.zerotier.one.events.NetworkInfoReplyEvent;
import com.zerotier.one.events.NetworkListReplyEvent;
import com.zerotier.one.events.NetworkReconfigureEvent;
import com.zerotier.one.events.NetworkRemovedEvent;
import com.zerotier.one.events.NodeDestroyedEvent;
import com.zerotier.one.events.NodeIDEvent;
import com.zerotier.one.events.NodeStatusEvent;
import com.zerotier.one.events.RequestNetworkInfoEvent;
import com.zerotier.one.events.RequestNetworkListEvent;
import com.zerotier.one.events.RequestNodeStatusEvent;
import com.zerotier.one.events.StopEvent;
import com.zerotier.one.util.InetAddressUtils;
import com.zerotier.one.util.NetworkIdUtils;
import com.zerotier.sdk.Event;
import com.zerotier.sdk.EventListener;
import com.zerotier.sdk.Node;
import com.zerotier.sdk.NodeException;
import com.zerotier.sdk.NodeStatus;
import com.zerotier.sdk.ResultCode;
import com.zerotier.sdk.VirtualNetworkConfig;
import com.zerotier.sdk.VirtualNetworkConfigListener;
import com.zerotier.sdk.VirtualNetworkConfigOperation;


import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.net.DatagramSocket;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.util.HashMap;

import de.greenrobot.event.EventBus;

public class ZeroTierOneService extends VpnService implements Runnable, EventListener, VirtualNetworkConfigListener {

    private static final String TAG = "ZT1_Service";

    public static final String ZT1_NETWORK_ID = "com.zerotier.one.network_id";

    public static final int MSG_JOIN_NETWORK = 1;
    public static final int MSG_LEAVE_NETWORK = 2;

    private Thread vpnThread;
    private UdpCom udpCom;
    private Thread udpThread;

    private TunTapAdapter tunTapAdapter;

    private Node node;
    private DataStore dataStore;

    DatagramSocket svrSocket;
    ParcelFileDescriptor vpnSocket;
    FileInputStream in;
    FileOutputStream out;

    private final HashMap<Long, VirtualNetworkConfig> networkConfigs;

    private EventBus eventBus = EventBus.getDefault();

    private long nextBackgroundTaskDeadline = 0;
    private long lastMulticastGroupCheck = 0;

    protected void setNextBackgroundTaskDeadline(long deadline) {
        synchronized (this) {
            nextBackgroundTaskDeadline = deadline;
        }
    }

    public ZeroTierOneService() {
        super();
        dataStore = new DataStore(this);
        networkConfigs = new HashMap<>();
        eventBus.register(this);

        Netcon.NetconWrapper wrapper = new Netcon.NetconWrapper();


        wrapper.startOneService();
/*
        if(wrapper.loadsymbols() == 4)
        {
            Log.e(TAG,"loadsymbols(): Symbol found");
            //Toast t = Toast.makeText(this, "WORKED", Toast.LENGTH_SHORT);
        }
        else {
            Log.e(TAG,"loadsymbols(): Symbol NOT found");
            //Toast t = Toast.makeText(this, "DIDNT WORK", Toast.LENGTH_SHORT);
        }
*/
    }



    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {




        return START_STICKY;
    }

    public void stopZeroTier() {
        if(udpThread != null && udpThread.isAlive()) {
            udpThread.interrupt();
            udpThread = null;
        }

        if(vpnThread != null && vpnThread.isAlive()) {
            vpnThread.interrupt();
            vpnThread = null;
        }

        if(svrSocket != null) {
            svrSocket.close();
            svrSocket = null;
        }

        if(node != null) {
            eventBus.post(new NodeDestroyedEvent());
            node.close();
            node = null;
        }
    }

    @Override
    public void onDestroy() {
        stopZeroTier();
        super.onDestroy();
    }

    @Override
    public void onRevoke() {
        Intent i = new Intent(this, RuntimeService.class);
        i.putExtra(RuntimeService.START_TYPE, RuntimeService.STOP_USER_INTERFACE);
        this.startService(i);

        stopZeroTier();
        try {
            vpnSocket.close();
        } catch (Exception e) {
            // swallow it
        }
        vpnSocket = null;

        stopSelf();

        Intent stopIntent = new Intent(this, RuntimeService.class);
        stopService(stopIntent);
    }

    public void run() {
        /*
        Log.d(TAG, "ZeroTierOne Service Started");

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        boolean autoRejoin = prefs.getBoolean("network_auto_rejoin_networks", true);

        if(autoRejoin) {
            // find all local network configs and join them
            File networksFolder = new File(getFilesDir(), "networks.d");
            if (networksFolder.exists()) {
                File[] networks = networksFolder.listFiles();
                for (File f : networks) {
                    if (f.getName().endsWith(".conf")) {
                        String filename = f.getName();
                        filename = filename.substring(0, filename.lastIndexOf('.'));
                        Log.d(TAG, "Loading network: " + filename);
                        ResultCode rc = node.join(NetworkIdUtils.hexStringToLong(filename));
                        if (rc != ResultCode.RESULT_OK) {
                            Log.d(TAG, "Error joining network: " + rc.toString());
                        }
                    }
                }
            }
        }

        Log.d(TAG, "This Node Address: " + Long.toHexString(node.address()));

        while(!Thread.interrupted()) {
            try {

                long dl = nextBackgroundTaskDeadline;
                long now = System.currentTimeMillis();

                if (dl <= now) {
                    long[] returnDeadline = {0};
                    ResultCode rc = node.processBackgroundTasks(now, returnDeadline);

                    synchronized(this) {
                        nextBackgroundTaskDeadline = returnDeadline[0];
                    }

                    if(rc != ResultCode.RESULT_OK) {
                        Log.e(TAG, "Error on processBackgroundTasks: " + rc.toString());
                        shutdown();
                    }
                }
                long delay = (dl > now) ? (dl - now) : 100;
                Thread.sleep(delay);
            } catch (InterruptedException ie) {
                break;
            } catch (Exception ex) {
                Log.e(TAG, ex.toString());
            }
        }
        Log.d(TAG, "ZeroTierOne Service Ended");
        */
    }

    //
    // EventBus events
    //

    public void onEvent(StopEvent e) {
        stopZeroTier();
    }

    public void onEvent(ManualDisconnectEvent e) {
        stopZeroTier();
    }

    public void onEventBackgroundThread(JoinNetworkEvent e) {
        /*
        Log.d(TAG, "Join Network Event");
        if(node == null) {
            return;
        }

        // TODO: Remove this once multi-network support is in place
        for(long nwid : networkConfigs.keySet()) {
            onEventBackgroundThread(new LeaveNetworkEvent(nwid));
        }

        networkConfigs.clear();

        ResultCode rc = node.join(e.getNetworkId());
        if(rc != ResultCode.RESULT_OK) {
            eventBus.post(new ErrorEvent(rc));
        }
        */
    }

    public void onEventBackgroundThread(LeaveNetworkEvent e) {
        /*
        Log.d(TAG, "Leave Network Event");

        if(node != null) {
            ResultCode rc = node.leave(e.getNetworkId());
            if (rc != ResultCode.RESULT_OK) {
                eventBus.post(new ErrorEvent(rc));
                return;
            }
            String certsFile = "networks.d/" + Long.toHexString(e.getNetworkId()) + ".mcerts";
            String confFile = "networks.d/" + Long.toHexString(e.getNetworkId()) + ".conf";
            dataStore.onDelete(confFile);
            dataStore.onDelete(certsFile);
        }
        */
    }

    public void onEventBackgroundThread(RequestNetworkInfoEvent e) {
        /*
        if(node == null) {
            return;
        }

        VirtualNetworkConfig vnc = node.networkConfig(e.getNetworkId());
        if(vnc != null) {
            eventBus.post(new NetworkInfoReplyEvent(vnc));
        }
        */
    }

    public void onEventBackgroundThread(RequestNetworkListEvent e) {
        /*
        if(node == null) {
            return;
        }



        VirtualNetworkConfig[] networks = node.networks();
        if(networks != null && networks.length > 0) {
            eventBus.post(new NetworkListReplyEvent(networks));
        }
        */
    }

    public void onEventBackgroundThread(RequestNodeStatusEvent e) {
        /*
        if (node == null) {
            return;
        }

        NodeStatus ns = node.status();

        eventBus.post(new NodeStatusEvent(ns));
        */
    }

    public void onEventAsync(NetworkReconfigureEvent e) {
        updateTunnelConfig();
    }

    //
    // Event Listener Overrides
    //
    public void onEvent(Event e) {
        /*
        Log.d(TAG, "Event: " + e.toString());

        if(node != null) {
            NodeStatus status = node.status();
            NodeStatusEvent nse = new NodeStatusEvent(status);
            eventBus.post(nse);
        }
        */
    }

    public void onTrace(String msg) {
        Log.d(TAG, "Trace: " + msg);
    }

    //
    // Virtual Network Config Listener overrides
    //

    public int onNetworkConfigurationUpdated(
            long nwid,
            VirtualNetworkConfigOperation op,
            VirtualNetworkConfig config) {
        /*
        Log.d(TAG, "Virtual Network Config Operation: " + op.toString());
        switch(op) {
            case VIRTUAL_NETWORK_CONFIG_OPERATION_UP: {
                Log.d(TAG, "Network Type:" + config.networkType().toString() + " " +
                        "Network Status: " + config.networkStatus().toString() + " " +
                        "Network Name: " + config.name() + " ");

                eventBus.post(new NetworkInfoReplyEvent(config));
            }
            break;

            case VIRTUAL_NETWORK_CONFIG_OPERATION_CONFIG_UPDATE: {
                Log.d(TAG, "Network Config Update!");


                VirtualNetworkConfig cfg = null;
                synchronized (networkConfigs) {
                    if (networkConfigs.containsKey(nwid)) {
                        cfg = networkConfigs.get(nwid);
                    }
                }

                if(cfg == null) {
                    // we don't already have this network config
                    Log.d(TAG, "Adding new network.");
                    synchronized (networkConfigs) {
                        networkConfigs.put(nwid, config);
                    }
                    eventBus.post(new NetworkReconfigureEvent());
                    eventBus.post(new NetworkInfoReplyEvent(config));
                    break;
                }

                if(!cfg.equals(config)) {
                    Log.d(TAG, "Updating network config");
                    synchronized (networkConfigs) {
                        networkConfigs.remove(nwid);
                        networkConfigs.put(nwid, config);
                    }
                    eventBus.post(new NetworkReconfigureEvent());
                }

                eventBus.post(new NetworkInfoReplyEvent(config));
            }
            break;

            case VIRTUAL_NETWORK_CONFIG_OPERATION_DOWN:
            case VIRTUAL_NETWORK_CONFIG_OPERATION_DESTROY:
                Log.d(TAG, "Network Down!");
                synchronized (networkConfigs) {
                    if (networkConfigs.containsKey(nwid)) {
                        networkConfigs.remove(nwid);
                    }
                }

                eventBus.post(new NetworkReconfigureEvent());
                eventBus.post(new NetworkRemovedEvent(nwid));
                break;
            default:
                Log.e(TAG, "Unknown Network Config Operation!");
                break;
        }
        */
        return 0;
    }

    protected void shutdown() {
        stopZeroTier();
        this.stopSelf();
    }

    /**
     * This should ONLY be called from onEventAsync(NetworkReconfigureEvent)
     */
    private void updateTunnelConfig() {

        /*
        synchronized (networkConfigs) {
            if (networkConfigs.isEmpty()) {
                return;
            }

            if (tunTapAdapter.isRunning()) {
                tunTapAdapter.interrupt();
            }

            tunTapAdapter.clearRouteMap();

            if (vpnSocket != null) {
                try {
                    vpnSocket.close();
                    in.close();
                    out.close();
                } catch (Exception e) {
                    // ignore
                }
                vpnSocket = null;
                in = null;
                out = null;
            }

            Builder builder = new Builder();

            int highestMtu = 0;

            for (VirtualNetworkConfig config : networkConfigs.values()) {
                if (config.isEnabled()) {
                    long nwid = config.networkId();

                    InetSocketAddress addresses[] = config.assignedAddresses();

                    int adi = 0;

                    for (int i = 0; i < addresses.length; ++i) {
                        Log.d(TAG, "Adding VPN Address: " + addresses[i].getAddress() + " Mac: " + Long.toHexString(config.macAddress()));

                        byte[] addrBytes = addresses[i].getAddress().getAddress();

                        try {
                            adi = ByteBuffer.wrap(addrBytes).getInt();
                        } catch (Exception e) {
                            Log.e(TAG, "Exception calculating multicast ADI: " + e.getMessage());
                            continue;
                        }

                        int routeSub = addresses[i].getPort();
                        InetAddress address = addresses[i].getAddress();

                        // TODO: Support IPv6
                        if(address instanceof Inet6Address) {
                            Log.d(TAG, "Got an IPV6 Address. Not adding it to the adapter");
                            continue;
                        }

                        InetAddress route = InetAddressUtils.addressToRoute(address, routeSub);
                        if(route == null) {
                            Log.e(TAG, "NULL route calculated!");
                            continue;
                        }

                        ResultCode rc = node.multicastSubscribe(nwid, TunTapAdapter.BROADCAST_MAC, adi);
                        if (rc != ResultCode.RESULT_OK) {
                            Log.e(TAG, "Error joining multicast group");
                        } else {
                            Log.d(TAG, "Joined multicast group");
                        }

                        builder.addAddress(address, routeSub);
                        builder.addRoute(route, routeSub);

                        Route r = new Route(route, routeSub);
                        tunTapAdapter.addRouteAndNetwork(r, nwid);


                        int mtu = config.mtu();
                        if (mtu > highestMtu) {
                            highestMtu = mtu;
                        }
                    }
                }
            }
            builder.setMtu(highestMtu);
            builder.setSession("ZeroTier One");


            vpnSocket = builder.establish();
            if(vpnSocket == null) {
                Log.e(TAG, "vpnSocket is NULL after builder.establish()!!!!");
                stopZeroTier();
                return;
            }

            in = new FileInputStream(vpnSocket.getFileDescriptor());
            out = new FileOutputStream(vpnSocket.getFileDescriptor());

            tunTapAdapter.setVpnSocket(vpnSocket);
            tunTapAdapter.setFileStreams(in, out);
            tunTapAdapter.startThreads();
            Log.i(TAG, "ZeroTier One Connected");
        }
        */
    }
}
