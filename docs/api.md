# Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`class `[`swig_libzt::_object`](#classswig__libzt_1_1__object) | 
`class `[`ZeroTier::lwIP`](#class_zero_tier_1_1lw_i_p) | [lwIP](#class_zero_tier_1_1lw_i_p) network stack driver class
`class `[`ZeroTier::picoTCP`](#class_zero_tier_1_1pico_t_c_p) | [picoTCP](#class_zero_tier_1_1pico_t_c_p) network stack driver class
`class `[`ZeroTier::RingBuffer`](#class_zero_tier_1_1_ring_buffer) | 
`class `[`swig_libzt::sockaddr_ll`](#classswig__libzt_1_1sockaddr__ll) | 

# class `swig_libzt::_object` {#classswig__libzt_1_1__object}

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members

# class `ZeroTier::lwIP` {#class_zero_tier_1_1lw_i_p}

[lwIP](#class_zero_tier_1_1lw_i_p) network stack driver class

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public void `[`lwip_init_interface`](#class_zero_tier_1_1lw_i_p_1a60993bd84468211821ac3740ea9822d9)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,const InetAddress & ip)` | Set up an interface in the network stack for the [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public int `[`lwip_num_total_pcbs`](#class_zero_tier_1_1lw_i_p_1ac2af70733b9aa338f8a3403406cff9b7)`()` | Returns the total number of PCBs of any time or state
`public int `[`lwip_add_dns_nameserver`](#class_zero_tier_1_1lw_i_p_1a50912323288b2dd0b2096222f7c19513)`(struct sockaddr * addr)` | Registers a DNS nameserver with the network stack
`public int `[`lwip_del_dns_nameserver`](#class_zero_tier_1_1lw_i_p_1aa5197cb9c29a9e2f622f31c3eaa79fdd)`(struct sockaddr * addr)` | Un-registers a DNS nameserver from the network stack
`public void `[`lwip_loop`](#class_zero_tier_1_1lw_i_p_1a034b52649abe57458b004bc177af9154)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap)` | Main stack loop
`public void `[`lwip_eth_rx`](#class_zero_tier_1_1lw_i_p_1a5ac0764e83f0804630f16b405158b0b2)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,const MAC & from,const MAC & to,unsigned int etherType,const void * data,unsigned int len)` | Packets from the ZeroTier virtual wire enter the stack here
`public int `[`lwip_Socket`](#class_zero_tier_1_1lw_i_p_1a6d46d5cb3ac7902584f8b7d35ac49178)`(void ** pcb,int socket_family,int socket_type,int protocol)` | Creates a stack-specific "socket" or "VirtualSocket object"
`public int `[`lwip_Connect`](#class_zero_tier_1_1lw_i_p_1a4f430c14b451efdbdeffcab534eebfee)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,const struct sockaddr * addr,socklen_t addrlen)` | Connect to remote host via userspace network stack interface - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public int `[`lwip_Bind`](#class_zero_tier_1_1lw_i_p_1a9b0e5941b48293dfdb47d9a428868d03)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,const struct sockaddr * addr,socklen_t addrlen)` | Bind to a userspace network stack interface - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public int `[`lwip_Listen`](#class_zero_tier_1_1lw_i_p_1a98f33586aa90448fc766c9deba92b9c0)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,int backlog)` | Listen for incoming VirtualSockets - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public `[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * `[`lwip_Accept`](#class_zero_tier_1_1lw_i_p_1a425421976238b3e32e2755f67a5e57c8)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs)` | Accept an incoming [VirtualSocket](#class_zero_tier_1_1_virtual_socket) - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public int `[`lwip_Read`](#class_zero_tier_1_1lw_i_p_1a41648f168ebf9a2058f980c4e0e49b80)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,bool lwip_invoked)` | Read from RX buffer to application - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public int `[`lwip_Write`](#class_zero_tier_1_1lw_i_p_1a32ce1dff3a067c785c627b6aea2d38f2)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,void * data,ssize_t len)` | Write to userspace network stack - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public int `[`lwip_Close`](#class_zero_tier_1_1lw_i_p_1a0996caf14238f343d41bd04477aad52f)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs)` | Close a [VirtualSocket](#class_zero_tier_1_1_virtual_socket) - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public int `[`lwip_Shutdown`](#class_zero_tier_1_1lw_i_p_1ac23df186fc71271bdfc9aa2b45270fb2)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,int how)` | Shuts down some aspect of a [VirtualSocket](#class_zero_tier_1_1_virtual_socket) - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

## Members

#### `public void `[`lwip_init_interface`](#class_zero_tier_1_1lw_i_p_1a60993bd84468211821ac3740ea9822d9)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,const InetAddress & ip)` {#class_zero_tier_1_1lw_i_p_1a60993bd84468211821ac3740ea9822d9}

Set up an interface in the network stack for the [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public int `[`lwip_num_total_pcbs`](#class_zero_tier_1_1lw_i_p_1ac2af70733b9aa338f8a3403406cff9b7)`()` {#class_zero_tier_1_1lw_i_p_1ac2af70733b9aa338f8a3403406cff9b7}

Returns the total number of PCBs of any time or state

#### `public int `[`lwip_add_dns_nameserver`](#class_zero_tier_1_1lw_i_p_1a50912323288b2dd0b2096222f7c19513)`(struct sockaddr * addr)` {#class_zero_tier_1_1lw_i_p_1a50912323288b2dd0b2096222f7c19513}

Registers a DNS nameserver with the network stack

#### `public int `[`lwip_del_dns_nameserver`](#class_zero_tier_1_1lw_i_p_1aa5197cb9c29a9e2f622f31c3eaa79fdd)`(struct sockaddr * addr)` {#class_zero_tier_1_1lw_i_p_1aa5197cb9c29a9e2f622f31c3eaa79fdd}

Un-registers a DNS nameserver from the network stack

#### `public void `[`lwip_loop`](#class_zero_tier_1_1lw_i_p_1a034b52649abe57458b004bc177af9154)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap)` {#class_zero_tier_1_1lw_i_p_1a034b52649abe57458b004bc177af9154}

Main stack loop

#### `public void `[`lwip_eth_rx`](#class_zero_tier_1_1lw_i_p_1a5ac0764e83f0804630f16b405158b0b2)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,const MAC & from,const MAC & to,unsigned int etherType,const void * data,unsigned int len)` {#class_zero_tier_1_1lw_i_p_1a5ac0764e83f0804630f16b405158b0b2}

Packets from the ZeroTier virtual wire enter the stack here

#### `public int `[`lwip_Socket`](#class_zero_tier_1_1lw_i_p_1a6d46d5cb3ac7902584f8b7d35ac49178)`(void ** pcb,int socket_family,int socket_type,int protocol)` {#class_zero_tier_1_1lw_i_p_1a6d46d5cb3ac7902584f8b7d35ac49178}

Creates a stack-specific "socket" or "VirtualSocket object"

#### `public int `[`lwip_Connect`](#class_zero_tier_1_1lw_i_p_1a4f430c14b451efdbdeffcab534eebfee)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,const struct sockaddr * addr,socklen_t addrlen)` {#class_zero_tier_1_1lw_i_p_1a4f430c14b451efdbdeffcab534eebfee}

Connect to remote host via userspace network stack interface - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public int `[`lwip_Bind`](#class_zero_tier_1_1lw_i_p_1a9b0e5941b48293dfdb47d9a428868d03)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,const struct sockaddr * addr,socklen_t addrlen)` {#class_zero_tier_1_1lw_i_p_1a9b0e5941b48293dfdb47d9a428868d03}

Bind to a userspace network stack interface - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public int `[`lwip_Listen`](#class_zero_tier_1_1lw_i_p_1a98f33586aa90448fc766c9deba92b9c0)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,int backlog)` {#class_zero_tier_1_1lw_i_p_1a98f33586aa90448fc766c9deba92b9c0}

Listen for incoming VirtualSockets - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public `[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * `[`lwip_Accept`](#class_zero_tier_1_1lw_i_p_1a425421976238b3e32e2755f67a5e57c8)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs)` {#class_zero_tier_1_1lw_i_p_1a425421976238b3e32e2755f67a5e57c8}

Accept an incoming [VirtualSocket](#class_zero_tier_1_1_virtual_socket) - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public int `[`lwip_Read`](#class_zero_tier_1_1lw_i_p_1a41648f168ebf9a2058f980c4e0e49b80)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,bool lwip_invoked)` {#class_zero_tier_1_1lw_i_p_1a41648f168ebf9a2058f980c4e0e49b80}

Read from RX buffer to application - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public int `[`lwip_Write`](#class_zero_tier_1_1lw_i_p_1a32ce1dff3a067c785c627b6aea2d38f2)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,void * data,ssize_t len)` {#class_zero_tier_1_1lw_i_p_1a32ce1dff3a067c785c627b6aea2d38f2}

Write to userspace network stack - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public int `[`lwip_Close`](#class_zero_tier_1_1lw_i_p_1a0996caf14238f343d41bd04477aad52f)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs)` {#class_zero_tier_1_1lw_i_p_1a0996caf14238f343d41bd04477aad52f}

Close a [VirtualSocket](#class_zero_tier_1_1_virtual_socket) - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public int `[`lwip_Shutdown`](#class_zero_tier_1_1lw_i_p_1ac23df186fc71271bdfc9aa2b45270fb2)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,int how)` {#class_zero_tier_1_1lw_i_p_1ac23df186fc71271bdfc9aa2b45270fb2}

Shuts down some aspect of a [VirtualSocket](#class_zero_tier_1_1_virtual_socket) - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

# class `ZeroTier::picoTCP` {#class_zero_tier_1_1pico_t_c_p}

[picoTCP](#class_zero_tier_1_1pico_t_c_p) network stack driver class

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public bool `[`pico_init_interface`](#class_zero_tier_1_1pico_t_c_p_1a2b7c5d6eb2d82dbbdbc89e771d79bb46)`(`[`ZeroTier::VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap)` | Set up an interface in the network stack for the [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public bool `[`pico_register_address`](#class_zero_tier_1_1pico_t_c_p_1aa6707bde6c134c7dabe0d3d6b21f4a1d)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,const InetAddress & ip)` | Register an address with the stack
`public bool `[`pico_route_add`](#class_zero_tier_1_1pico_t_c_p_1a0eb83b0d5e2216a660e3cb362b711468)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,const InetAddress & addr,const InetAddress & nm,const InetAddress & gw,int metric)` | Adds a route to the [picoTCP](#class_zero_tier_1_1pico_t_c_p) device
`public bool `[`pico_route_del`](#class_zero_tier_1_1pico_t_c_p_1a150d1b22d58f1db9a2a7e1947a95b6b8)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,const InetAddress & addr,const InetAddress & nm,int metric)` | Deletes a route from the [picoTCP](#class_zero_tier_1_1pico_t_c_p) device
`public int `[`pico_add_dns_nameserver`](#class_zero_tier_1_1pico_t_c_p_1a03e41c0adf5fc2e7fe142bc8fcc598e3)`(struct sockaddr * addr)` | Registers a DNS nameserver with the network stack
`public int `[`pico_del_dns_nameserver`](#class_zero_tier_1_1pico_t_c_p_1a70916ff5ce56493e5e4593a15c2f943d)`(struct sockaddr * addr)` | Un-registers a DNS nameserver from the network stack
`public void `[`pico_loop`](#class_zero_tier_1_1pico_t_c_p_1ac0362a390aa425307c5ceda8a4919d91)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap)` | Main stack loop
`public void `[`pico_eth_rx`](#class_zero_tier_1_1pico_t_c_p_1a2ecda3823555b33972f4f823319d0810)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,const ZeroTier::MAC & from,const ZeroTier::MAC & to,unsigned int etherType,const void * data,unsigned int len)` | Packets from the ZeroTier virtual wire enter the stack here
`public int `[`pico_Socket`](#class_zero_tier_1_1pico_t_c_p_1ab50d2e20f2809d50af81572c10f483b3)`(struct pico_socket ** p,int socket_family,int socket_type,int protocol)` | Creates a stack-specific "socket" or "VirtualSocket object"
`public int `[`pico_Connect`](#class_zero_tier_1_1pico_t_c_p_1a50adbf489ac93aa552074ff310c5ce58)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,const struct sockaddr * addr,socklen_t addrlen)` | Connect to remote host via userspace network stack interface - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public int `[`pico_Bind`](#class_zero_tier_1_1pico_t_c_p_1a8dc43551315ca811fafd5d36ce247a96)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,const struct sockaddr * addr,socklen_t addrlen)` | Bind to a userspace network stack interface - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public int `[`pico_Listen`](#class_zero_tier_1_1pico_t_c_p_1a89ff688290b1f95e2d4ada3838864eb8)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,int backlog)` | Listen for incoming VirtualSockets - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public `[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * `[`pico_Accept`](#class_zero_tier_1_1pico_t_c_p_1aae54932a41b848796fb40eef746eb1c1)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs)` | Accept an incoming [VirtualSocket](#class_zero_tier_1_1_virtual_socket) - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public int `[`pico_Read`](#class_zero_tier_1_1pico_t_c_p_1a018aafaf6f7825e4a50e419b43562c7e)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,ZeroTier::PhySocket * sock,`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,bool stack_invoked)` | Read from RX buffer to application - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public int `[`pico_Write`](#class_zero_tier_1_1pico_t_c_p_1ad218cb8522ce3b4b12a08716bd6ac3e4)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,void * data,ssize_t len)` | Write to userspace network stack - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public int `[`pico_Close`](#class_zero_tier_1_1pico_t_c_p_1ae9a940d05212741e6fcfad3c1830587f)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs)` | Close a [VirtualSocket](#class_zero_tier_1_1_virtual_socket) - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)
`public int `[`pico_Shutdown`](#class_zero_tier_1_1pico_t_c_p_1aeddf3ff69cf70e94b0793a2b33ae00a1)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,int how)` | Shuts down some aspect of a [VirtualSocket](#class_zero_tier_1_1_virtual_socket) - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

## Members

#### `public bool `[`pico_init_interface`](#class_zero_tier_1_1pico_t_c_p_1a2b7c5d6eb2d82dbbdbc89e771d79bb46)`(`[`ZeroTier::VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap)` {#class_zero_tier_1_1pico_t_c_p_1a2b7c5d6eb2d82dbbdbc89e771d79bb46}

Set up an interface in the network stack for the [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public bool `[`pico_register_address`](#class_zero_tier_1_1pico_t_c_p_1aa6707bde6c134c7dabe0d3d6b21f4a1d)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,const InetAddress & ip)` {#class_zero_tier_1_1pico_t_c_p_1aa6707bde6c134c7dabe0d3d6b21f4a1d}

Register an address with the stack

#### `public bool `[`pico_route_add`](#class_zero_tier_1_1pico_t_c_p_1a0eb83b0d5e2216a660e3cb362b711468)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,const InetAddress & addr,const InetAddress & nm,const InetAddress & gw,int metric)` {#class_zero_tier_1_1pico_t_c_p_1a0eb83b0d5e2216a660e3cb362b711468}

Adds a route to the [picoTCP](#class_zero_tier_1_1pico_t_c_p) device

#### `public bool `[`pico_route_del`](#class_zero_tier_1_1pico_t_c_p_1a150d1b22d58f1db9a2a7e1947a95b6b8)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,const InetAddress & addr,const InetAddress & nm,int metric)` {#class_zero_tier_1_1pico_t_c_p_1a150d1b22d58f1db9a2a7e1947a95b6b8}

Deletes a route from the [picoTCP](#class_zero_tier_1_1pico_t_c_p) device

#### `public int `[`pico_add_dns_nameserver`](#class_zero_tier_1_1pico_t_c_p_1a03e41c0adf5fc2e7fe142bc8fcc598e3)`(struct sockaddr * addr)` {#class_zero_tier_1_1pico_t_c_p_1a03e41c0adf5fc2e7fe142bc8fcc598e3}

Registers a DNS nameserver with the network stack

#### `public int `[`pico_del_dns_nameserver`](#class_zero_tier_1_1pico_t_c_p_1a70916ff5ce56493e5e4593a15c2f943d)`(struct sockaddr * addr)` {#class_zero_tier_1_1pico_t_c_p_1a70916ff5ce56493e5e4593a15c2f943d}

Un-registers a DNS nameserver from the network stack

#### `public void `[`pico_loop`](#class_zero_tier_1_1pico_t_c_p_1ac0362a390aa425307c5ceda8a4919d91)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap)` {#class_zero_tier_1_1pico_t_c_p_1ac0362a390aa425307c5ceda8a4919d91}

Main stack loop

#### `public void `[`pico_eth_rx`](#class_zero_tier_1_1pico_t_c_p_1a2ecda3823555b33972f4f823319d0810)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,const ZeroTier::MAC & from,const ZeroTier::MAC & to,unsigned int etherType,const void * data,unsigned int len)` {#class_zero_tier_1_1pico_t_c_p_1a2ecda3823555b33972f4f823319d0810}

Packets from the ZeroTier virtual wire enter the stack here

#### `public int `[`pico_Socket`](#class_zero_tier_1_1pico_t_c_p_1ab50d2e20f2809d50af81572c10f483b3)`(struct pico_socket ** p,int socket_family,int socket_type,int protocol)` {#class_zero_tier_1_1pico_t_c_p_1ab50d2e20f2809d50af81572c10f483b3}

Creates a stack-specific "socket" or "VirtualSocket object"

#### `public int `[`pico_Connect`](#class_zero_tier_1_1pico_t_c_p_1a50adbf489ac93aa552074ff310c5ce58)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,const struct sockaddr * addr,socklen_t addrlen)` {#class_zero_tier_1_1pico_t_c_p_1a50adbf489ac93aa552074ff310c5ce58}

Connect to remote host via userspace network stack interface - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public int `[`pico_Bind`](#class_zero_tier_1_1pico_t_c_p_1a8dc43551315ca811fafd5d36ce247a96)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,const struct sockaddr * addr,socklen_t addrlen)` {#class_zero_tier_1_1pico_t_c_p_1a8dc43551315ca811fafd5d36ce247a96}

Bind to a userspace network stack interface - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public int `[`pico_Listen`](#class_zero_tier_1_1pico_t_c_p_1a89ff688290b1f95e2d4ada3838864eb8)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,int backlog)` {#class_zero_tier_1_1pico_t_c_p_1a89ff688290b1f95e2d4ada3838864eb8}

Listen for incoming VirtualSockets - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public `[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * `[`pico_Accept`](#class_zero_tier_1_1pico_t_c_p_1aae54932a41b848796fb40eef746eb1c1)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs)` {#class_zero_tier_1_1pico_t_c_p_1aae54932a41b848796fb40eef746eb1c1}

Accept an incoming [VirtualSocket](#class_zero_tier_1_1_virtual_socket) - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public int `[`pico_Read`](#class_zero_tier_1_1pico_t_c_p_1a018aafaf6f7825e4a50e419b43562c7e)`(`[`VirtualTap`](#class_zero_tier_1_1_virtual_tap)` * tap,ZeroTier::PhySocket * sock,`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,bool stack_invoked)` {#class_zero_tier_1_1pico_t_c_p_1a018aafaf6f7825e4a50e419b43562c7e}

Read from RX buffer to application - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public int `[`pico_Write`](#class_zero_tier_1_1pico_t_c_p_1ad218cb8522ce3b4b12a08716bd6ac3e4)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,void * data,ssize_t len)` {#class_zero_tier_1_1pico_t_c_p_1ad218cb8522ce3b4b12a08716bd6ac3e4}

Write to userspace network stack - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public int `[`pico_Close`](#class_zero_tier_1_1pico_t_c_p_1ae9a940d05212741e6fcfad3c1830587f)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs)` {#class_zero_tier_1_1pico_t_c_p_1ae9a940d05212741e6fcfad3c1830587f}

Close a [VirtualSocket](#class_zero_tier_1_1_virtual_socket) - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

#### `public int `[`pico_Shutdown`](#class_zero_tier_1_1pico_t_c_p_1aeddf3ff69cf70e94b0793a2b33ae00a1)`(`[`VirtualSocket`](#class_zero_tier_1_1_virtual_socket)` * vs,int how)` {#class_zero_tier_1_1pico_t_c_p_1aeddf3ff69cf70e94b0793a2b33ae00a1}

Shuts down some aspect of a [VirtualSocket](#class_zero_tier_1_1_virtual_socket) - Called from [VirtualTap](#class_zero_tier_1_1_virtual_tap)

# class `ZeroTier::RingBuffer` {#class_zero_tier_1_1_ring_buffer}

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  explicit `[`RingBuffer`](#class_zero_tier_1_1_ring_buffer_1aebe17f1aeb326b0a8aea386c7b7e6c20)`(size_t size)` | create a [RingBuffer](#class_zero_tier_1_1_ring_buffer) with space for up to size elements.
`public inline  `[`RingBuffer`](#class_zero_tier_1_1_ring_buffer_1a9e155757874ede65827ceb4997ac7cb0)`(const `[`RingBuffer`](#class_zero_tier_1_1_ring_buffer)`< T > & ring)` | 
`public inline  `[`~RingBuffer`](#class_zero_tier_1_1_ring_buffer_1ab9689c75059d8e0bc9e0738949bd79f1)`()` | 
`public inline T * `[`get_buf`](#class_zero_tier_1_1_ring_buffer_1adcb011631c37786127fefc6ffc680054)`()` | 
`public inline size_t `[`produce`](#class_zero_tier_1_1_ring_buffer_1a092c42678950af6cc93f86b88528d884)`(size_t n)` | 
`public inline size_t `[`consume`](#class_zero_tier_1_1_ring_buffer_1a7180600ec76acc17fd3e9dc5c7b4c6dd)`(size_t n)` | 
`public inline size_t `[`write`](#class_zero_tier_1_1_ring_buffer_1a93355378858604110e7c647430183ab4)`(const T * data,size_t n)` | 
`public inline size_t `[`read`](#class_zero_tier_1_1_ring_buffer_1a7fead1ebb1231842d820f4e1f4d11b1a)`(T * dest,size_t n)` | 
`public inline size_t `[`count`](#class_zero_tier_1_1_ring_buffer_1a2150fb027bd0956520d21fb2d8d2a8cf)`()` | 
`public inline size_t `[`getFree`](#class_zero_tier_1_1_ring_buffer_1a28d95936deeff51268b13c4c09905f18)`()` | 

## Members

#### `public inline  explicit `[`RingBuffer`](#class_zero_tier_1_1_ring_buffer_1aebe17f1aeb326b0a8aea386c7b7e6c20)`(size_t size)` {#class_zero_tier_1_1_ring_buffer_1aebe17f1aeb326b0a8aea386c7b7e6c20}

create a [RingBuffer](#class_zero_tier_1_1_ring_buffer) with space for up to size elements.

#### `public inline  `[`RingBuffer`](#class_zero_tier_1_1_ring_buffer_1a9e155757874ede65827ceb4997ac7cb0)`(const `[`RingBuffer`](#class_zero_tier_1_1_ring_buffer)`< T > & ring)` {#class_zero_tier_1_1_ring_buffer_1a9e155757874ede65827ceb4997ac7cb0}

#### `public inline  `[`~RingBuffer`](#class_zero_tier_1_1_ring_buffer_1ab9689c75059d8e0bc9e0738949bd79f1)`()` {#class_zero_tier_1_1_ring_buffer_1ab9689c75059d8e0bc9e0738949bd79f1}

#### `public inline T * `[`get_buf`](#class_zero_tier_1_1_ring_buffer_1adcb011631c37786127fefc6ffc680054)`()` {#class_zero_tier_1_1_ring_buffer_1adcb011631c37786127fefc6ffc680054}

#### `public inline size_t `[`produce`](#class_zero_tier_1_1_ring_buffer_1a092c42678950af6cc93f86b88528d884)`(size_t n)` {#class_zero_tier_1_1_ring_buffer_1a092c42678950af6cc93f86b88528d884}

#### `public inline size_t `[`consume`](#class_zero_tier_1_1_ring_buffer_1a7180600ec76acc17fd3e9dc5c7b4c6dd)`(size_t n)` {#class_zero_tier_1_1_ring_buffer_1a7180600ec76acc17fd3e9dc5c7b4c6dd}

#### `public inline size_t `[`write`](#class_zero_tier_1_1_ring_buffer_1a93355378858604110e7c647430183ab4)`(const T * data,size_t n)` {#class_zero_tier_1_1_ring_buffer_1a93355378858604110e7c647430183ab4}

#### `public inline size_t `[`read`](#class_zero_tier_1_1_ring_buffer_1a7fead1ebb1231842d820f4e1f4d11b1a)`(T * dest,size_t n)` {#class_zero_tier_1_1_ring_buffer_1a7fead1ebb1231842d820f4e1f4d11b1a}

#### `public inline size_t `[`count`](#class_zero_tier_1_1_ring_buffer_1a2150fb027bd0956520d21fb2d8d2a8cf)`()` {#class_zero_tier_1_1_ring_buffer_1a2150fb027bd0956520d21fb2d8d2a8cf}

#### `public inline size_t `[`getFree`](#class_zero_tier_1_1_ring_buffer_1a28d95936deeff51268b13c4c09905f18)`()` {#class_zero_tier_1_1_ring_buffer_1a28d95936deeff51268b13c4c09905f18}

# class `swig_libzt::sockaddr_ll` {#classswig__libzt_1_1sockaddr__ll}

```
class swig_libzt::sockaddr_ll
  : public swig_libzt._object
```  

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  `[`this`](#classswig__libzt_1_1sockaddr__ll_1a5cf75468bf5aba9ec0d629bf9f2e5410) | 
`public def `[`__init__`](#classswig__libzt_1_1sockaddr__ll_1a454f730652c2759be217e4acecf9caea)`(self)` | 

## Members

#### `public  `[`this`](#classswig__libzt_1_1sockaddr__ll_1a5cf75468bf5aba9ec0d629bf9f2e5410) {#classswig__libzt_1_1sockaddr__ll_1a5cf75468bf5aba9ec0d629bf9f2e5410}

#### `public def `[`__init__`](#classswig__libzt_1_1sockaddr__ll_1a454f730652c2759be217e4acecf9caea)`(self)` {#classswig__libzt_1_1sockaddr__ll_1a454f730652c2759be217e4acecf9caea}

Generated by [Moxygen](https://sourcey.com/moxygen)