package com.zerotier.one.service;

import android.os.ParcelFileDescriptor;
import android.util.Log;

import com.zerotier.one.util.IPPacketUtils;
import com.zerotier.sdk.Node;
import com.zerotier.sdk.ResultCode;
import com.zerotier.sdk.VirtualNetworkConfig;
import com.zerotier.sdk.VirtualNetworkFrameListener;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.net.InetAddress;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.HashMap;

/**
 * Created by Grant on 6/3/2015.
 */
public class TunTapAdapter implements VirtualNetworkFrameListener {
    public static final String TAG = "TunTapAdapter";

    private static final int ARP_PACKET  = 0x0806;
    private static final int IPV4_PACKET = 0x0800;
    private static final int IPV6_PACKET = 0x86dd;

    public static final long BROADCAST_MAC = 0xffffffffffffL;

    private ARPTable arpTable;
    private Node node;
    private ZeroTierOneService ztService;
    private ParcelFileDescriptor vpnSocket;
    private FileInputStream in;
    private FileOutputStream out;

    private final HashMap<Route, Long> routeMap;

    private Thread receiveThread;

    public TunTapAdapter(ZeroTierOneService ztService) {
        this.ztService = ztService;
        arpTable = new ARPTable();
        routeMap = new HashMap<Route, Long>();
    }

    public void setNode(Node node) {
        this.node = node;
    }

    public void setVpnSocket(ParcelFileDescriptor vpnSocket) {
        this.vpnSocket = vpnSocket;
    }

    public void setFileStreams(FileInputStream in, FileOutputStream out) {
        this.in = in;
        this.out = out;
    }

    public void addRouteAndNetwork(Route route, long networkId) {
        synchronized (routeMap) {
            routeMap.put(route, networkId);
        }
    }

    public void clearRouteMap() {
        synchronized (routeMap) {
            routeMap.clear();
        }
    }

    public void startThreads() {
        receiveThread = new Thread("Tunnel Receive Thread") {
            public void run() {
                try {
                    Log.d(TAG, "TUN Receive Thread Started");

                    ByteBuffer buffer = ByteBuffer.allocate(32767);
                    buffer.order(ByteOrder.LITTLE_ENDIAN);
                    try {
                        while (!isInterrupted()) {
                            boolean idle = true;
                            int length = in.read(buffer.array());
                            if (length > 0) {
                                Log.d(TAG, "Sending packet to ZeroTier. " + length + " bytes.");
                                idle = false;

                                byte packet[] = new byte[length];
                                System.arraycopy(buffer.array(), 0, packet, 0, length);

                                InetAddress destAddress = IPPacketUtils.getDestIP(packet);
                                InetAddress sourceAddress = IPPacketUtils.getSourceIP(packet);

                                long nwid = networkIdForDestination(destAddress);
                                if (nwid == 0) {
                                    Log.e(TAG, "Unable to find network ID for destination address: " + destAddress);
                                    continue;
                                }

                                VirtualNetworkConfig cfg = node.networkConfig(nwid);
                                InetAddress myAddress = cfg.assignedAddresses()[0].getAddress();

                                long srcMac = cfg.macAddress();
                                long bgt_dl[] = new long[1];

                                if (arpTable.hasMacForAddress(destAddress)) {
                                    long destMac = arpTable.getMacForAddress(destAddress);

                                    ResultCode rc = node.processVirtualNetworkFrame(
                                            System.currentTimeMillis(),
                                            nwid,
                                            srcMac,
                                            destMac,
                                            IPV4_PACKET,
                                            0,
                                            packet,
                                            bgt_dl);

                                    if (rc != ResultCode.RESULT_OK) {
                                        Log.e(TAG, "Error calling processVirtualNetworkFrame: " + rc.toString());
                                    } else {
                                        Log.d(TAG, "Packet sent to ZT");
                                        ztService.setNextBackgroundTaskDeadline(bgt_dl[0]);
                                    }
                                } else {
                                    Log.d(TAG, "Unknown dest MAC address.  Need to look it up. " + destAddress);
                                    byte[] arpRequest = arpTable.getRequestPacket(srcMac, myAddress, destAddress);

                                    ResultCode rc = node.processVirtualNetworkFrame(
                                            System.currentTimeMillis(),
                                            nwid,
                                            srcMac,
                                            BROADCAST_MAC,
                                            ARP_PACKET,
                                            0,
                                            arpRequest,
                                            bgt_dl);

                                    if (rc != ResultCode.RESULT_OK) {
                                        Log.e(TAG, "Error sending ARP packet: " + rc.toString());
                                    } else {
                                        Log.d(TAG, "ARP Request Sent!");
                                        ztService.setNextBackgroundTaskDeadline(bgt_dl[0]);
                                    }
                                }
                                buffer.clear();
                            } else {
                                //Log.d(TAG, "No bytes read: " + length);
                            }

                            if (idle) {
                                Thread.sleep(100);
                            }
                        }
                    } catch (InterruptedException e) {
                        throw e;
                    } catch (Exception e) {
                        Log.e(TAG, "Error in TUN Receive: " + e.getMessage());
                    }
                } catch (Exception e) {
                    // swallow InterruptedException
                }
                Log.d(TAG, "TUN Receive Thread ended");
            }
        };

        receiveThread.start();
    }

