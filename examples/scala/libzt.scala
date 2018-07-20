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

class ZeroTier {
  
  // socket families
  // socket types
  // basic service controls
  @native def start(path: String, blocking: Boolean): Int
  @native def startjoin(path: String, nwid: Long): Int
  @native def stop(): Unit
  @native def running(): Int
  @native def join(nwid: Long): Unit
  @native def leave(nwid: Long): Unit
  // advanced service controls
  //@native def get_path(): Unit
  @native def get_node_id(): Long
  //@native def get_6plane_addr(): Unit
  //@native def get_rfc4193_addr(): Unit
  // socket API
  @native def socket(socket_family: Int, socket_type: Int, protocol: Int): Int  
  @native def connect(fd: Int, addr: String, port: Int): Int
  @native def bind(fd: Int, addr: String, port: Int): Int
  @native def listen(fd: Int, backlog: Int): Int
  @native def accept(fd: Int, addr: Object, addrlen: Int): Int
  @native def accept4(fd: Int, addr: Object, addrlen: Int, flags: Int): Int
  @native def close(fd: Int): Int
  @native def setsockopt(fd: Int, level: Int, optname: Int, optval: Object, optlen: Int): Int
  @native def getsockopt(fd: Int, level: Int, optname: Int, optval: Object, optlen: Int): Int
  @native def read(fd: Int, buf: Object, len: Int): Int
  @native def write(fd: Int, buf: Object, len: Int): Int
  @native def send(fd: Int, buf: Object, len: Int, flags: Int): Int
  @native def sendto(fd: Int, buf: Object, len: Int, addr: Object, addrlen: Int): Int
  @native def sendmsg(fd: Int, msg: Object, flags: Int): Int
  @native def recv(fd: Int, buf: Object, len: Int, flags: Int): Int
  @native def recvfrom(fd: Int, buf: Object, len: Int, addr: Object, addrlen: Int): Int
  @native def recvmsg(fd: Int, msg: Object, flags: Int): Int
  @native def shutdown(fd: Int, how: Int): Int
  //@native def getsockname(): Int
  //@native def getpeername(): Int
  //@native def gethostname(): Int
  //@native def sethostname(): Int
  //@native def gethostbyname(): Object
  //@native def poll(): Int
  //@native def select(): Int
  @native def fcntl(fd: Int, cmd: Int, flags: Int): Int
  @native def ioctl(fd: Int, request: Long, argp: Object): Int
  // stack controls
  //@native def add_dns(): Int
  //@native def del_dns(): Int
}
