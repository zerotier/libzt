package zerotier;

import java.math.BigInteger;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.UnknownHostException;
import java.util.regex.Pattern;

/*

The ZTAddress object is merely a convenience object for moving address information
across the JNI memory border.

*/

public class Address
{
    // int -> byte array
    static public byte[] toIPByteArray(long addr){
        return new byte[]{(byte)addr,(byte)(addr>>>8),(byte)(addr>>>16),(byte)(addr>>>24)};
    }

    // byte array -> int
    long toIPInt(String _addr) {
        long result = 0;
        for(String part: _addr.split(Pattern.quote("."))) {
            result = result << 8;
            result |= Integer.parseInt(part);
        }
        return result;
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
            // should never happen
            return null;
        }
    }

    public String toString() {
        return Address() + ":" + Port();
    }

    public Address()
    {
        port = -1;
        _rawAddr = -1;
    }

    public Address(String _addr, int _port)
    {
        _rawAddr = toIPInt(_addr);
        port = _port;
    }

    public void ZTAddress(InetSocketAddress ins)
    {
        port = ins.getPort();
        _rawAddr = toIPInt(ins.getAddress().getHostAddress());
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

    public boolean isValid() {
        return port != -1 && !Address().startsWith("-1.-1.-1.-1/-1");
    }
}