    public void interrupt() {
        if (receiveThread != null) {
            receiveThread.interrupt();
            try {
                receiveThread.join();
            } catch (InterruptedException e) {
                // swallow
            }
        }
    }

    public boolean isRunning() {
        if (receiveThread == null) {
            return false;
        } else {
            return receiveThread.isAlive();
        }
    }

    public void onVirtualNetworkFrame(
            long nwid,
            long srcMac,
            long destMac,
            long etherType,
            long vlanId,
            byte[] frameData) {
        Log.d(TAG, "Got Virtual Network Frame.  Network ID: " + Long.toHexString(nwid) + " Source MAC: " + Long.toHexString(srcMac) +
                " Dest MAC: " + Long.toHexString(destMac) + " Ether type: " + etherType + " VLAN ID: " + vlanId + " Frame Length: " + frameData.length);

        if (vpnSocket == null) {
            Log.e(TAG, "vpnSocket is null!");
            return;
        }

        if (in == null || out == null) {
            Log.e(TAG, "no in/out streams");
            return;
        }

        if (etherType == ARP_PACKET) {
            Log.d(TAG, "Got ARP Packet");

            ARPTable.ARPReplyData data = arpTable.processARPPacket(frameData);

            if (data != null && data.destMac != 0 && data.destAddress != null) {
                // We need to reply here.
                long deadline[] = new long[1];

                VirtualNetworkConfig cfg = node.networkConfig(nwid);

                InetAddress myAddress = cfg.assignedAddresses()[0].getAddress();

                byte[] replyPacket = arpTable.getReplyPacket(cfg.macAddress(), myAddress, data.destMac, data.destAddress);
                ResultCode rc = node.processVirtualNetworkFrame(
                        System.currentTimeMillis(),
                        nwid,
                        cfg.macAddress(),
                        srcMac,
                        ARP_PACKET,
                        0,
                        replyPacket,
                        deadline);

                if (rc != ResultCode.RESULT_OK) {
                    Log.e(TAG, "Error sending ARP packet: " + rc.toString());
                } else {
                    Log.d(TAG, "ARP Reply Sent!");
                    ztService.setNextBackgroundTaskDeadline(deadline[0]);
                }
            }

        } else if ((etherType == IPV4_PACKET) ||
                (etherType == IPV6_PACKET)) {
            // Got IPv4 or IPv6 packet!
            Log.d(TAG, "Got IP packet. Length: " + frameData.length + " Bytes");

            try {
                InetAddress sourceAddress = IPPacketUtils.getSourceIP(frameData);
                if (sourceAddress != null) {
                    arpTable.setAddress(sourceAddress, srcMac);
                }
                out.write(frameData);
            } catch (Exception e) {
                Log.e(TAG, "Error writing data to vpn socket: " + e.getMessage());
            }
        } else {
            Log.d(TAG, "Unknown Packet Type Received: 0x" + String.format("%02X%02X", frameData[12], frameData[13]));
        }
    }

    private long networkIdForDestination(InetAddress destination) {
        synchronized (routeMap) {
            for (Route r : routeMap.keySet()) {
                if (r.belongsToRoute(destination)) {
                    return routeMap.get(r);
                }
            }
        }

        return 0L;
    }
}
