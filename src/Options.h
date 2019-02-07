#ifndef LIBZT_OPTIONS_H
#define LIBZT_OPTIONS_H

//////////////////////////////////////////////////////////////////////////////
// Callbacks                                                                //
//////////////////////////////////////////////////////////////////////////////

#define ZTS_NODE_CALLBACKS    1
#define ZTS_NETWORK_CALLBACKS 1
#define ZTS_NETIF_CALLBACKS   1
#define ZTS_PEER_CALLBACKS    1

/**
 * The maximum number of un-processed callback messages
 */
#define ZTS_CALLBACK_MSG_QUEUE_LEN 256

//////////////////////////////////////////////////////////////////////////////
// Timing                                                                   //
//////////////////////////////////////////////////////////////////////////////

/**
 * How often callback messages are assembled and/or sent
 */
#define ZTS_CALLBACK_PROCESSING_INTERVAL 25

/**
 * Polling interval (in ms) for file descriptors wrapped in the Phy I/O loop (for raw drivers only)
 */
#define ZTS_PHY_POLL_INTERVAL 1

#define ZTS_HOUSEKEEPING_INTERVAL 50

/**
 * By how much thread I/O and callback loop delays are multiplied (unitless)
 */
#define ZTS_HIBERNATION_MULTIPLIER 50

//////////////////////////////////////////////////////////////////////////////
// Thread names                                                             //
//////////////////////////////////////////////////////////////////////////////

#define ZTS_SERVICE_THREAD_NAME "ZeroTierServiceThread"

#define ZTS_EVENT_CALLBACK_THREAD_NAME "ZeroTierEventCallbackThread"




#define LWIP_FRAMES_HANDLED_PER_CORE_CALL 16 // How many frames are handled per call from core
#define LWIP_GUARDED_BUF_CHECK_INTERVAL 5 // in ms
#define LWIP_MAX_GUARDED_RX_BUF_SZ 1024 // number of frame pointers that can be cached waiting for receipt into core



#define PEER_CACHING 0


#endif