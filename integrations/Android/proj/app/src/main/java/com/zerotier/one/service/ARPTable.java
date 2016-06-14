package com.zerotier.one.service;

import android.util.Log;

import java.net.InetAddress;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;

public class ARPTable {
    public static final String TAG = "ARPTable";

    private static final int REQUEST = 1;
    private static final int REPLY   = 2;

    private final HashMap<InetAddress, Long> inetAddressToMacAddress;
    private final HashMap<Long, InetAddress> macAddressToInetAdddress;

    private static final long ENTRY_TIMEOUT = 120000L; // 2 minutes

    private final ArrayList<ArpEntry> entries;

    public ARPTable() {
        entries = new ArrayList<ArpEntry>();
        inetAddressToMacAddress = new HashMap<InetAddress, Long>();
        macAddressToInetAdddress = new HashMap<Long, InetAddress>();
        timeoutThread.start();
    }

    @Override
    protected void finalize() throws Throwable {
        timeoutThread.interrupt();
        super.finalize();
    }

    void setAddress(InetAddress addr, long mac) {
        synchronized (inetAddressToMacAddress) {
            inetAddressToMacAddress.put(addr, mac);
        }
        synchronized (macAddressToInetAdddress) {
            macAddressToInetAdddress.put(mac, addr);
        }

        synchronized (entries) {
            ArpEntry entry = new ArpEntry(mac, addr);
            if(entries.contains(entry)) {
                // update the entry time
                int index = entries.indexOf(entry);
                entries.get(index).updateTime();
            } else {
                entries.add(entry);
            }
        }
    }

    private void updateArpEntryTime(long mac) {
        synchronized (entries) {
            for(ArpEntry e : entries) {
                if(mac == e.getMac()) {
                    e.updateTime();
                }
            }
        }
    }

    private void updateArpEntryTime(InetAddress addr) {
        synchronized (entries) {
            for(ArpEntry e : entries) {
                if(addr.equals(e.getAddress())) {
                    e.updateTime();
                }
            }
        }
    }

    /**
     * Gets the MAC address for a given InetAddress
     *
     * @param addr address to get the MAC for
     * @return MAC address as long.  -1 if the address isn't known
     */
    long getMacForAddress(InetAddress addr) {
        synchronized (inetAddressToMacAddress) {
            if(inetAddressToMacAddress.containsKey(addr)) {
                long mac = inetAddressToMacAddress.get(addr);
                updateArpEntryTime(mac);
                return mac;
            }
        }
        return -1;
    }

    /**
     * Get the InetAddress for a given MAC address
     *
     * @param mac mac address to lookup.
     * @return InetAddress if it's in the map.  Otherwise null.
     */
    InetAddress getAddressForMac(long mac) {
        synchronized (macAddressToInetAdddress) {
            if (macAddressToInetAdddress.containsKey(mac)) {
                InetAddress addr = macAddressToInetAdddress.get(mac);
                updateArpEntryTime(addr);
                return addr;
            }
        }

        return null;
    }

    public boolean hasMacForAddress(InetAddress addr) {
        synchronized (inetAddressToMacAddress) {
            return inetAddressToMacAddress.containsKey(addr);
        }
    }

    public boolean hasAddressForMac(long mac) {
        synchronized (macAddressToInetAdddress) {
            return macAddressToInetAdddress.containsKey(mac);
        }
    }

    public static byte[] longToBytes(long l) {
        // allocate an 8 byte buffer. Long.SIZE returns number of bits so divide by 8
        ByteBuffer buffer = ByteBuffer.allocate(Long.SIZE/8);
        buffer.putLong(l);
        return buffer.array();
    }

    public byte[] getRequestPacket(long senderMac, InetAddress senderAddress, InetAddress destinationAddress) {
        return getARPPacket(REQUEST, senderMac, 0, senderAddress, destinationAddress);
    }

    public byte[] getReplyPacket(long senderMac, InetAddress senderAddress, long destMac, InetAddress destAddress) {
        return getARPPacket(REPLY, senderMac, destMac, senderAddress, destAddress);
    }

