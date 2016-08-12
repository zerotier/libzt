package ZeroTier;

import java.math.BigInteger;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.UnknownHostException;

public class ZTAddress
{
    static public byte[] toIPByteArray(long addr){
        return new byte[]{(byte)addr,(byte)(addr>>>8),(byte)(addr>>>16),(byte)(addr>>>24)};
    }

    int pack(byte[] bytes) {
        int val = 0;
        for (int i = 0; i < bytes.length; i++) {
            val <<= 8;
            val |= bytes[i] & 0xff;
        }
        return val;
    }

    public int port;
    public int Port() {
        return port;
    }

    public long _rawAddr;
    public String Address()
    {
        try {
            return InetAddress.getByAddress(toIPByteArray(_rawAddr)).getHostAddress();
        } catch (UnknownHostException e) {
            //should never happen
            return null;
        }
    }

    public String toString() {
        return Address() + ":" + Port();
    }

    public ZTAddress()
    {

    }

    public ZTAddress(String _addr, int _port)
    {
        port = _port;
        _rawAddr = pack(_addr.getBytes());
    }

    public void ZTAddress(InetSocketAddress ins)
    {

    }

    public InetSocketAddress ToInetSocketAddress() throws IllegalArgumentException {
        InetSocketAddress sock_addr = null;
        try {
            sock_addr = new InetSocketAddress(Address(), port);
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        }
        return sock_addr;
    }
}