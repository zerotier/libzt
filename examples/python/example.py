"""Example low-level socket usage"""

import time
import sys

import libzt


def print_usage():
    """print help"""
    print(
        "\nUsage: <server|client> <storage_path> <net_id> <remote_ip> <remote_port>\n"
    )
    print("Ex: python3 example.py server . 0123456789abcdef 8080")
    print("Ex: python3 example.py client . 0123456789abcdef 192.168.22.1 8080\n")
    if len(sys.argv) < 5:
        print("Too few arguments")
    if len(sys.argv) > 6:
        print("Too many arguments")
    sys.exit(0)


#
# (Optional) Event handler
#
def on_zerotier_event(event_code, id):
    if event_code == libzt.ZTS_EVENT_NODE_ONLINE:
        print("ZTS_EVENT_NODE_ONLINE (" + str(event_code) + ") : " + hex(id))
    if event_code == libzt.ZTS_EVENT_NODE_OFFLINE:
        print("ZTS_EVENT_NODE_OFFLINE (" + str(event_code) + ") : " + hex(id))
    if event_code == libzt.ZTS_EVENT_NETWORK_READY_IP4:
        print("ZTS_EVENT_NETWORK_READY_IP4 (" + str(event_code) + ") : " + hex(id))
    if event_code == libzt.ZTS_EVENT_NETWORK_READY_IP6:
        print("ZTS_EVENT_NETWORK_READY_IP6 (" + str(event_code) + ") : " + hex(id))
    if event_code == libzt.ZTS_EVENT_PEER_DIRECT:
        print("ZTS_EVENT_PEER_DIRECT (" + str(event_code) + ") : " + hex(id))
    if event_code == libzt.ZTS_EVENT_PEER_RELAY:
        print("ZTS_EVENT_PEER_RELAY (" + str(event_code) + ") : " + hex(id))


#
# Main
#
def main():
    mode = None  # client|server
    storage_path = "."  # Where identity files are stored
    net_id = 0  # Network to join
    remote_ip = None  # ZeroTier IP of remote node
    remote_port = 8080  # ZeroTier port your app logic may use

    if len(sys.argv) < 5 or len(sys.argv) > 6:
        print_usage()
    if sys.argv[1] == "server" and len(sys.argv) == 5:
        mode = sys.argv[1]
        storage_path = sys.argv[2]
        net_id = int(sys.argv[3], 16)
        remote_port = int(sys.argv[4])
    if sys.argv[1] == "client" and len(sys.argv) == 6:
        mode = sys.argv[1]
        storage_path = sys.argv[2]
        net_id = int(sys.argv[3], 16)
        remote_ip = sys.argv[4]
        remote_port = int(sys.argv[5])
    if mode is None:
        print_usage()
    print("mode         = ", mode)
    print("storage_path = ", storage_path)
    print("net_id       = ", net_id)
    print("remote_ip    = ", remote_ip)
    print("remote_port  = ", remote_port)

    #
    #  Node initialization and start
    #
    print("Starting ZeroTier...")

    n = libzt.ZeroTierNode()
    n.init_set_event_handler(on_zerotier_event)  # Optional
    n.init_from_storage(storage_path)  # Optional
    n.init_set_port(9994)  # Optional
    n.node_start()

    print("Waiting for node to come online...")
    while not n.node_is_online():
        time.sleep(1)
    print("Joining network:", hex(net_id))
    n.net_join(net_id)
    while not n.net_transport_is_ready(net_id):
        time.sleep(1)
    print("Joined network")

    #
    # Example server
    #
    if mode == "server":
        print("Starting server...")
        serv = libzt.socket(libzt.ZTS_AF_INET, libzt.ZTS_SOCK_STREAM, 0)
        try:
            # serv.setblocking(True)
            serv.bind(("0.0.0.0", remote_port))
            serv.listen(5)
            while True:
                conn, addr = serv.accept()
                print("Accepted connection from: ", addr)
                while True:
                    data = conn.recv(4096)
                    if data:
                        print("recv: ", data)
                    if not data:
                        break
                    print("send: ", data)
                    sent_bytes = conn.send(data)  # echo back to the server
                    print("sent: " + str(sent_bytes) + " byte(s)")
                conn.close()
                print("client disconnected")
        except Exception as ex:
            print(ex)
        print("errno=", libzt.errno())  # See include/ZeroTierSockets.h for codes

    #
    # Example client
    #
    if mode == "client":
        print("Starting client...")
        client = libzt.socket(libzt.ZTS_AF_INET, libzt.ZTS_SOCK_STREAM, 0)
        try:
            print("connecting...")
            client.connect((remote_ip, remote_port))
            data = "Hello, network!"
            print("send: ", data)
            sent_bytes = client.send(data)
            print("sent: " + str(sent_bytes) + " byte(s)")
            data = client.recv(1024)
            print("recv: ", repr(data))
        except Exception as ex:
            print(ex)
        print("errno=", libzt.errno())


if __name__ == "__main__":
    main()
