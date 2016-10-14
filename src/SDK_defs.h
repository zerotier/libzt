// --- lwIP
#define APPLICATION_POLL_FREQ           2
#define ZT_LWIP_TCP_TIMER_INTERVAL      50
#define STATUS_TMR_INTERVAL             500 // How often we check connection statuses (in ms)

// --- picoTCP 
#define MAX_PICO_FRAME_RX_BUF_SZ        ZT_MAX_MTU * 128

// --- jip


// --- General

// TCP Buffer sizes
#define DEFAULT_TCP_TX_BUF_SZ           1024 * 1024
#define DEFAULT_TCP_RX_BUF_SZ           1024 * 1024

// TCP RX/TX buffer soft boundaries
#define DEFAULT_TCP_TX_BUF_SOFTMAX      DEFAULT_TCP_TX_BUF_SZ * 0.80
#define DEFAULT_TCP_TX_BUF_SOFTMIN      DEFAULT_TCP_TX_BUF_SZ * 0.20
#define DEFAULT_TCP_RX_BUF_SOFTMAX      DEFAULT_TCP_RX_BUF_SZ * 0.80
#define DEFAULT_TCP_RX_BUF_SOFTMIN      DEFAULT_TCP_RX_BUF_SZ * 0.20

// UDP Buffer sizes (should be about the size of your MTU)
#define DEFAULT_UDP_TX_BUF_SZ           ZT_MAX_MTU
#define DEFAULT_UDP_RX_BUF_SZ           ZT_MAX_MTU * 128