    public byte[] getARPPacket(int packetType, long senderMac, long destMac, InetAddress senderAddress, InetAddress destinationAddress) {
        byte[] packet = new byte[28];

        // Ethernet packet type
        packet[0] = 0;
        packet[1] = 1;

        // IPV4 Protocol.  0x0800
        packet[2] = 0x08;
        packet[3] = 0;

        // Hardware MAC address length
        packet[4] = 6;

        // IP address length
        packet[5] = 4;

        packet[6] = 0;
        packet[7] = (byte)packetType;

        byte[] senderMacBuffer = longToBytes(senderMac);
        System.arraycopy(senderMacBuffer, 2, packet, 8, 6);

        byte[] addrBuffer = senderAddress.getAddress();
        System.arraycopy(addrBuffer, 0, packet, 14, 4);

        byte[] destMacBuffer = longToBytes(destMac);
        System.arraycopy(destMacBuffer, 2, packet, 18, 6);

        byte[] destAddrBuffer = destinationAddress.getAddress();
        System.arraycopy(destAddrBuffer, 0, packet, 24, 4);

        return packet;
    }

    /**
     * Returns true if a reply is needed
     *
     * @param replyBuffer
     * @return
     */
    public ARPReplyData processARPPacket(byte[] replyBuffer) {
        Log.d(TAG, "Processing ARP packet");

        byte[] srcMacBuffer = new byte[8];
        System.arraycopy(replyBuffer, 8, srcMacBuffer, 2, 6);

        byte[] senderAddressBuffer = new byte[4];
        System.arraycopy(replyBuffer, 14, senderAddressBuffer, 0, 4);

        byte[] destMacBuffer = new byte[8];
        System.arraycopy(replyBuffer, 18, destMacBuffer, 2, 6);

        byte[] destAddressBuffer = new byte[4];
        System.arraycopy(replyBuffer, 24, destAddressBuffer, 0, 4);

        InetAddress senderAddress = null;
        try {
            senderAddress = InetAddress.getByAddress(senderAddressBuffer);
        } catch (Exception e) {
        }

        InetAddress destAddress = null;
        try {
            destAddress = InetAddress.getByAddress(destAddressBuffer);
        } catch (Exception e) {
        }

        long sourceMac = ByteBuffer.wrap(srcMacBuffer).getLong();
        long destMac = ByteBuffer.wrap(destMacBuffer).getLong();

        if(sourceMac != 0 && senderAddress != null) {
            setAddress(senderAddress,sourceMac);
        }

        if(destMac != 0 && destAddress != null) {
            setAddress(destAddress, destMac);
        }


        if(replyBuffer[7] == 1) {
            Log.d(TAG, "Reply needed");

            ARPReplyData data = new ARPReplyData();
            data.destMac = sourceMac;
            data.destAddress = senderAddress;

            return data;
        }

        return null;
    }

    public class ARPReplyData {
        public long senderMac;
        public InetAddress senderAddress;
        public long destMac;
        public InetAddress destAddress;
    }

    private class ArpEntry {
        private long mac;
        private InetAddress address;
        private long time;

        ArpEntry(long mac, InetAddress address) {
            this.mac = mac;
            this.address = address;
            updateTime();
        }

        public long getMac() { return this.mac; }
        public InetAddress getAddress() { return this.address; }

        public void updateTime() {
            this.time = System.currentTimeMillis();
        }

        public boolean equals(ArpEntry e) {
            // purposely do not check time
            // mac & address alone determine equality
            return  (mac == e.mac) &&
                    (address.equals(e.address));
        }
    }

    private Thread timeoutThread = new Thread("ARP Timeout Thread") {
        public void run() {
            try {
                synchronized (entries) {
                    for (ArpEntry e : entries) {
                        if(e.time < (System.currentTimeMillis() + ENTRY_TIMEOUT) ) {
                            synchronized(macAddressToInetAdddress) {
                                macAddressToInetAdddress.remove(e.mac);
                            }

                            synchronized (inetAddressToMacAddress) {
                                inetAddressToMacAddress.remove(e.address);
                            }

                            entries.remove(e);
                        }
                    }
                }

                Thread.sleep(1000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    };
}
