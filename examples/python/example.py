import time, sys

import libzt

# Where identity files are stored
keyPath = "."

# Network to join
networkId = 0

# Port used by ZeroTier to send encpryted UDP traffic
# NOTE: Should be different from other instances of ZeroTier
# running on the same machine
ztServicePort = 9997

remoteIP = None

# A port your app logic may use
serverPort = 8080

# Flags to keep state
is_joined = False
is_online = False
mode = None

def print_usage():
	print("\nUsage: <server|client> <id_path> <nwid> <ztServicePort> <remoteIP> <serverPort>\n")
	print("   Ex: python3 example.py server . 0123456789abcdef 9994 8080")
	print("   Ex: python3 example.py client . 0123456789abcdef 9994 192.168.22.1 8080\n")
	if (len(sys.argv) < 6):
		print('Too few arguments')
	if (len(sys.argv) > 7):
		print('Too many arguments')
	exit(0)
#
if (len(sys.argv) < 6 or len(sys.argv) > 7):
	print_usage()

if (sys.argv[1] == 'server' and len(sys.argv) == 6):
	mode          = sys.argv[1]
	keyPath       = sys.argv[2]
	networkId     = int(sys.argv[3],16)
	ztServicePort = int(sys.argv[4])
	serverPort    = int(sys.argv[5])

if (sys.argv[1] == 'client' and len(sys.argv) == 7):
	mode          = sys.argv[1]
	keyPath       = sys.argv[2]
	networkId     = int(sys.argv[3],16)
	ztServicePort = int(sys.argv[4])
	remoteIP      = sys.argv[5]
	serverPort    = int(sys.argv[6])

if (mode is None):
	print_usage()

print('mode          = ', mode)
print('path          = ', keyPath)
print('networkId     = ', networkId)
print('ztServicePort = ', ztServicePort)
print('remoteIP      = ', remoteIP)
print('serverPort    = ', serverPort)



#
# Event handler
#
class MyEventCallbackClass(libzt.PythonDirectorCallbackClass):
	def on_zerotier_event(self, msg):
		global is_online
		global is_joined
		print("eventCode=", msg.eventCode)
		if (msg.eventCode == libzt.ZTS_EVENT_NODE_ONLINE):
			print("ZTS_EVENT_NODE_ONLINE")
			print("nodeId="+hex(msg.node.address))
			# The node is now online, you can join/leave networks
			is_online = True
		if (msg.eventCode == libzt.ZTS_EVENT_NODE_OFFLINE):
			print("ZTS_EVENT_NODE_OFFLINE")
		if (msg.eventCode == libzt.ZTS_EVENT_NETWORK_READY_IP4):
			print("ZTS_EVENT_NETWORK_READY_IP4")
			is_joined = True
			# The node has successfully joined a network and has an address
			# you can perform network calls now
		if (msg.eventCode == libzt.ZTS_EVENT_PEER_DIRECT):
			print("ZTS_EVENT_PEER_DIRECT")
		if (msg.eventCode == libzt.ZTS_EVENT_PEER_RELAY):
			print("ZTS_EVENT_PEER_RELAY")



#
# Example start and join logic
#
print("Starting ZeroTier...");
eventCallback = MyEventCallbackClass()
libzt.zts_start(keyPath, eventCallback, ztServicePort)
print("Waiting for node to come online...")
while (not is_online):
	time.sleep(1)
print("Joining network:", hex(networkId));
libzt.zts_join(networkId)
while (not is_joined):
	time.sleep(1) # You can ping this app at this point
print('Joined network')



#
# Example server
#
if (mode == 'server'):
	print("Starting server...")
	try:
		serv = libzt.zerotier.socket(libzt.ZTS_AF_INET, libzt.ZTS_SOCK_STREAM, 0)
		serv.bind(('::', serverPort))
		serv.listen(5)
		while True:
			conn, addr = serv.accept()
			print('Accepted connection from: ', addr)
			while True:
				print('recv()...')
				data = conn.recv(4096)
				if data:
					print('data = ', data)
					#print(type(b'what'))
					#exit(0)
				if not data: break
				print('send()...')
				#bytes(data, 'ascii')  + b'\x00'
				n_bytes = conn.send(data) # echo back to the server
				print('sent ' + str(n_bytes) + ' byte(s)')
			conn.close()
			print('client disconnected')
	except Exception as e:
		print(e)


#
# Example client
#
if (mode == 'client'):
	print("Starting client...")
	try:
		client = libzt.zerotier.socket(libzt.ZTS_AF_INET, libzt.ZTS_SOCK_STREAM, 0)
		print("connecting...")
		client.connect((remoteIP, serverPort))
		print("send...")
		data = 'Hello, world!'
		client.send(data)
		print("rx...")
		data = client.recv(1024)

		print('Received', repr(data))
	except Exception as e:
		print(e)

