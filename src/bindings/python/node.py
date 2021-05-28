import libzt

_user_specified_event_handler = None


class _EventCallbackClass(libzt.PythonDirectorCallbackClass):
    """ZeroTier event callback class"""

    pass


class MyEventCallbackClass(_EventCallbackClass):
    def on_zerotier_event(self, msg):
        id = 0
        if msg.event_code == libzt.ZTS_EVENT_NODE_ONLINE:
            id = msg.node.node_id
        if msg.event_code == libzt.ZTS_EVENT_NODE_OFFLINE:
            id = msg.node.node_id
        if msg.event_code == libzt.ZTS_EVENT_NETWORK_READY_IP4:
            id = msg.network.net_id
        if msg.event_code == libzt.ZTS_EVENT_NETWORK_READY_IP6:
            id = msg.network.net_id
        if msg.event_code == libzt.ZTS_EVENT_PEER_DIRECT:
            id = msg.peer.peer_id
        if msg.event_code == libzt.ZTS_EVENT_PEER_RELAY:
            id = msg.peer.peer_id
        # Now that we've adjusted internal state, notify user
        global _user_specified_event_handler
        if _user_specified_event_handler is not None:
            _user_specified_event_handler(msg.event_code, id)


class ZeroTierNode:
    native_event_handler = None

    def __init__(self):
        self.native_event_handler = MyEventCallbackClass()
        libzt.zts_init_set_event_handler(self.native_event_handler)

    """ZeroTier Node"""

    def init_from_storage(self, storage_path):
        """Initialize the node from storage (or tell it to write to that location)"""
        return libzt.zts_init_from_storage(storage_path)

    """Set the node's event handler"""

    def init_set_event_handler(self, event_handler):
        global _user_specified_event_handler
        _user_specified_event_handler = event_handler

    def init_set_port(self, port):
        """Set the node's primary port"""
        return libzt.zts_init_set_port(port)

    def node_start(self):
        """Start the ZeroTier service"""
        return libzt.zts_node_start()

    def node_stop(self):
        """Stop the ZeroTier service"""
        return libzt.zts_node_stop()

    def node_free(self):
        """Permanently shut down the network stack"""
        return libzt.zts_node_free()

    def net_join(self, net_id):
        """Join a ZeroTier network"""
        return libzt.zts_net_join(net_id)

    def net_leave(self, net_id):
        """Leave a ZeroTier network"""
        return libzt.zts_net_leave(net_id)

    def node_is_online(self):
        return libzt.zts_node_is_online()

    def node_id(self):
        return libzt.zts_node_get_id()

    def net_transport_is_ready(self, net_id):
        return libzt.zts_net_transport_is_ready(net_id)

    def addr_get_ipv4(self, net_id):
        return libzt.zts_py_addr_get_str(net_id, libzt.ZTS_AF_INET)

    def addr_get_ipv6(self, net_id):
        return libzt.zts_py_addr_get_str(net_id, libzt.ZTS_AF_INET6)
