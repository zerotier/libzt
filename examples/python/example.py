'''Example low-level socket usage'''

import time
import sys

import libzt

def print_usage():
    '''print help'''
    print(
        "\nUsage: <server|client> <id_path> <nwid> <zt_service_port> <remote_ip> <remote_port>\n"
    )
    print("Ex: python3 demo.py server . 0123456789abcdef 9994 8080")
    print("Ex: python3 demo.py client . 0123456789abcdef 9994 192.168.22.1 8080\n")
    if len(sys.argv) < 6:
        print("Too few arguments")
    if len(sys.argv) > 7:
        print("Too many arguments")
    sys.exit(0)


is_joined = False # Flags to keep state
is_online = False # Flags to keep state

#
# Event handler
#
class MyEventCallbackClass(libzt.EventCallbackClass):
    def on_zerotier_event(self, msg):
        global is_online
        global is_joined
        print("eventCode=", msg.eventCode)
        if msg.eventCode == libzt.ZTS_EVENT_NODE_ONLINE:
            print("ZTS_EVENT_NODE_ONLINE")
            print("nodeId=" + hex(msg.node.address))
            # The node is now online, you can join/leave networks
            is_online = True
        if msg.eventCode == libzt.ZTS_EVENT_NODE_OFFLINE:
            print("ZTS_EVENT_NODE_OFFLINE")
        if msg.eventCode == libzt.ZTS_EVENT_NETWORK_READY_IP4:
            print("ZTS_EVENT_NETWORK_READY_IP4")
            is_joined = True
            # The node has successfully joined a network and has an address
            # you can perform network calls now
        if msg.eventCode == libzt.ZTS_EVENT_PEER_DIRECT:
            print("ZTS_EVENT_PEER_DIRECT")
        if msg.eventCode == libzt.ZTS_EVENT_PEER_RELAY:
            print("ZTS_EVENT_PEER_RELAY")

#
# Main
#
def main():
    global is_online
    global is_joined

    key_file_path = "." # Where identity files are stored
    network_id = 0 # Network to join
    # Port used by ZeroTier to send encpryted UDP traffic
    # NOTE: Should be different from other instances of ZeroTier
    # running on the same machine
    zt_service_port = 9997
    remote_ip = None # ZeroTier IP of remote node
    remote_port = 8080 # ZeroTier port your app logic may use
    mode = None # client|server

    if len(sys.argv) < 6 or len(sys.argv) > 7:
        print_usage()
    if sys.argv[1] == "server" and len(sys.argv) == 6:
        mode = sys.argv[1]
        key_file_path = sys.argv[2]
        network_id = int(sys.argv[3], 16)
        zt_service_port = int(sys.argv[4])
        remote_port = int(sys.argv[5])
    if sys.argv[1] == "client" and len(sys.argv) == 7:
        mode = sys.argv[1]
        key_file_path = sys.argv[2]
        network_id = int(sys.argv[3], 16)
        zt_service_port = int(sys.argv[4])
        remote_ip = sys.argv[5]
        remote_port = int(sys.argv[6])
    if mode is None:
        print_usage()
    print("mode          = ", mode)
    print("path          = ", key_file_path)
    print("network_id     = ", network_id)
    print("zt_service_port = ", zt_service_port)
    print("remote_ip      = ", remote_ip)
    print("remote_port    = ", remote_port)

    #
    # Example start and join logic
    #
    print("Starting ZeroTier...")
    event_callback = MyEventCallbackClass()
    libzt.start(key_file_path, event_callback, zt_service_port)
    print("Waiting for node to come online...")
    while not is_online:
        time.sleep(1)
    print("Joining network:", hex(network_id))
    libzt.join(network_id)
    while not is_joined:
        time.sleep(1)  # You can ping this app at this point
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
            data = "Hello, world!"
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
