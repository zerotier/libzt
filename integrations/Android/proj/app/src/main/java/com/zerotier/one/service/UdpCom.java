package com.zerotier.one.service;

import android.util.Log;

import com.zerotier.sdk.Node;
import com.zerotier.sdk.PacketSender;
import com.zerotier.sdk.ResultCode;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.Inet6Address;
import java.net.InetSocketAddress;
import java.net.SocketTimeoutException;

/**
 * Created by Grant on 6/3/2015.
 */
public class UdpCom implements PacketSender, Runnable {
    private final static String TAG = "UdpCom";

    Node node;
    ZeroTierOneService ztService;
    DatagramSocket svrSocket;

    UdpCom(ZeroTierOneService service, DatagramSocket socket) {
        svrSocket = socket;
        this.ztService = service;
    }

    public void setNode(Node node) {
        this.node = node;
    }

    public int onSendPacketRequested(
            InetSocketAddress localAddress,
            InetSocketAddress remoteAddress,
            byte[] packetData,
            int ttl) {
        // TTL is ignored.  No way to set it on a UDP packet in Java
        if(svrSocket == null) {
            Log.e(TAG, "Attempted to send packet on a null socket");
            return -1;
        }

        try {
            DatagramPacket p = new DatagramPacket(packetData, packetData.length, remoteAddress);
            Log.d(TAG, "onSendPacketRequested: Sent "+ p.getLength() + " bytes to " + remoteAddress.toString());
            svrSocket.send(p);
        } catch (Exception ex) {
            return -1;
        }
        return 0;
    }

    public void run() {
        Log.d(TAG, "UDP Listen Thread Started.");
        try {
            long[] bgtask = new long[1];
            byte[] buffer = new byte[16384];
            while(!Thread.interrupted()) {

                bgtask[0] = 0;
                DatagramPacket p = new DatagramPacket(buffer, buffer.length);

                try {
                    svrSocket.receive(p);
                    if(p.getLength() > 0)
                    {
                        byte[] received = new byte[p.getLength()];
                        System.arraycopy(p.getData(), 0, received, 0, p.getLength());
                        Log.d(TAG, "Got " + p.getLength() + " Bytes From: " + p.getAddress().toString() +":" + p.getPort());

                        ResultCode rc = node.processWirePacket(System.currentTimeMillis(), null, new InetSocketAddress(p.getAddress(), p.getPort()), received, bgtask);

                        if(rc != ResultCode.RESULT_OK) {
                            Log.e(TAG, "procesWirePacket returned: " + rc.toString());
                            ztService.shutdown();
                        }

                        ztService.setNextBackgroundTaskDeadline( bgtask[0] );
                    }
                } catch (SocketTimeoutException e) {
                    Log.d(TAG, "Socket Timeout");
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        Log.d(TAG, "UDP Listen Thread Ended.");
    }
}
