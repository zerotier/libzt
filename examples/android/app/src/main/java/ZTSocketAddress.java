/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2018  ZeroTier, Inc.  https://www.zerotier.com/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --
 *
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial closed-source software that incorporates or links
 * directly against ZeroTier software without disclosing the source code
 * of your own application.
 */

package zerotier;

import java.net.*;

// Designed to transport address information across the JNI boundary
public class ZTSocketAddress
{
    public byte[] _ip6 = new byte[16];
    public byte[] _ip4 = new byte[4];

    public long _ipdata;
    public long _ipdata_ext;

    public int _family;
    public int _port; // Also reused for netmask or prefix

    public ZTSocketAddress() {}

    public ZTSocketAddress(String ipStr, int port)
    {
        if(ipStr.contains(":")) {
            _family = zerotier.ZeroTier.AF_INET6;
            try {
                InetAddress ip = InetAddress.getByName(ipStr);
                _ip6 = ip.getAddress();
            }
            catch (Exception e) { }
        }
        else if(ipStr.contains(".")) {
            _family = zerotier.ZeroTier.AF_INET;
            try {
                InetAddress ip = InetAddress.getByName(ipStr);
                _ip4 = ip.getAddress();
            }
            catch (Exception e) { }     
        }
        _port = port;
    }

    public int getPort() { return _port; }
    public int getNetmask() { return _port; }
    public int getPrefix() { return _port; }

    private String ipString()
    {
        if (_family == zerotier.ZeroTier.AF_INET) {
            try {
                InetAddress inet = InetAddress.getByAddress(_ip4);
                return "" + inet.getHostAddress();
            } catch (Exception e) {
                System.out.println(e);
            }
        }
        if (_family == zerotier.ZeroTier.AF_INET6) {
            try {
                InetAddress inet = InetAddress.getByAddress(_ip6);
                return "" + inet.getHostAddress();
            } catch (Exception e) {
                System.out.println(e);
            }
        }
        return "";
    }

    public String toString() { return ipString() + ":" + _port; }
    public String toCIDR() { return ipString() + "/" + _port; }
}
