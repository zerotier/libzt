package ZeroTier;

import java.net.InetSocketAddress;

public class ZTAddress
{
    public String addr;
    public int port;

    public InetSocketAddress ToInetSocketAddress() throws IllegalArgumentException {
        InetSocketAddress sock_addr = null;
        try {
            sock_addr = new InetSocketAddress(addr, port);
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        }
        return sock_addr;
    }
}