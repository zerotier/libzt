/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2017  ZeroTier, Inc.  https://www.zerotier.com/
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
}